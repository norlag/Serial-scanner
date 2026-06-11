cmake_minimum_required(VERSION 3.16)
project(serial-scanner VERSION 0.4.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# --- Find GTK3 ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(LIBSERIALPORT REQUIRED libserialport)

# --- Sources ---
set(SOURCES
    src/main.c
    src/device.c
    src/scanner.c
    src/scan_echo.c
    src/scan_passive.c
    src/scan_at.c
)

set(HEADERS
    include/device.h
    include/scanner.h
    include/scan_modes.h
)

# --- Executable ---
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${GTK3_INCLUDE_DIRS}
    ${LIBSERIALPORT_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)
target_link_directories(${PROJECT_NAME} PRIVATE
    ${GTK3_LIBRARY_DIRS}
    ${LIBSERIALPORT_LIBRARY_DIRS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GTK3_LIBRARIES}
    ${LIBSERIALPORT_LIBRARIES}
    pthread
)
target_compile_options(${PROJECT_NAME} PRIVATE
    ${GTK3_CFLAGS_OTHER}
    ${LIBSERIALPORT_CFLAGS_OTHER}
    -Wall -Wextra -Wpedantic
)

# --- Windows DLL bundling (post-build step) ─────────────────────────────
# On MSYS2/MinGW-w64, GTK3 and its ~30 transitive dependencies are installed
# into C:\msys64\mingw64\bin\.  When the .exe is launched from File Explorer,
# CMD, or PowerShell, the Windows PE loader cannot find these DLLs because
# that directory is not in the standard Windows PATH.
#
# This post-build step copies every required DLL next to the .exe so that
# the application runs standalone without manual PATH configuration.
#
# For development: you can skip this by passing -DWINDOWS_BUNDLE_DLLS=OFF.
# For distribution: this step is enabled by default on Windows.
option(WINDOWS_BUNDLE_DLLS "Copy MinGW runtime DLLs next to the executable on Windows" ON)

if(WINDOWS_BUNDLE_DLLS AND WIN32)
    include(cmake/copy_dlls.cmake)
endif()

# --- Install ---
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES README.md DESTINATION share/doc/${PROJECT_NAME})

# --- Windows DLL Bundling ---
# Include the post-build DLL bundling script on Windows/MinGW builds.
# This copies all required GTK3 and transitive dependency DLLs next to the
# executable so it can be launched from File Explorer or a regular CMD window.
# Set WINDOWS_BUNDLE_DLLS=OFF to disable.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_dlls.cmake")
