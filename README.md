# ZayneScript

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Language: C](https://img.shields.io/badge/Language-C-informational.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](#building)
[![Docs](https://img.shields.io/badge/Docs-online-green.svg)](https://hollishake.github.io/zaynescript)
[![GitHub](https://img.shields.io/badge/GitHub-HolliShake%2Fzaynescript-181717.svg?logo=github)](https://github.com/HolliShake/zaynescript)

```
╔══════════════════════════════════════════════════════════════════════════════════════════╗
║                                                                                          ║
║  ███████╗ █████╗ ██╗   ██╗███╗   ██╗███████╗███████╗ ██████╗██████╗ ██╗██████╗ ████████╗ ║
║  ╚══███╔╝██╔══██╗╚██╗ ██╔╝████╗  ██║██╔════╝██╔════╝██╔════╝██╔══██╗██║██╔══██╗╚══██╔══╝ ║
║    ███╔╝ ███████║ ╚████╔╝ ██╔██╗ ██║█████╗  ███████╗██║     ██████╔╝██║██████╔╝   ██║    ║
║   ███╔╝  ██╔══██║  ╚██╔╝  ██║╚██╗██║██╔══╝  ╚════██║██║     ██╔══██╗██║██╔═══╝    ██║    ║
║  ███████╗██║  ██║   ██║   ██║ ╚████║███████╗███████║╚██████╗██║  ██║██║██║        ██║    ║
║  ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═══╝╚══════╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═╝        ╚═╝    ║
║                                                                                          ║
╚══════════════════════════════════════════════════════════════════════════════════════════╝
```

**ZayneScript** is a custom interpreted scripting language written in pure C. It features dynamic typing, first-class functions with closures, object-oriented programming with classes, and a comprehensive standard library.

## Building

To build ZayneScript, you need a C compiler (GCC or Clang) and GNU Make.

> **Build status quirks**
>
> - The Makefile uses **Clang** by default (`CC := clang`). Swap to `CC=gcc make` if Clang is unavailable.
> - SQLite is compiled as a **separate shared library** (`sqlite/libsqlite3.so` / `sqlite3.dll` on Windows) and must not be compiled into the same translation unit as the rest of the source. The amalgamation script (`amalgamate.py`) reflects this — `sqlite3.c` is excluded from `dist/zscript.c` intentionally.
> - On Linux the ASAN build (`make debug`) requires `libasan`. If linking fails, fall back to `make release`.
> - On Windows (MinGW), `-ldl` and `-lpthread` are **not available** — the `run.bat` script omits them automatically.
> - The `-lm` flag is required on all platforms for math functions.
> - `BUILD_DATE` must be passed as a quoted string macro: `-DBUILD_DATE='"YYYY-MM-DD"'`.

### Linux / macOS — using Make (recommended)

```bash
# Debug build (default)
make

# Release build (optimised, no debug symbols)
make release

# Build and run immediately
make run

# Remove the compiled binary
make clean
```

### Linux / macOS — manual GCC

```bash
gcc -O3 -DNDEBUG -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c ./libbf/*.c -o zscript.exe -lm -ldl -lpthread
```

### Windows (using MinGW/GCC)

Use the provided `run.bat` script. It automatically creates a `win32\` output folder containing `zscript.exe`, `sqlite3.dll`, and a `lib\` subfolder synced from the project's `lib\` directory.

```batch
:: Debug build + run
.\run.bat

:: Debug build only (outputs to win32\)
.\run.bat --compile

:: Optimised release build (outputs to win32\)
.\run.bat --release

:: Build then run the full test suite
.\run.bat --tests
```

Or build manually (note: no `-ldl`/`-lpthread` on Windows):

```bash
:: Build sqlite3.dll first
gcc -shared -O2 -o win32\sqlite3.dll sqlite\sqlite3.c -Wl,--out-implib,sqlite\libsqlite3.a

:: Then build the interpreter
gcc -O3 -DNDEBUG -Wno-pointer-sign main.c src\core\*.c src\*.c utf\*.c utf\utf8proc\*.c libbf\*.c -o win32\zscript.exe -lm -Lsqlite -lsqlite3
```

## Detailed Build Instructions

### Prerequisites

- **Clang** (recommended) or GCC
- **LLD** (LLVM linker)
- **CMake** (for MariaDB Connector/C)
- **GNU Make**
- **nproc** or getconf (for parallel builds)
- **MariaDB Connector/C** and **SQLite3** sources are included in `thirdparty/`
- On Windows: MinGW-w64 toolchain (for cross-compiling)

### Building on Linux / macOS

1. **Install dependencies:**
   - On Arch Linux: `sudo pacman -S clang lld cmake make`
   - On Ubuntu: `sudo apt install clang lld cmake make`
   - On macOS: Use Homebrew for `clang`, `lld`, `cmake`, `make` if not present.

2. **Build (Debug, recommended for development):**
   ```bash
   make
   # or explicitly
   make debug
   ```
   - This builds with AddressSanitizer and leak detection enabled.

3. **Build (Release, optimized):**
   ```bash
   make release
   ```
   - This produces an optimized binary with LTO and no debug symbols.

4. **Architecture selection:**
   - Auto-detect (default): `make`
   - Force 64-bit: `make 64`
   - Force 32-bit x32 ABI: `make 32`
   - Force 32-bit i686: `make 32i686`

5. **Clean build artifacts:**
   ```bash
   make clean
   ```

6. **Install (system-wide):**
   ```bash
   sudo make install
   ```
   - Installs binary to `/usr/local/bin/zscript` and libraries to `/usr/local/lib/zscript/`

7. **Uninstall:**
   ```bash
   sudo make uninstall
   ```

8. **Amalgamate (single-file source):**
   ```bash
   make amalgamate
   ```
   - Bundles all sources into one file via `amalgamate.py`.

#### Troubleshooting
- If you see errors about missing `clang`, `lld`, or `cmake`, install them as above.
- If linking fails with AddressSanitizer, try `make release`.
- If you see errors about missing `libatomic`, install your system's atomic library (e.g., `libatomic-ops-dev`).
- If you want to use GCC instead of Clang: `make CC=gcc`.

### Building on Windows (MinGW-w64)

1. **Install MinGW-w64 toolchain** (e.g., `mingw-w64-gcc`, `mingw-w64-cmake`).
2. **Build for Windows:**
   ```bash
   make win32-64    # 64-bit
   make win32-32    # 32-bit
   make win32-32i686 # 32-bit i686
   ```
   - Output is in `win32/<arch>/zscript.exe` with required DLLs.
   - The Makefile will build `sqlite3.dll` and `libmariadb.dll` as needed.

3. **Copy assets:**
   - The Makefile automatically copies `lib/` and `tests/` into the output directory.

4. **Manual build (not recommended):**
   - See the Makefile for the exact flags and required libraries.

#### Windows Notes
- No `-ldl` or `-lpthread` on Windows.
- Use the provided `run.bat` for simple builds and tests.
- If you see errors about missing DLLs, ensure `sqlite3.dll` and `libmariadb.dll` are present in the output directory.

### Cross-compiling for Windows from Linux
- Install MinGW-w64 and required CMake toolchain files.
- Use the `make win32-64` or similar targets as above.
- The Makefile will attempt to find the correct toolchain and build all dependencies.

---

For more details, see the Makefile and the documentation in `docs/`.

## Usage

Once built, you can run the interpreter using the executable.

### Run a Script

```bash
./zscript --run <file.zs>
```

Example:
```bash
./zscript --run tests/test_closure.zs
```

### Run Tests

To run all tests in the `tests/` directory:

```bash
# Linux / macOS
./zscript.exe --run tests/test_all.zs

# Windows (after run.bat --compile or run.bat --release)
.\run.bat --tests
```

### Help

```bash
./zscript --help
```

## Language Guide

For the full language reference — syntax, types, control flow, classes, async/await, modules, and the standard library — see the documentation:

**[hollishake.github.io/zaynescript](https://hollishake.github.io/zaynescript)**

## Project Structure

-   `src/`: Core source code (lexer, parser, compiler, interpreter).
-   `src/core/`: Standard library implementation (`io`, `math`, `os`, `array`, `date`).
-   `tests/`: Test scripts (`.zs` files) demonstrating language features.
-   `utf/`: UTF-8 processing libraries.
-   `main.c`: Entry point.

## License

MIT License

## Author

Philipp Andrew Redondo © 2025-Present. All rights reserved.

