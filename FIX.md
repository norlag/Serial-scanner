# Fix for GTK3 DLL Missing Error on Windows

## Problem

When running `serial-scanner.exe` on Windows (built with MinGW/MSYS2), the
executable fails with the error:

```
Impossible d'executer le code, car libgtk-3-0.dll est introuvable dans votre ordinateur.
Essayer de reinstaller l'application pour regler le probleme.
```

Translation: "Cannot run the code because libgtk-3-0.dll was not found on your computer. Try reinstalling the application to fix the problem."

## Root Cause

MSYS2/MinGW-w64 installs GTK3 and its ~30 transitive dependency DLLs into
`C:\msys64\mingw64\bin\`, which is NOT in the standard Windows PATH. When
the `.exe` is launched from File Explorer, CMD, or PowerShell, the Windows PE
loader cannot find `libgtk-3-0.dll` or any of its dependencies.

The CMake build correctly links against the GTK3 libraries, but the resulting
`.exe` expects its DLL dependencies to be in the same directory as the
executable, in the Windows system directories, or in the PATH. None of these
conditions are met by default on a MinGW/MSYS2 installation.

## Solutions

### Option A: CMake Post-Build DLL Bundling (Recommended for Distribution)

The project now includes `cmake/copy_dlls.cmake` which is automatically
included from `CMakeLists.txt`. When building on Windows, it:

1. Discovers the MinGW64 DLL directory using three fallback strategies
2. Copies all `.dll` files from that directory next to the executable
3. Creates a `dist/` subdirectory containing everything needed to run

To build with DLL bundling:

```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

The bundled files will be in `build/dist/`. Launch via:
```
build\dist\serial-scanner.bat
```

To disable: `cmake -DWINDOWS_BUNDLE_DLLS=OFF ..`

### Option B: Standalone Batch Script (Manual Distribution)

If you built without DLL bundling (or want to bundle manually):

1. Copy `serial-scanner.exe` to a working directory
2. Run `dist\setup_dlls.bat` to bundle all required DLLs:

```bash
dist\setup_dlls.bat
```

This creates a `dist/` subdirectory with all needed DLLs. Launch via:
```
dist\serial-scanner.bat
```

### Option C: Run from MSYS2 MinGW64 Terminal

If you have MSYS2 installed, simply launch the executable from the MinGW64
terminal, where the PATH is automatically configured:

```bash
# In MSYS2 MinGW64 terminal
./build/serial-scanner.exe
```

### Option D: Add MSYS2 to Windows PATH

Add `C:\msys64\mingw64\bin` to the Windows system PATH (System Properties ->
Environment Variables -> Path). After restarting any open terminals, the
executable will work from anywhere.

## Required DLLs

The complete list of DLLs required at runtime includes:

**GTK3 Core:**
- libgtk-3-0.dll, libgdk-3-0.dll, libgdk_pixbuf-2.0-0.dll

**GLib/GIO:**
- libglib-2.0-0.dll, libgio-2.0-0.dll, libgmodule-2.0-0.dll,
  libgobject-2.0-0.dll, libgthread-2.0-0.dll, libglib-2.0.dll,
  libffi-8.dll, libpcre2-8-0.dll

**Pango/Cairo:**
- libpango-1.0-0.dll, libpangocairo-1.0-0.dll, libpangoft2-1.0-0.dll,
  libpangowin32-1.0-0.dll, libcairo-2.dll, libcairo-gobject-2.dll,
  libfreetype-6.dll, libfontconfig-1.dll, libbz2-1.dll,
  libpng16-16.dll, libtiff-6.dll, libwebp-7.dll, libzstd-1.dll,
  liblzma-5.dll, libjpeg-8.dll, liblz4-1.dll

**Text/Internationalization:**
- libintl-8.dll, libiconv-2.dll, libgraphite2.dll, libharfbuzz-0.dll

**System/Threading:**
- libwinpthread-1.dll, libgcc_s_seh-1.dll, libstdc++-6.dll

**Serial Port:**
- libserialport-0.dll

**Compiler Runtime:**
- Microsoft Visual C++ Redistributable (vcredist) may be needed for
  certain MSYS2 packages. Download from:
  https://aka.ms/vs/17/release/vc_redist.x64.exe

## Design Notes & Caveats

### Three-Tier DLL Discovery

`cmake/copy_dlls.cmake` uses a cascading fallback strategy:

1. **pkg-config** - Queries `pkg-config --variable=prefix gtk+-3.0` for the
   canonical install prefix, then appends `/bin` on Windows.
2. **CMake find-pkg cache** - Falls back to `GTK3_PREFIX` set by
   `pkg_check_modules(GTK3 REQUIRED gtk+-3.0)`.
3. **Hardcoded MSYS2 paths** - Last resort checks for
   `C:\msys64\mingw64\bin` and `C:\msys64\usr\bin`.

### Cache Safety

The `MINGW64_BIN_DIR` variable is NOT stored in the CMake cache to prevent
cache pollution across builds. It is a local variable only.

### MSYS2 Shell Requirements

Some MSYS2 DLLs (particularly those in `/usr/bin`) have their own DLL
dependencies. When running from the MSYS2 shell, the MSYS2 runtime handles
this transparently. When running from CMD/PowerShell, the bundled `dist/`
directory approach (Option A/B) is the most reliable.

### CI Integration

The CI workflow (`build-windows` job) currently builds the project but does
not bundle DLLs or verify runtime. Consider adding a post-build step that
runs `setup_dlls.bat` and verifies the executable launches.
