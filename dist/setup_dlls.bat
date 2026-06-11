@echo off
REM ============================================================================
REM setup_dlls.bat -- Copy MinGW runtime DLLs for Serial-scanner distribution
REM
REM USAGE:
REM   1. Copy serial-scanner.exe and this .bat file to a folder
REM   2. Run this .bat file from the SAME folder
REM   3. The script copies all required DLLs next to serial-scanner.exe
REM   4. After that, serial-scanner.exe can be launched from anywhere
REM      (File Explorer, Start Menu, etc.) without any PATH configuration.
REM
REM PREREQUISITES:
REM   - MSYS2 MinGW-w64 must be installed (default: C:\msys64\mingw64\bin\)
REM   - This batch file must be in the same directory as serial-scanner.exe
REM
REM The DLLs are copied ONCE. Running this script multiple times is safe
REM (files are overwritten in place).
REM ============================================================================

setlocal enabledelayedexpansion

REM Determine where this script lives (same dir as the .exe)
set "SCRIPT_DIR=%~dp0"
set "TARGET_DIR=%SCRIPT_DIR%"

REM Try to find the MSYS2 MinGW bin directory
set "MINGW_BIN="
if exist "C:\msys64\mingw64\bin\libgtk-3-0.dll" (
    set "MINGW_BIN=C:\msys64\mingw64\bin"
) else if exist "D:\msys64\mingw64\bin\libgtk-3-0.dll" (
    set "MINGW_BIN=D:\msys64\mingw64\bin"
) else if exist "%MINGW64_HOME%\mingw64\bin\libgtk-3-0.dll" (
    set "MINGW_BIN=%MINGW64_HOME%\mingw64\bin"
)

if "%MINGW_BIN%"=="" (
    echo ERROR: Cannot find MSYS2 MinGW-w64 installation.
    echo Please set MINGW64_HOME environment variable to point to your MSYS2 install.
    echo Or copy the DLLs manually from C:\msys64\mingw64\bin\
    pause
    exit /b 1
)

echo MSYS2 MinGW bin directory: %MINGW_BIN%
echo Target directory:          %TARGET_DIR%
echo.

REM List of all required DLLs (core GTK3 + transitive deps + system DLLs)
set "DLLS=" ^
libgtk-3-0.dll ^
libgdk-3-0.dll ^
libgdk_pixbuf-2.0-0.dll ^
libgdk-win32-2.0-0.dll ^
libglib-2.0-0.dll ^
libgmodule-2.0-0.dll ^
libgobject-2.0-0.dll ^
libgthread-2.0-0.dll ^
libffi-8.dll ^
libpcre-1.dll ^
libpango-1.0-0.dll ^
libpangocairo-1.0-0.dll ^
libpangoft2-1.0-0.dll ^
libcairo-2.dll ^
libcairo-gobject-2.dll ^
libfreetype-6.dll ^
libpng16-16.dll ^
libharfbuzz-0.dll ^
libfribidi-0.dll ^
libgraphite2-3.dll ^
libpixman-1-0.dll ^
libbz2-1.dll ^
libthai-0.dll ^
libdatrie-1.dll ^
libintl-8.dll ^
libiconv-2.dll ^
libwinpthread-1.dll ^
libstdc++-6.dll ^
libgcc_s_seh-1.dll ^
libserialport-0.dll ^
libgmp-10.dll ^
libmpfr-1.dll ^
libmpc-1.dll

set /a COPIED=0
set /a SKIPPED=0
set /a MISSING=0

for %%D in (%DLLS%) do (
    if exist "%MINGW_BIN%\%%D" (
        copy /Y "%MINGW_BIN%\%%D" "%TARGET_DIR%" >nul
        if !errorlevel! equ 0 (
            set /a COPIED+=1
            echo   [OK] %%D
        ) else (
            echo   [FAIL] %%D (copy error)
            set /a SKIPPED+=1
        )
    ) else (
        echo   [MISS] %%D (not found in %MINGW_BIN%)
        set /a MISSING+=1
    )
)

echo.
echo ============================================================
echo Summary: Copied !COPIED! DLL(s), !SKIPPED! copy error(s), !MISSING! missing
echo ============================================================

if !MISSING! gtr 0 (
    echo.
    echo WARNING: Some DLLs could not be found. The application may not run.
    echo Check that you installed the full MSYS2 MinGW-w64 toolkit:
    echo   pacman -S mingw-w64-x86_64-gtk3 mingw-w64-x86_64-libserialport
    pause
) else (
    echo.
    echo All DLLs copied successfully. You can now run serial-scanner.exe
    echo from File Explorer, Start Menu, or any location.
    pause
)
