@echo off
REM ============================================================================
REM serial-scanner.bat -- Launcher for Serial-scanner on Windows
REM
 USAGE:
   Double-click this file to launch Serial-scanner, or run from CMD.
   This launcher ensures the MSYS2 MinGW bin directory is on PATH
   so GTK3 DLLs are found at runtime.
============================================================================

REM If DLLs have been bundled by the CMake post-build step, prefer them
if exist "%~dp0libgtk-3-0.dll" (
    REM DLLs are bundled -- no PATH manipulation needed
    goto :run
)

REM DLLs are NOT bundled. Try to find MSYS2 MinGW bin directory.
set "MINGW_BIN="
if exist "C:\msys64\mingw64\bin\libgtk-3-0.dll" (
    set "MINGW_BIN=C:\msys64\mingw64\bin"
) else if exist "D:\msys64\mingw64\bin\libgtk-3-0.dll" (
    set "MINGW_BIN=D:\msys64\mingw64\bin"
)

if not "%MINGW_BIN%"=="" (
    set "PATH=%MINGW_BIN%;%PATH%"
)

:run
cd /d "%~dp0"
start "Serial Scanner" /B serial-scanner.exe %*
