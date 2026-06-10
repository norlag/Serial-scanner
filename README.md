# Serial Scanner

A cross-platform desktop GUI application for discovering and scanning serial/COM ports. Built in pure C11 with GTK3 for the user interface and libserialport for cross-platform serial communication.

## Features

- **Device Discovery**: Automatically detects and lists all available serial ports on the system
- **Smart Auto-Baud Scanning**: Three scanning modes to detect device baudrates:
  - **Mode A (Echo Loopback)**: Sends a character and waits for echo
  - **Mode B (Passive Listen)**: Listens for valid ASCII data streams
  - **Mode C (AT Command)**: Sends AT commands and checks for OK response
- **Real-time Console**: Color-coded log output with [INFO], [TRY], [TIMEOUT], and [SUCCESS] tags
- **Interactive Terminal**: Send manual commands to discovered devices
- **Cross-platform**: Works on Linux and Windows

## Technical Stack

- **Language**: C11
- **GUI Framework**: GTK3
- **Serial Communication**: libserialport (cross-platform)
- **Build System**: CMake
- **Threading**: pthreads for background scanning, g_idle_add() for GTK thread-safe callbacks

## Prerequisites

### Linux

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libgtk-3-dev libserialport-dev

# Fedora
sudo dnf install gcc cmake gtk3-devel libserialport-devel

# Arch
sudo pacman -S base-devel cmake gtk3 libserialport
```

### Windows (MSYS2/MinGW)

1. Install MSYS2 from https://www.msys2.org/
2. Open MSYS2 MinGW64 terminal
3. Install dependencies:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-gtk3 mingw-w64-x86_64-libserialport
   ```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

```bash
./serial-scanner
```

Note: On Linux, you may need to add your user to the `dialout` group to access serial ports:
```bash
sudo usermod -a -G dialout $USER
```

## Architecture

The application follows a modular architecture:

- `src/main.c` - GTK3 UI and application entry point
- `src/device.c` - Device enumeration and port management (libserialport wrapper)
- `src/scanner.c` - Scanner lifecycle and background thread management
- `src/scan_echo.c` - Echo loopback scanning mode
- `src/scan_passive.c` - Passive listening scanning mode
- `src/scan_at.c` - AT command scanning mode

### Threading Model

- **Main Thread**: GTK event loop and UI updates
- **Worker Thread**: Serial port scanning (pthread)
- **Thread Safety**: Background thread uses `g_idle_add()` to safely dispatch log messages to the GTK main thread

## Repository Structure

```
/
├── .github/workflows/ci.yml    # GitHub Actions CI for Linux and Windows
├── CMakeLists.txt              # Cross-platform build configuration
├── README.md                   # This file
├── include/
│   ├── device.h                # Device enumeration API
│   ├── scanner.h               # Scanner lifecycle API
│   └── scan_modes.h            # Scan mode definitions
└── src/
    ├── main.c                  # GTK3 UI and main entry point
    ├── device.c                # Device management implementation
    ├── scanner.c               # Scanner orchestration
    ├── scan_echo.c             # Echo loopback mode
    ├── scan_passive.c          # Passive listening mode
    └── scan_at.c               # AT command mode
```

## License

MIT License
