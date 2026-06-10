# Serial Scanner

A cross-platform (Windows & Linux) desktop GUI application for scanning, identifying, and interacting with serial devices.

Built with **Pure C (C11)** and **GTK3**.

## Features
- Real-time serial device discovery (port name, manufacturer, description)
- Smart auto-baudrate detection with three modes:
  - **Mode A**: Echo Loopback — transmits a character and waits for echo
  - **Mode B**: Passive Listen — listens for ASCII data streams
  - **Mode C**: AT Command — sends "AT\r\n" and checks for "OK" response
- Interactive terminal mode for direct device communication
- Color-coded real-time console log

## Build Requirements
- **Linux:** GTK3 dev libraries, cmake, gcc, pthreads
- **Windows:** MSYS2/MinGW with GTK3, cmake

## Building
```bash
mkdir build && cd build
cmake ..
make
```

## License
MIT
