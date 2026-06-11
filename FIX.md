# Fix: "libgtk-3-0.dll introuvable" Error on Windows

## Symptom

When launching `serial-scanner.exe` on Windows (double-clicking from File Explorer, running from CMD or PowerShell), the application fails with:

```
Impossible d'executer le code, car libgtk-3-0.dll est introuvable
(The code cannot be executed because libgtk-3-0.dll was not found)
```

## Root Cause

MSYS2/MinGW-w64 packages install GTK3 and all ~30+ transitive dependency DLLs into:

```
C:\msys64\mingw64\bin\
```

This directory is **NOT** in the standard Windows PATH. The MSYS2 shell (`/usr/bin/msys-2.0.dll`) adds it to PATH automatically, but only when you launch a terminal from the MSYS2 environment.

When you run the `.exe` from File Explorer, CMD, or PowerShell, the Windows PE loader searches:
1. The directory containing the `.exe` -- **DLLs are not there**
2. `C:\Windows\System32\`, `C:\Windows\SysWOW64\`, etc. -- **DLLs are not there**
3. The directories listed in the system `PATH` -- **`C:\msys64\mingw64\bin\` is not there**

Result: **the loader fails immediately.**

## Solution Overview

Three solutions are provided, from simplest to most robust:

| Solution | Use Case | Effort |
|----------|----------|--------|
| A. Run from MSYS2 shell | Development only | Zero |
| B. CMake post-build DLL copy | Distribution (recommended) | Zero (auto) |
| C. Batch launcher script | Distribution (manual) | One-time setup |

---

## Solution A: Run from MSYS2 MinGW64 Shell (Development Only)

If you are developing and testing, simply launch the application from the MSYS2 MinGW64 terminal:

```bash
# Open "MSYS2 MinGW64" from the Start Menu
cd /path/to/Serial-scanner/build
./serial-scanner
```

The MSYS2 shell automatically sets `PATH` to include `/mingw64/bin`, so all DLLs are found.

**Do NOT use this for end-user distribution.**

---

## Solution B: CMake Post-Build DLL Copy (Recommended for Distribution)

The CMakeLists.txt has been updated with an automatic post-build step that copies all required DLLs next to the `.exe` after every build. This is the **recommended approach** for distribution.

### How It Works

1. At CMake configure time, the script searches for the MinGW64 DLL directory using three strategies:
   - Query `pkg-config` for GTK3 library paths and resolve the DLL directory
   - Fall back to `GTK3_LIBRARY_DIRS` from `pkg_check_modules`
   - Last resort: check standard paths (`C:\msys64\mingw64\bin\`, `D:\msys64\mingw64\bin\`)

2. At build time (POST_BUILD), a generated batch script copies every required DLL into the same directory as the `.exe`.

3. The resulting build directory contains a fully self-contained application.

### Usage

```bash
# Standard build (DLLs are auto-copied on Windows)
cd Serial-scanner
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make

# The .exe and all DLLs are now in the build/ directory
# You can copy the entire build/ directory to distribute
```

To **disable** automatic DLL bundling (e.g., for Linux builds on a cross-compiler):

```bash
cmake .. -DWINDOWS_BUNDLE_DLLS=OFF
```

### What Gets Copied

The script discovers DLLs from the linker flags and also bundles these always-required system DLLs:
- `libwinpthread-1.dll` (threading)
- `libstdc++-6.dll` (C++ runtime)
- `libgcc_s_seh-1.dll` (GCC runtime)
- `libserialport-0.dll` (serial port library)
- Plus all GTK3 transitive dependencies (~30 DLLs)

---

## Solution C: Batch Launcher Script (Manual Distribution)

If you prefer not to modify the build process, use the provided batch scripts in the `dist/` directory.

### Option 1: One-Time DLL Setup

1. Build the project as usual (from MSYS2 shell or CI)
2. Copy `serial-scanner.exe` and `dist/setup_dlls.bat` to a distribution folder
3. Run `setup_dlls.bat` -- it copies all DLLs automatically
4. Distribute the folder

```batch
REM In the distribution folder:
setup_dlls.bat
serial-scanner.exe   <-- now works without MSYS2 shell
```

### Option 2: Batch Launcher with PATH Setup

1. Build the project
2. Copy `serial-scanner.exe`, `dist/serial-scanner.bat`, and `dist/setup_dlls.bat` to the distribution folder
3. Run `serial-scanner.bat` instead of the `.exe` directly

The launcher checks if DLLs are bundled. If not, it finds the MSYS2 installation and adds it to PATH before launching.

---

## Complete Distribution Checklist

When preparing a Windows release, verify:

- [ ] `serial-scanner.exe` is present
- [ ] All DLLs are present in the same directory (run `setup_dlls.bat` to copy them)
- [ ] `README.md` is present (for user documentation)
- [ ] Test on a **clean Windows machine** (or VM) with no MSYS2 installed
- [ ] The application launches without any PATH configuration

### Files in a Distribution Package

```
serial-scanner/
  serial-scanner.exe        <-- the application
  libgtk-3-0.dll            <-- GTK3 core (auto-copied)
  libgdk-3-0.dll            <-- GDK core (auto-copied)
  libgdk_pixbuf-2.0-0.dll   <-- GDK Pixbuf (auto-copied)
  libglib-2.0-0.dll         <-- GLib (auto-copied)
  ... (30+ more DLLs) ...
  README.md                 <-- user documentation
```

---

## Troubleshooting

### "Some DLLs not found" after running setup_dlls.bat

Install the full set of MSYS2 MinGW-w64 packages:

```bash
# In MSYS2 MinGW64 terminal:
pacman -S mingw-w64-x86_64-gtk3 mingw-w64-x86_64-libserialport
```

### Still getting DLL errors after copying?

Use `ldd` (from MSYS2) to check dependencies:

```bash
ldd serial-scanner.exe
```

Any DLLs showing "not found" are missing from your distribution folder.

### Building on CI (GitHub Actions)

The CI workflow already builds in the MSYS2 environment. To package DLLs in CI, add the DLL copy step:

```yaml
- name: Bundle Windows DLLs
  if: runner.os == 'Windows'
  shell: msys2 {0}
  run: |
    cd build
    cmake --build . --config Release
    # DLLs are auto-copied by the post-build step
    ls *.dll  # verify they were copied
```

---

## Files Modified/Created

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Updated with `WINDOWS_BUNDLE_DLLS` option and `copy_dlls.cmake` include |
| `cmake/copy_dlls.cmake` | New: CMake script for auto-discovering and copying DLLs |
| `dist/setup_dlls.bat` | New: Batch script for one-time DLL copying |
| `dist/serial-scanner.bat` | New: Batch launcher with automatic PATH fallback |
| `FIX.md` | This file |
