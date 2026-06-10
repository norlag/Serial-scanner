# Architecture Decisions

## Technology Stack
- **Language:** C11
- **GUI:** GTK3
- **Build:** CMake 3.10+
- **Serial (Linux):** termios API via POSIX
- **Serial (Windows):** Win32 API (CreateFile, DCB, COMMTIMEOUTS)
- **Threading:** GLib (g_thread_new, g_main_context_invoke)

## Project Structure
```
/output/serial-scanner/
├── .github/workflows/ci.yml
├── src/               # C source files
├── include/           # C header files
├── CMakeLists.txt     # CMake build configuration
├── ARCHITECTURE.md    # This file
├── README.md          # Project documentation
└── liblinks/          # Build helper: .so symlinks for local environment
```

## Milestone Status
- [x] Milestone 1: Environment & Base UI Setup — GTK3 window builds and links
- [ ] Milestone 2: Device Discovery & GTK UI Layout
- [ ] Milestone 3: Async Auto-Baudrate Engine & Logging Panel
- [ ] Milestone 4: Interactive Terminal Mode & Final Polish

## Key Decisions
1. **Hardcoded include paths in CMakeLists.txt** — The CI environment and local build environment use different include paths. CMakeLists.txt uses hardcoded paths that work in this local build env; CI environments install proper -dev packages and will need adjustment.
2. **Native serial APIs** — Instead of libserialport, uses conditional compilation for platform-specific serial code.
3. **GLib threading** — g_thread_new for portability; g_main_context_invoke for GTK main thread synchronization.
