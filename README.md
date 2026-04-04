# ZayneScript

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Language: C](https://img.shields.io/badge/Language-C-informational.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](#building)
[![Docs](https://img.shields.io/badge/Docs-online-green.svg)](https://hollishake.github.io/zaynescript)

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

## Features

-   **Dynamic Typing**: Variables can hold values of any type.
-   **Functions**: First-class functions, closures, recursion, and higher-order functions.
-   **Async Functions**: Functions can be declared `async` to run asynchronously.
-   **BigInt Support**: Arbitrary-precision integers using the `n` suffix (e.g., `100n`).
-   **Object-Oriented**:
    -   Class-based inheritance.
    -   Static methods and static properties.
    -   Constructors (`init`).
    -   `this` keyword for instance access.
-   **Data Structures**:
    -   **Arrays**: Dynamic arrays with built-in methods (`push`, `pop`, `length`, `each`, `keep`).
    -   **Objects**: Key-value pairs with dot and bracket notation, object shorthand, and spread syntax.
-   **Control Flow**:
    -   `if`/`else` conditionals with optional initializer (`:=`).
    -   `for`, `while` (with optional initializer), and `do-while` loops.
    -   `break` and `continue` statements.
    -   `switch` statements and expression-position `switch`.
    -   `try`/`catch` blocks for error handling.
    -   Ternary expressions: `cond ? a : b` and postfix `value if (cond) else alt`.
-   **Operators**:
    -   Arithmetic: `+`, `-`, `*`, `/`, `%`
    -   Bitwise: `&`, `|`, `^`, `<<`, `>>`
    -   Logical: `&&`, `||`, `!`
    -   Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
    -   Augmented assignment: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`
    -   Increment / Decrement: `++`, `--` (prefix and postfix)
    -   Spread: `...` in array and object literals
-   **Standard Library**:
    -   `core:io`: Input/Output (`print`, `println`, `scan`, `parseNum`, `format`, `clearScreen`, `setColor`).
    -   `core:math`: Math library (`sin`, `cos`, `pow`, `sqrt`, etc.).
    -   `core:os`: OS utilities (`getCwd`, `getPid`, `getUser`, `getType`, `system`).
    -   `core:Array`: Array class with static helpers.
    -   `core:Date`: Date and time class.
    -   `core:Promise`: Promise class for manual async composition.
    -   `core:sqlite`: SQLite3 bindings (`Database`, `Statement`).
-   **Modules**: Named and wildcard imports, `lib:` user libraries, and relative file imports.

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

