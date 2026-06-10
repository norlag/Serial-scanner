cmake_minimum_required(VERSION 3.10)
project(SerialScanner LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Source files ---
set(SOURCES
    src/main.c
)

# --- Include directories (hardcoded for this environment) ---
set(GTK_INCLUDE_DIRS
    /tmp/local-install/usr/include/gtk-3.0
    /tmp/local-install/usr/include/pango-1.0
    /tmp/local-install/usr/include/glib-2.0
    /tmp/local-install/usr/lib/x86_64-linux-gnu/glib-2.0/include
    /tmp/local-install/usr/include/harfbuzz
    /usr/include/freetype2
    /tmp/local-install/usr/include/gio-unix-2.0
    /tmp/local-install/usr/include/fribidi
    /tmp/local-install/usr/include/cairo
    /usr/include/pixman-1
    /tmp/local-install/usr/include/gdk-pixbuf-2.0
    /tmp/local-install/usr/include/atk-1.0
    /tmp/local-install/usr/include/at-spi2-atk/2.0
    /tmp/local-install/usr/include/at-spi-2.0
    /usr/include/dbus-1.0
    /usr/lib/x86_64-linux-gnu/dbus-1.0/include
    include
)

# --- Library directories ---
set(GTK_LIB_DIRS
    /output/serial-scanner/liblinks
)

# --- Libraries to link ---
set(GTK_LIBS
    gtk-3 gdk-3 pangocairo-1.0 pango-1.0 harfbuzz atk-1.0
    cairo-gobject cairo gdk_pixbuf-2.0 gio-2.0 gobject-2.0
    glib-2.0 gmodule-2.0 z
)

# --- Executable ---
add_executable(serial-scanner ${SOURCES})

target_include_directories(serial-scanner PRIVATE ${GTK_INCLUDE_DIRS})
target_link_directories(serial-scanner PRIVATE ${GTK_LIB_DIRS})
target_link_libraries(serial-scanner ${GTK_LIBS})

# --- Platform-specific ---
if(WIN32)
    target_link_libraries(serial-scanner ws2_32)
else()
    target_link_libraries(serial-scanner pthread rt m)
    target_compile_options(serial-scanner PRIVATE -Wall -Wextra -pedantic)
endif()