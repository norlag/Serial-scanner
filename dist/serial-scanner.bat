@echo off
REM serial-scanner.bat - Launcher for Serial-scanner on Windows
REM
REM This script launches serial-scanner.exe with the correct DLL search path.
REM It checks for a bundled "dist" directory first, then falls back to
REM checking the MSYS2/MinGW-w64 installation directories.
REM
REM Usage:
REM   serial-scanner.bat [ARGS...]
REM
REM Pass any command-line arguments to serial-scanner.exe after the script name.
REM
REM Example:
REM   serial-scanner.bat --verbose --port COM3

setlocal enabledelayedexpansion

REM Determine the directory containing this batch file
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Locate the executable
set "EXE_PATH=%SCRIPT_DIR%\serial-scanner.exe"
if not exist "%EXE_PATH%" (
    REM Try build directory
    set "EXE_PATH=%SCRIPT_DIR%\build\serial-scanner.exe"
)
if not exist "%EXE_PATH%" (
    echo [ERROR] serial-scanner.exe not found.
    echo [ERROR] Please run setup_dlls.bat first to bundle the required DLLs.
    echo.
    echo Or build from source using CMake:
    echo   mkdir build
    echo   cd build
    echo   cmake -G "MinGW Makefiles" ..
    echo   cmake --build .
    pause
    exit /b 1
)

REM Build DLL search path
REM Priority: bundled dist/ > MSYS2 bin directories > system PATH

set "DLL_PATH_PARTS="

REM 1. Bundled dist directory (created by setup_dlls.bat or CMake post-build)
if exist "%SCRIPT_DIR%\dist" (
    set "DLL_PATH_PARTS=%SCRIPT_DIR%\dist"
    echo [INFO] Using bundled DLLs from dist/
)

REM 2. MSYS2 MinGW64 bin directory
if exist "C:\msys64\mingw64\bin" (
    if defined DLL_PATH_PARTS (
        set "DLL_PATH_PARTS=!DLL_PATH_PARTS!;C:\msys64\mingw64\bin"
    ) else (
        set "DLL_PATH_PARTS=C:\msys64\mingw64\bin"
        echo [INFO] Using MSYS2 DLLs from C:\msys64\mingw64\bin
    )
)

REM 3. MSYS2 /usr/bin
if exist "C:\msys64\usr\bin" (
    if defined DLL_PATH_PARTS (
        set "DLL_PATH_PARTS=!DLL_PATH_PARTS!;C:\msys64\usr\bin"
    ) else (
        set "DLL_PATH_PARTS=C:\msys64\usr\bin"
        echo [INFO] Using MSYS2 DLLs from C:\msys64\usr\bin
    )
)

REM 4. MINGW_PREFIX environment variable
if defined MINGW_PREFIX (
    if exist "%MINGW_PREFIX%\bin" (
        if defined DLL_PATH_PARTS (
            set "DLL_PATH_PARTS=!DLL_PATH_PARTS!;%MINGW_PREFIX%\bin"
        ) else (
            set "DLL_PATH_PARTS=%MINGW_PREFIX%\bin"
            echo [INFO] Using MSYS2 DLLs from %MINGW_PREFIX%\bin
        )
    )
)

REM Launch the executable with the correct PATH
if defined DLL_PATH_PARTS (
    set "PATH=%DLL_PATH_PARTS%;%PATH%"
)

echo [INFO] Launching Serial-Scanner...
echo.

REM Execute the binary with any passed arguments
"%EXE_PATH%" %*

echo.
echo [INFO] Serial-Scanner exited with code %errorlevel%
pause
