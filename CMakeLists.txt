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

# --- Install ---
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES README.md DESTINATION share/doc/${PROJECT_NAME})
