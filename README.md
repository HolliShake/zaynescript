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
-   **Modules**: Named and wildcard imports for code organization.

## Building

To build ZayneScript, you need a C compiler (GCC) and GNU Make.

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

You can use the provided `run.bat` script:

```batch
.\run.bat --compile
```

Or run the GCC command manually:

```bash
gcc -O3 -DNDEBUG -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c ./libbf/*.c -o zscript.exe -lm
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

-   `var`: Global mutable variable.
-   `const`: Global immutable constant.
-   `local`: Block/function-scoped mutable variable.

Multiple declarations can be made in a single statement, separated by commas.

```javascript
var globalX = 10;
const PI = 3.14159;
const a = 1, b = 2, c = 3; // Multiple declarations

fn example() {
    local x = 20;
    local f = 6, g = 7; // Multiple locals
    if (true) {
        local y = 30; // Block scoped — not visible outside this block
    }
}
```

### Functions & Closures

Functions are declared with `fn`. They are first-class values and support closures.

```javascript
fn greet(name) {
    println("Hello", name);
}

fn makeAdder(x) {
    return fn(y) {
        return x + y;
    };
}

const add5 = makeAdder(5);
println(add5(10)); // 15
```

#### Async Functions

The `async` keyword is placed **after** the parameter list. Calling an async function returns a **Promise** immediately; use `await` inside another async function to wait for the result.

```javascript
fn fetchData() async {
    return "result";
}

// Anonymous async function expression
const task = fn() async {
    return "done";
};
```

Use `await` to suspend the current async function until the awaited promise resolves:

```javascript
fn topLevel() async {
    return "Hello";
}

fn callMe() async {
    println(await topLevel()); // Hello
    println(await topLevel()); // Hello
    return 1;
}

println(callMe()); // <Promise>
```

#### Promise Chaining (`.then`)

Promises expose a `.then(callback)` method for chaining reactions without `await`. Each `.then` receives the resolved value of the previous step and its return value becomes the next promise in the chain:

```javascript
fn awaitable() async {
    return "Hola!";
}

const v = awaitable()
    .then(fn(v) {
        println("resolved with:", v); // resolved with: Hola!
        return 42;
    })
    .then(fn(v) {
        println("chained value:", v); // chained value: 42
        return "done";
    })
    .then(println); // done

println(v); // <Promise>
```

### Operators

#### Ternary Expressions

Two forms are supported:

```javascript
// Standard C-style ternary
local result = condition ? "yes" : "no";

// Postfix if/else form
local result = "yes" if (condition) else "no";
```

#### Spread Operator

Use `...` to spread arrays or objects:

```javascript
local a = [1, 2, 3];
local b = [...a, 4, 5];       // [1, 2, 3, 4, 5]

local obj1 = { x: 1 };
local obj2 = { ...obj1, y: 2 }; // { x: 1, y: 2 }
```

#### BigInt Literals

Append `n` to a number literal to create a BigInt:

```javascript
println(100n + 2n);    // 102n
println(5n * 5n);      // 25n
println(5n / 2n);      // 2n  (integer division)
println(1 << 2n);      // 4
println(10.56n);       // BigInt from float literal
println(2.2e2n);       // BigInt from scientific notation
```

### Control Flow

#### `if` / `else`

```javascript
if (x > 0) {
    println("positive");
} else {
    println("non-positive");
}
```

The `if` condition also supports an optional initializer using `:=` (short assign), separated by `;`:

```javascript
if (val := computeValue(); val > 0) {
    println("got", val);
}
```

#### `for` Loop

The `for` loop uses `:=` for its initializer (the variable is scoped to the loop):

```javascript
for (i := 0; i < 10; i++) {
    println(i);
}

// Augmented assignment step
for (x := 0.0; x < 1.0; x += 0.1) {
    println(x);
}
```

#### `while` Loop

Standard while:

```javascript
while (condition) {
    // ...
}
```

While with an initializer (variable scoped to the loop):

```javascript
while (i := 0; i < 10) {
    println(i);
    i++;
}
```

While with an initializer, condition, and mutator:

```javascript
while (i := 0; i < 10; i++) {
    println(i);
}
```

#### `do-while` Loop

```javascript
var n = 0;
do {
    println(n++);
} while (n < 5)
```

#### `try` / `catch`

Both the `try` and `catch` bodies must be block statements `{ }`:

```javascript
try {
    local x = riskyOperation();
} catch (e) {
    println("Error:", e);
}
```

### Classes

Classes support single inheritance. The superclass is specified in parentheses after the class name. The constructor is named `init`.

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
d.speak(); // Buddy says Woof!
```

#### Static Members

Use `static` before `fn` for static methods, or `static name = value;` for static properties:

```javascript
class MathUtils {
    static PI = 3.14159;

    static fn square(x) {
        return x * x;
    }
}

println(MathUtils.PI);          // 3.14159
println(MathUtils.square(4));   // 16
```

### Arrays & Objects

```javascript
// Arrays
local list = [1, 2, 3];
list.push(4);
println(list[0]);        // 1
println(list.length());  // 4
list.pop();

// Spread in array
local more = [...list, 5, 6];

// Objects
local person = {
    name: "Alice",
    age: 30
};
println(person.name);      // Alice
println(person["age"]);    // 30

// Shorthand: if key name matches variable, value can be omitted
local x = 10;
local obj = { x };  // equivalent to { x: x }

// Spread in object
local extended = { ...person, city: "NYC" };
```

### Modules & Imports

```javascript
// Named imports
import { println, scan, parseNum } from "core:io";

// Wildcard import — module is bound to its name
import "core:math"; // available as `math`

// Named imports from other modules
import { getUser, getCwd } from "core:os";
import { Array } from "core:Array";
import "core:Date"; // available as `Date`
```

### Standard Library

#### `core:io` (Input/Output)

```javascript
import { print, println, scan, parseNum, format, clearScreen, setColor } from "core:io";

print("Enter your name: ");
local name = scan();
println("Hello", name);
println(format("Value: {}", 42));
```

-   **`print(...args)`**: Prints arguments to stdout without a trailing newline.
-   **`println(...args)`**: Prints arguments followed by a newline.
-   **`scan(prompt?)`**: Reads a line from stdin, optionally printing a prompt.
-   **`parseNum(str)`**: Parses a string into a number.
-   **`format(fmt, ...args)`**: Returns a formatted string (uses `{}` placeholders).
-   **`clearScreen()`**: Clears the terminal screen.
-   **`setColor(...args)`**: Sets the terminal output color.

#### `core:math` (Mathematics)

```javascript
import "core:math";

println(math.sqrt(16));  // 4
println(math.pi);        // 3.1415926535...
println(math.pow(2, 8)); // 256
```

-   **Single-arg functions**: `abs`, `acos`, `asin`, `atan`, `ceil`, `cos`, `cosh`, `exp`, `floor`, `log`, `log10`, `round`, `sin`, `sinh`, `sqrt`, `tan`, `tanh`.
-   **Two-arg functions**: `atan2`, `hypot`, `max`, `min`, `pow`.
-   **Constants**: `pi`, `e`.

#### `core:os` (Operating System)

```javascript
import { getCwd, getPid, getUser, getType, system } from "core:os";

println(getCwd());   // current working directory
println(getPid());   // process ID
println(getUser());  // current username
println(getType());  // OS type string
system("ls -la");    // execute a shell command
```

#### `core:Array`

```javascript
import { Array } from "core:Array";

local nums = [1, 2, 3, 4, 5];

// each(callback(index, element)) — map
local doubled = nums.each(fn(i, e) { return e * 2; });

// keep(callback(index, element)) — filter
local odds = nums.keep(fn(i, e) { return e % 2 != 0; });

println(nums.length()); // 5
nums.push(6);
nums.pop();

// Static method added at runtime
Array.sum = fn(arr) {
    local total = 0;
    arr.each(fn(i, e) { total += e; });
    return total;
};
println(Array.sum(nums)); // 15
```

#### `core:Date`

```javascript
import "core:Date";

// Current date/time
const now = new Date();
println(now.toString());

// From a date string
const d1 = new Date("2023-12-25");
println(d1.getFullYear()); // 2023
println(d1.getMonth());    // 11
println(d1.getDate());     // 25

// From components: year, month (0-based), day[, hours, minutes, seconds]
const d2 = new Date(2024, 0, 1, 12, 30, 45);
println(d2.getHours());   // 12
println(d2.getMinutes()); // 30
println(d2.getSeconds()); // 45

// From a Unix timestamp (milliseconds)
const d3 = new Date(1705449600000);
println(d3.getTime());
```

-   **Instance methods**: `getFullYear()`, `getMonth()`, `getDate()`, `getDay()`, `getHours()`, `getMinutes()`, `getSeconds()`, `getTime()`, `toString()`.

## Project Structure

-   `src/`: Core source code (lexer, parser, compiler, interpreter).
-   `src/core/`: Standard library implementation (`io`, `math`, `os`, `array`, `date`).
-   `tests/`: Test scripts (`.zs` files) demonstrating language features.
-   `utf/`: UTF-8 processing libraries.
-   `main.c`: Entry point.

## License

MIT License

## Author

Philipp Andrew Redondo
