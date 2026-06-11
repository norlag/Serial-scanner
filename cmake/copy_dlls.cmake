# copy_dlls.cmake -- Post-build step to copy MinGW runtime DLLs next to the executable.
#
# This script is included from CMakeLists.txt only on Windows (MSYS2/MinGW-w64).
# It finds the mingw64/bin directory and copies every required DLL into the
# same folder as the built executable so that the program launches from
# File Explorer / CMD / PowerShell without manual PATH setup.
#
# Design decisions:
#   1. We discover the DLL directory by querying pkg-config for the
#      pkgdatadir of gtk+-3.0, then walking up to find the parent /bin
#      directory.  This is more reliable than hard-coding CMAKE_FIND_ROOT_PATH
#      which may point to a sysroot that doesn't map to a Windows path.
#   2. We use pkg-config to enumerate the DLL dependency list at build time
#      (via pkg-config --libs-only-L and then resolving .dll names) so that
#      new dependencies added by upstream packages are automatically picked up.
#   3. A small batch helper (copy_dlls.bat) is generated at configure time
#      and invoked as a POST_BUILD step.  This avoids CMake string-quoting
#      issues with Windows paths containing spaces.

# ── Step 1: Locate the MinGW-w64 /bin directory ──────────────────────────

# Strategy A: try to find pkg-config and query it
find_program(MINGW_PKGCONFIG NAMES mingw32-pkg-config pkg-config)

if(NOT DEFINED MINGW64_BIN_DIR OR MINGW64_BIN_DIR STREQUAL "")
    set(MINGW64_BIN_DIR "" CACHE PATH "Path to mingw64/bin (optional -- auto-detected by default)")
endif()

if(NOT MINGW64_BIN_DIR)
    if(MINGW_PKGCONFIG)
        # Query the directory of libgtk-3-0.dll via pkg-config
        # pkg-config --variable=pc_path or --libs-only-L gives us library paths
        execute_process(
            COMMAND ${MINGW_PKGCONFIG} --libs-only-L gtk+-3.0
            OUTPUT_VARIABLE _PKGCFG_LIB_DIRS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        # _PKGCFG_LIB_DIRS looks like "-LC:/msys64/mingw64/lib -LC:/msys64/mingw64/lib"
        string(REPLACE " " ";" _PKGCFG_LIB_LIST "${_PKGCFG_LIB_DIRS}")
        foreach(_DIR ${_PKGCFG_LIB_LIST})
            string(REPLACE "-L" "" _DIR "${_DIR}")
            if(EXISTS "${_DIR}/../bin/libgtk-3-0.dll")
                get_filename_component(MINGW64_BIN_DIR "${_DIR}/../bin" ABSOLUTE)
                break()
            endif()
        endforeach()
    endif()
endif()

# Strategy B: fallback -- look at GTK3_LIBRARY_DIRS from pkg_check_modules
if(NOT MINGW64_BIN_DIR)
    if(DEFINED GTK3_LIBRARY_DIRS)
        string(REPLACE " " ";" _FALLBACK_LIB_LIST "${GTK3_LIBRARY_DIRS}")
        foreach(_DIR ${_FALLBACK_LIB_LIST})
            if(EXISTS "${_DIR}/../bin/libgtk-3-0.dll")
                get_filename_component(MINGW64_BIN_DIR "${_DIR}/../bin" ABSOLUTE)
                break()
            endif()
        endforeach()
    endif()
endif()

# Strategy C: last resort -- check common MSYS2 install location
if(NOT MINGW64_BIN_DIR)
    if(EXISTS "C:/msys64/mingw64/bin/libgtk-3-0.dll")
        set(MINGW64_BIN_DIR "C:/msys64/mingw64/bin")
    elseif(EXISTS "D:/msys64/mingw64/bin/libgtk-3-0.dll")
        set(MINGW64_BIN_DIR "D:/msys64/mingw64/bin")
    endif()
endif()

if(NOT MINGW64_BIN_DIR)
    message(WARNING
        "copy_dlls.cmake: Could not locate mingw64/bin directory.\n"
        "  GTK3 DLLs will NOT be copied automatically.\n"
        "  Set MINGW64_BIN_DIR in CMake cache to the path of mingw64/bin.\n"
        "  Or run the build from the MSYS2 MinGW64 shell."
    )
else()
    message(STATUS "copy_dlls.cmake: Found MinGW64 DLL directory: ${MINGW64_BIN_DIR}")
endif()

# ── Step 2: Enumerate required DLLs ─────────────────────────────────────

# We try to discover DLLs from the linker's library list first, then fall
# back to a known-good list for core system DLLs that don't show up in
# pkg-config --libs output.

set(_FOUND_DLLS "")

if(MINGW64_PKGCFG_LIBS)
    # Convert linker flags to DLL names
    # e.g. -lgtk-3 -lgdk-3 -> libgtk-3-0.dll, libgdk-3-0.dll
    foreach(_LIB ${MINGW_PKGCFG_DLL_NAMES})
        # Convert -llibname to libname-0.dll or libname-X.dll
        string(REPLACE "-l" "" _LIBNAME "${_LIB}")
        # Heuristic: most MinGW DLLs are libXXX-0.dll
        set(_DLLNAME "lib${_LIBNAME}-0.dll")
        if(EXISTS "${MINGW64_BIN_DIR}/${_DLLNAME}")
            list(APPEND _FOUND_DLLS "${_DLLNAME}")
        else()
            # Try -1, -2 suffixes
            set(_TRY "")
            foreach(_VER 1 2 3 4 5 6 7 8 9)
                if(EXISTS "${MINGW64_BIN_DIR}/lib${_LIBNAME}-${_VER}.dll")
                    set(_TRY "lib${_LIBNAME}-${_VER}.dll")
                    break()
                endif()
            endforeach()
            if(_TRY)
                list(APPEND _FOUND_DLLS "${_TRY}")
            endif()
        endif()
    endforeach()
endif()

# Always include these core system DLLs (they never appear in pkg-config output)
set(_SYSTEM_DLLS
    libwinpthread-1.dll
    libstdc++-6.dll
    libgcc_s_seh-1.dll
    libgmp-10.dll
    libmpfr-1.dll
    libmpc-1.dll
)

foreach(_SYS_DLL ${_SYSTEM_DLLS})
    if(EXISTS "${MINGW64_BIN_DIR}/${_SYS_DLL}")
        list(APPEND _FOUND_DLLS "${_SYS_DLL}")
    endif()
endforeach()

# Also add libserialport DLL if found
if(EXISTS "${MINGW64_BIN_DIR}/libserialport-0.dll")
    list(APPEND _FOUND_DLLS "libserialport-0.dll")
endif()

# Remove duplicates
list(REMOVE_DUPLICATES _FOUND_DLLS)

list(LENGTH _FOUND_DLLS _DLL_COUNT)
message(STATUS "copy_dlls.cmake: Found ${_DLL_COUNT} DLLs to bundle")

# ── Step 3: Generate the batch copy script ──────────────────────────────

set(_BATCH_FILE "${CMAKE_BINARY_DIR}/copy_dlls.bat")

file(WRITE "${_BATCH_FILE}"
"@echo off\r\n"
"setlocal enabledelayedexpansion\r\n"
"\r\n"
"set \"MINGW_BIN=${MINGW64_BIN_DIR}\"\r\n"
"set \"TARGET_DIR=%~dp0\"\r\n"
"\r\n"
"set /a COPIED=0\r\n"
"\r\n"
)

# Write copy commands for each DLL
foreach(_DLL ${_FOUND_DLLS})
    file(APPEND "${_BATCH_FILE}"
"if exist \"%%MINGW_BIN%%\\${_DLL}\" (\r\n"
"    copy /Y \"%%MINGW_BIN%%\\${_DLL}\" \"%%TARGET_DIR%%\" >nul\r\n"
"    set /a COPIED+=1\r\n"
") else (\r\n"
"    echo WARNING: ${_DLL} not found in %%MINGW_BIN%%\r\n"
")\r\n"
"\r\n"
)
endforeach()

# Write summary line
file(APPEND "${_BATCH_FILE}"
"echo >>> Copying MinGW runtime DLLs...\r\n"
"echo Copied %%COPIED%% DLL(s) to %%TARGET_DIR%%\r\n"
)

# ── Step 4: Hook into the build ─────────────────────────────────────────

# We only add the post-build step if we found at least one DLL
if(_FOUND_DLLS AND MINGW64_BIN_DIR)
    # Build a human-readable list of DLLs for the comment
    list(JOIN _FOUND_DLLS ", " _DLL_LIST)

    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${_BATCH_FILE}"
        COMMENT "Copying ${_DLL_COUNT} MinGW DLLs (${_DLL_LIST})"
        VERBATIM
    )
endif()
