---
layout: home
title: Home
nav_order: 1
description: "ZayneScript — a dynamic scripting language written in pure C."
permalink: /
---

# ZayneScript
{: .fs-9 }

A dynamic, interpreted scripting language written in **pure C**. First-class functions, closures, async/await, BigInt, OOP, and a rich built-in standard library — all in a single executable.
{: .fs-6 .fw-300 }

[Language Guide]({% link language.md %}){: .btn .btn-primary .fs-5 .mb-4 .mb-md-0 .mr-2 }
[Standard Library]({% link stdlib.md %}){: .btn .fs-5 .mb-4 .mb-md-0 }

---

## Features

| | Feature | Summary |
|---|---|---|
| ⚡ | **Dynamic Typing** | Variables hold any type at runtime. No annotations required. |
| 🔒 | **First-Class Functions** | Functions are values — pass them, return them, close over variables. |
| 🔄 | **Async / Await** | `async` functions return Promises. Chain with `.then()` / `.error()`. |
| 🔢 | **BigInt** | Arbitrary-precision integers with the `n` suffix — `100n + 2n`. |
| 🏗️ | **Object-Oriented** | Class-based inheritance, constructors (`init`), static members, `this`. |
| 📦 | **Module System** | Named & wildcard imports, `core:` built-ins, `lib:` user libraries, relative paths. |
| 🗃️ | **Arrays & Objects** | Dynamic arrays (`push`, `pop`, `each`, `keep`) and key-value objects with spread syntax. |
| 🎛️ | **Expressive Control Flow** | `for`/`while`/`do-while`, `switch` expressions, `try`/`catch`, ternary forms. |
| 📚 | **Standard Library** | Six built-in modules: `io`, `math`, `os`, `Array`, `Date`, `Promise`. |

---

## Quick Start

### 1 — Build

Requires **GCC** and **GNU Make**.

```bash
# Linux / macOS (recommended)
make

# Optimised release build
make release

# Or compile manually
gcc -O3 -DNDEBUG -Wno-pointer-sign \
    main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c ./libbf/*.c \
    -o zscript.exe -lm -ldl -lpthread
```

Windows (MinGW):

```batch
.\run.bat --compile
```

### 2 — Write a script

```javascript
import { println } from "core:io";

println("Hello, ZayneScript!");
```

### 3 — Run it

```bash
./zscript.exe --run hello.zs
```

---

## A Taste of the Language

```javascript
import { println, format } from "core:io";
import { sqrt, pi }        from "core:math";

// ── Closures ──────────────────────────────────────────────────
fn makeCounter() {
    local count = 0;
    return fn() {
        count += 1;
        return count;
    };
}

var tick = makeCounter();
println(tick(), tick(), tick()); // 1 2 3

// ── Classes with inheritance ───────────────────────────────────
class Shape {
    fn area() { return 0; }
}

class Circle (Shape) {
    fn init(r) { this.r = r; }
    fn area()  { return pi * this.r * this.r; }
}

const c = new Circle(5);
println(format("Area: {}", c.area())); // Area: 78.539816...

// ── Async / Await ─────────────────────────────────────────────
fn fetchData() async { return "payload"; }

fn main() async {
    const data = await fetchData();
    println("Got:", data); // Got: payload
}

main();

// ── BigInt ────────────────────────────────────────────────────
println(2n ** 64n); // 18446744073709551616n
```

---

## Usage

| Command | Description |
|---|---|
| `./zscript.exe --run <file.zs>` | Execute a script |
| `./zscript.exe --tests` | Run all scripts in `tests/` |
| `./zscript.exe --help` | Print help text |

---

## Project Structure

| Path | Contents |
|---|---|
| `src/` | Interpreter core — lexer, parser, compiler, bytecode evaluator |
| `src/core/` | Standard library modules (`io`, `math`, `os`, `array`, `date`, `promise`) |
| `tests/` | Example `.zs` scripts demonstrating every feature |
| `lib/` | User-level library files importable via `lib:` |
| `utf/` | UTF-8 processing helpers (utf8proc) |
| `libbf/` | BigInt backend (libbf) |
| `main.c` | Interpreter entry point |
