# ZayneScript

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
-   **Object-Oriented**:
    -   Class-based inheritance.
    -   Static and instance members.
    -   Constructors (`init`).
    -   `this` keyword for instance access.
-   **Data Structures**:
    -   **Arrays**: Dynamic arrays with built-in methods like `push` and `length`.
    -   **Objects**: Key-value pairs (HashMaps) with dot and bracket notation access.
-   **Control Flow**:
    -   `if`, `else` conditionals.
    -   `for`, `while`, and `do-while` loops.
    -   `break` and `continue` statements.
    -   `try-catch` blocks for error handling.
-   **Standard Library**:
    -   `core:io`: Input/Output operations (`print`, `println`, `scan`).
    -   `core:math`: Comprehensive math library (`sin`, `cos`, `pow`, `sqrt`, etc.).
-   **Modules**: Import system for code organization.

## Building

To build ZayneScript, you need a C compiler (like GCC).

### Windows (using MinGW/GCC)

You can use the provided `run.bat` script:

```batch
.\run.bat --compile
```

Or run the GCC command manually:

```bash
gcc -O3 -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c -o zscript.exe -lm
```

### Linux / macOS

```bash
gcc -O3 -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c -o zscript -lm -ldl -lpthread
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
./zscript --tests
```

### Help

```bash
./zscript --help
```

## Language Guide

### Variables

-   `var`: Global mutable variables.
-   `const`: Global immutable constants.
-   `local`: Function or block-scoped mutable variables.

```javascript
var globalX = 10;
const PI = 3.14159;

fn example() {
    local x = 20;
    if (true) {
        local y = 30; // Block scoped
    }
}
```

### Functions & Closures

Functions are first-class citizens and support closures.

```javascript
fn makeAdder(x) {
    return fn(y) {
        return x + y;
    };
}

const add5 = makeAdder(5);
println(add5(10)); // Outputs: 15
```

### Classes

Classes support inheritance and static members.

```javascript
class Animal {
    fn speak() {
        println("Generic animal sound");
    }
}

class Dog (Animal) {
    fn init(name) {
        this.name = name;
    }

    fn speak() {
        println(this.name, "says Woof!");
    }
}

const d = new Dog("Buddy");
d.speak(); // Outputs: Buddy says Woof!
```

### Arrays & Objects

```javascript
// Arrays
local list = [1, 2, 3];
list.push(4);
println(list[0]); // 1

// Objects
local person = {
    name: "Alice",
    age: 30
};
println(person.name);
```

### Standard Library

Import modules using the `import` statement.

```javascript
import { println, scan } from "core:io";
import "core:math"; // Imports as 'math' object

println("Hello World");
println(math.sqrt(16)); // 4
```

## Project Structure

-   `src/`: Core source code (lexer, parser, compiler, interpreter).
-   `src/core/`: Standard library implementation (`io`, `math`, `array`).
-   `tests/`: Test scripts (`.zs` files) demonstrating language features.
-   `utf/`: UTF-8 processing libraries.
-   `main.c`: Entry point.

## License

MIT License

## Author

Philipp Andrew Redondo
