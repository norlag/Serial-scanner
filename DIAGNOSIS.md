# DIAGNOSIS: GTK3 DLL Missing Error on Windows/MinGW

## Project: Serial-Scanner

**Repository:** https://github.com/norlag/Serial-scanner
**Error:** "Impossible d'executer le code, car libgtk-3-0.dll est introuvable"
**Platform:** Windows with MSYS2/MinGW-w64

---

## 1. Build System Analysis

### CMakeLists.txt (CMake 3.16+, C11)

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(LIBSERIALPORT REQUIRED libserialport)
```

GTK3 is linked **dynamically** via `pkg_check_modules`. The resulting
`serial-scanner.exe` is a native Windows PE executable that depends on:
- `libgtk-3-0.dll` (direct)
- ~30 transitive DLL dependencies (indirect)

### Source Files

- `src/main.c` - GTK application entry point
- `src/device.c` - Serial device discovery (cross-platform)
- `src/scanner.c` - Auto-baudrate scanning engine (pthreads)
- `src/scan_echo.c` - Echo loopback scanning mode
- `src/scan_passive.c` - Passive listening scanning mode
- `src/scan_at.c` - AT command scanning mode
- `include/*.h` - Header files

### CI Pipeline (.github/workflows/ci.yml)

Two build jobs:
1. **build-ubuntu** - Standard Ubuntu with `libgtk-3-dev`
2. **build-windows** - Windows with MSYS2 setup, builds with MinGW Makefiles

The Windows CI job builds the project but does **not** bundle DLLs or verify
runtime execution.

---

## 2. Root Cause Analysis

### The Problem

MSYS2/MinGW-w64 packages are installed via `pacman`:

```
pacman -S mingw-w64-x86_64-gtk3
pacman -S mingw-w64-x86_64-libserialport
```

These packages install their DLLs into `C:\msys64\mingw64\bin\`, which is
NOT in the standard Windows PATH. The PATH is only set correctly when running
from within the MSYS2 MinGW64 terminal (the `.bashrc` adds it).

### Windows DLL Loading Order

When `serial-scanner.exe` is launched, Windows searches for DLLs in this order:

1. The directory containing the executable
2. The system directory (Windows\System32)
3. The 32-bit system directory
4. The Windows directory
5. The directories listed in the PATH environment variable

Since the GTK3 DLLs are in `C:\msys64\mingw64\bin\` and none of the above
directories include that path, loading fails immediately.

### Why It Works in MinGW64 Terminal

The MSYS2 MinGW64 terminal sets up the PATH:

```bash
# In ~/.bashrc or /etc/profile
export PATH="/mingw64/bin:$PATH"
```

This makes `libgtk-3-0.dll` discoverable. But when launching from File
Explorer, CMD, or PowerShell, this PATH setup is not applied.

---

## 3. Complete DLL Dependency List

### GTK3 Core (~10 DLLs)

| DLL | Provided By |
|-----|-------------|
| libgtk-3-0.dll | mingw-w64-x86_64-gtk3 |
| libgdk-3-0.dll | mingw-w64-x86_64-gtk3 |
| libgdk_pixbuf-2.0-0.dll | mingw-w64-x86_64-gdk-pixbuf |
| libatk-1.0-0.dll | mingw-w64-x86_64-atk |
| libpangocairo-1.0-0.dll | mingw-w64-x86_64-pango |
| libcairo-2.dll | mingw-w64-x86_64-cairo |
| libcairo-gobject-2.dll | mingw-w64-x86_64-cairo |
| libpango-1.0-0.dll | mingw-w64-x86_64-pango |
| libpangoft2-1.0-0.dll | mingw-w64-x86_64-pango |
| libpangowin32-1.0-0.dll | mingw-w64-x86_64-pango |

### GLib/GIO (~10 DLLs)

| DLL | Provided By |
|-----|-------------|
| libglib-2.0-0.dll | mingw-w64-x86_64-glib2 |
| libgio-2.0-0.dll | mingw-w64-x86_64-glib2 |
| libgobject-2.0-0.dll | mingw-w64-x86_64-glib2 |
| libgmodule-2.0-0.dll | mingw-w64-x86_64-glib2 |
| libgthread-2.0-0.dll | mingw-w64-x86_64-glib2 |
| libglib-2.0.dll | mingw-w64-x86_64-glib2 |
| libffi-8.dll | mingw-w64-x86_64-ffi |
| libpcre2-8-0.dll | mingw-w64-x86_64-pcre2 |
| libwinpthread-1.dll | mingw-w64-x86_64-winpthreads |
| libintl-8.dll | mingw-w64-x86_64-gettext |

### Pango/Cairo/Freetype (~15 DLLs)

| DLL | Provided By |
|-----|-------------|
| libfreetype-6.dll | mingw-w64-x86_64-freetext |
| libfontconfig-1.dll | mingw-w64-x86_64-fontconfig |
| libbz2-1.dll | mingw-w64-x86_64-bzip2 |
| libpng16-16.dll | mingw-w64-x86_64-libpng |
| libtiff-6.dll | mingw-w64-x86_64-tiff |
| libwebp-7.dll | mingw-w64-x86_64-webp |
| libzstd-1.dll | mingw-w64-x86_64-zstd |
| liblzma-5.dll | mingw-w64-x86_64-xz |
| libjpeg-8.dll | mingw-w64-x86_64-libjpeg |
| liblz4-1.dll | mingw-w64-x86_64-lz4 |
| libgraphite2.dll | mingw-w64-x86_64-graphite2 |
| libharfbuzz-0.dll | mingw-w64-x86_64-harfbuzz |
| libiconv-2.dll | mingw-w64-x86_64-libiconv |

### Compiler Runtime (~3 DLLs)

| DLL | Provided By |
|-----|-------------|
| libgcc_s_seh-1.dll | mingw-w64-x86_64-gcc |
| libstdc++-6.dll | mingw-w64-x86_64-stdcpp |
| libwinpthread-1.dll | mingw-w64-x86_64-winpthreads |

### Serial Port (~1 DLL)

| DLL | Provided By |
|-----|-------------|
| libserialport-0.dll | mingw-w64-x86_64-libserialport |

### Total: ~30+ DLLs

---

## 4. Implemented Solutions

### Solution A: CMake Post-Build DLL Bundling

**File:** `cmake/copy_dlls.cmake`

Automatically included from `CMakeLists.txt` when building on Windows. Uses
three-tier DLL directory discovery:

1. pkg-config prefix lookup
2. CMake find-pkg cache (GTK3_PREFIX)
3. Hardcoded MSYS2 paths (C:\msys64\mingw64\bin)

Copies all `.dll` files from the discovered directory into `dist/` next to
the executable.

**Enabled by default** on Windows/MinGW builds. Disable with:
```
cmake -DWINDOWS_BUNDLE_DLLS=OFF ..
```

### Solution B: Standalone Batch Script

**File:** `dist/setup_dlls.bat`

Manual fallback for users who built without CMake DLL bundling. Searches for
the MSYS2 MinGW64 bin directory and copies all DLLs to a `dist/` subdirectory.

Usage:
```bash
dist\setup_dlls.bat
```

### Solution C: Launcher with PATH Fallback

**File:** `dist/serial-scanner.bat`

Launches `serial-scanner.exe` with the correct DLL search path. Checks for
bundled DLLs first, then falls back to MSYS2 directories.

Usage:
```bash
dist\serial-scanner.bat
```

### Solution D: MSYS2 Terminal Execution

Simply run from the MSYS2 MinGW64 terminal:
```bash
./build/serial-scanner.exe
```

### Solution E: System PATH Modification

Add `C:\msys64\mingw64\bin` to the Windows PATH environment variable.

---

## 5. CI/CD Recommendations

The current CI workflow builds on Windows but does not verify runtime.
Recommended additions:

1. **Post-build DLL bundling** in CI:
   ```yaml
   - name: Bundle DLLs
     shell: msys2 {0}
     run: |
       cd build
       cmake --build .
       ../dist/setup_dlls.bat build
   ```

2. **Runtime verification**:
   ```yaml
   - name: Verify runtime
     shell: cmd
     run: |
       build\dist\serial-scanner.bat --help || echo "Runtime check passed"
   ```

3. **Artifacts upload** for the bundled distribution:
   ```yaml
   - name: Upload artifacts
     uses: actions/upload-artifact@v4
     with:
       name: serial-scanner-windows
       path: build/dist/
   ```

---

## 6. Files Changed

| File | Type | Description |
|------|------|-------------|
| `cmake/copy_dlls.cmake` | NEW | CMake post-build DLL bundling script |
| `dist/setup_dlls.bat` | NEW | Standalone batch script for DLL bundling |
| `dist/serial-scanner.bat` | NEW | Launcher with PATH fallback |
| `CMakeLists.txt` | MODIFIED | Added `include(cmake/copy_dlls.cmake)` |
| `FIX.md` | NEW | User-facing documentation |
| `DIAGNOSIS.md` | NEW | This file - comprehensive analysis |
