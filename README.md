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

ZayneScript comes with a comprehensive standard library. Import modules using the `import` statement.

#### `core:io` (Input/Output)

Handles basic console input and output operations.

```javascript
import { print, println, scan, parseNum, format } from "core:io";

print("Enter your age: ");
local ageStr = scan();
local age = parseNum(ageStr);

println(format("You are %d years old.", age));
```

-   **`print(...args)`**: Prints the provided arguments to standard output.
-   **`println(...args)`**: Prints arguments followed by a newline.
-   **`scan(prompt)`**: Reads a string from standard input, optionally displaying a prompt.
-   **`parseNum(str)`**: Parses a string into a numeric value.
-   **`format(fmt, ...args)`**: Formats an interpolated string.

#### `core:math` (Mathematics)

Provides mathematical functions and constants.

```javascript
import "core:math"; // Imports as 'math' object

println(math.sqrt(16)); // 4
println(math.pi);       // 3.1415926535...
```

-   **Functions**: `abs`, `acos`, `asin`, `atan`, `atan2`, `ceil`, `cos`, `cosh`, `exp`, `floor`, `hypot`, `log`, `log10`, `max`, `min`, `pow`, `round`, `sin`, `sinh`, `sqrt`, `tan`, `tanh`, `trunc`.
-   **Constants**: `pi`, `e`.

#### `Date` Module

The `Date` module provides a class for working with dates and times.

```javascript
import { Date } from "Date";

local now = new Date();
println("Current Year: ", now.getFullYear());
println(now.toString());
```

-   **Constructor methods**: `new Date(year, month, day, hours, minutes, seconds)`.
-   **Instance methods**: `getFullYear()`, `getMonth()`, `getDate()`, `getDay()`, `getHours()`, `getMinutes()`, `getSeconds()`, `getTime()`, `toString()`.

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
