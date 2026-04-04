---
layout: default
title: Language Guide
nav_order: 2
has_toc: true
---

# Language Guide
{: .no_toc }

Complete reference for ZayneScript syntax, semantics, and built-in behaviour — derived from the interpreter source and test suite.
{: .fs-5 .fw-300 }

<details open markdown="block">
  <summary>Table of contents</summary>
  {: .text-delta }
- TOC
{:toc}
</details>

---

## Overview

ZayneScript is a **dynamically-typed**, interpreted scripting language compiled to an internal bytecode format and evaluated by a C runtime. Its syntax is close to JavaScript / C with a few intentional differences:

- `async` is placed **after** the parameter list, not before `fn`.
- Block-scoped variables use `local` instead of `let`.
- `for` / `while` initialisers use `:=` (short assign), not a keyword declaration.
- Switch expressions are written in **value position** using `=>`.

---

## Data Types

| Type | Examples | Notes |
|---|---|---|
| `null` | `null` | Absence of a value |
| `bool` | `true`, `false` | |
| `int` | `0`, `-1`, `42` | Machine integer |
| `num` | `3.14`, `1e-9` | 64-bit double |
| `bigint` | `100n`, `2n**64n` | Arbitrary precision (libbf) |
| `str` | `"hello"`, `'world'` | UTF-8 string |
| `array` | `[1, 2, 3]` | Dynamic, heterogeneous |
| `object` | `{ x: 1 }` | Key-value map |
| `function` | `fn() {}` | First-class, closures |
| `class` | `class Foo {}` | Class descriptor |
| `promise` | returned by `async` calls | Async future value |

---

## Variables

Three declaration keywords with distinct scoping rules:

| Keyword | Scope | Mutable |
|---|---|---|
| `var` | Global (module level) | Yes |
| `const` | Global (module level) | No |
| `local` | Block / function | Yes |

Multiple declarations can be combined with commas:

```javascript
var globalX = 10;
const PI = 3.14159;
const a = 1, b = 2, c = 3;   // multiple consts

fn example() {
    local x = 20;
    local f = 6, g = 7;       // multiple locals

    if (true) {
        local y = 30;         // block-scoped; invisible outside the if
    }
}
```

{: .note }
`local` variables declared inside `if`, `for`, `while`, and other blocks are restricted to that block's scope.

---

## Operators

| Category | Operators |
|---|---|
| Arithmetic | `+` `-` `*` `/` `%` |
| Bitwise | `&` `\|` `^` `<<` `>>` |
| Logical | `&&` `\|\|` `!` |
| Comparison | `==` `!=` `<` `<=` `>` `>=` |
| Augmented assignment | `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` |
| Increment / Decrement | `++` `--` (prefix and postfix) |
| Spread | `...` (array and object literals) |
| Exponentiation (BigInt) | `**` |

---

## Ternary Expressions

Two syntactic forms are supported:

```javascript
// Standard C-style ternary
local result = condition ? "yes" : "no";

// Postfix if/else form
local result = "yes" if (condition) else "no";
```

---

## Spread Operator

Use `...` to expand an array or object in-place:

```javascript
local a = [1, 2, 3];
local b = [...a, 4, 5];         // [1, 2, 3, 4, 5]

local obj1 = { x: 1 };
local obj2 = { ...obj1, y: 2 }; // { x: 1, y: 2 }
```

---

## BigInt Literals

Append `n` to any numeric literal to create an arbitrary-precision BigInt:

```javascript
println(100n + 2n);   // 102n
println(5n * 5n);     // 25n
println(5n / 2n);     // 2n  (integer division)
println(1 << 2n);     // 4
println(10.56n);      // BigInt from float literal
println(2.2e2n);      // BigInt from scientific notation
println(2n ** 64n);   // 18446744073709551616n
```

{: .note }
Mixed BigInt / non-BigInt arithmetic is supported in shift operators; other mixed operations follow coercion rules.

---

## Functions & Closures

Functions are declared with `fn`. They are first-class values and can be assigned, passed as arguments, and returned.

```javascript
fn greet(name) {
    println("Hello", name);
}

// Anonymous function expression
const square = fn(x) { return x * x; };

// Higher-order function
fn apply(f, x) { return f(x); }
println(apply(square, 5)); // 25
```

### Closures

Inner functions capture variables from their enclosing scope and can mutate them:

```javascript
fn counter() {
    local count = 0;
    return fn() {
        count += 1;
        return count;
    };
}

var c = counter();
println(c()); // 1
println(c()); // 2
println(c()); // 3
```

### Multiple closures sharing state

```javascript
fn makeCounters() {
    local shared = 0;
    local inc = fn() { shared += 1; };
    local dec = fn() { shared -= 1; };
    local get = fn() { return shared; };
    return [inc, dec, get];
}

var counters = makeCounters();
var inc = counters[0];
var dec = counters[1];
var get = counters[2];

inc(); inc();
println(get()); // 2
dec();
println(get()); // 1
```

---

## Async / Await

The `async` keyword is placed **after** the parameter list. Calling an async function returns a **Promise** immediately. Use `await` inside another async function to suspend until the awaited promise resolves.

```javascript
fn fetchData() async {
    return "payload";
}

fn main() async {
    const data = await fetchData();
    println("Got:", data); // Got: payload
    return 1;
}

println(main()); // <Promise> — non-blocking
```

### Anonymous async functions

```javascript
const task = fn() async {
    return "done";
};
```

### Nested await

```javascript
fn topLevel() async { return "Hello"; }

fn callMe() async {
    println(await topLevel()); // Hello
    println(await topLevel()); // Hello
    return 1;
}

println(callMe()); // <Promise>
```

---

## Promises

Every `async` function returns a Promise. The `.then(callback)` method chains a reaction to the resolved value; its return value becomes the next promise in the chain. `.error(callback)` handles rejections.

```javascript
fn awaitable() async { return "Hola!"; }

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

### Error handling with `.error()`

```javascript
fn risky() async {
    return someUndefinedOp();
}

risky()
    .then(fn(v) { println("success:", v); })
    .error(fn(e) { println("caught:", e); });
```

---

## if / else

```javascript
if (x > 0) {
    println("positive");
} else {
    println("non-positive");
}
```

### Optional initialiser (`:=`)

The condition supports an optional initialiser separated by `;`. The variable is scoped to the `if` block:

```javascript
if (val := computeValue(); val > 0) {
    println("got", val);
}
```

---

## for Loop

The initialiser uses `:=`; the loop variable is scoped to the loop:

```javascript
for (i := 0; i < 10; i++) {
    println(i);
}

// Floating-point step
for (x := 0.0; x < 1.0; x += 0.1) {
    println(x);
}
```

---

## while Loop

```javascript
// Standard while
while (condition) {
    // ...
}

// With initialiser — variable scoped to loop
while (i := 0; i < 10) {
    println(i);
    i++;
}

// With initialiser + mutator (three-part form)
while (i := 0; i < 10; i++) {
    println(i);
}
```

---

## do-while Loop

```javascript
var n = 0;
do {
    println(n++);
} while (n < 5);
```

---

## switch

Two forms are available: **statement** and **expression**.

### Statement form

Uses `case` with `:` and block bodies. Comma-separated values match multiple literals in one case:

```javascript
switch (num) {
    case 0, 1: {
        println("matched 0 or 1");
    }
    case 2, 3: {
        println("matched 2 or 3");
    }
    default: {
        println("no match");
    }
}
```

### Expression form

Placed after the value being tested; uses `=>` and produces a value. No `break` needed:

```javascript
const label = 100 switch {
    case 10          => "Ten"
    case 20          => "Twenty"
    case 10, 30, 100 => "A Hundred"
    default          => "Unknown"
};

println(label); // A Hundred
```

---

## try / catch

Both `try` and `catch` bodies must be block statements `{ }`:

```javascript
try {
    local x = riskyOperation();
    println("success:", x);
} catch (e) {
    println("Error:", e);
}
```

{: .tip }
Any runtime error (type mismatch, undefined variable, etc.) can be caught with `try`/`catch`.

---

## break & continue

`break` exits the nearest enclosing loop or switch. `continue` skips to the next iteration. Both correctly unwind through nested `try`/`catch` blocks:

```javascript
for (i := 0; i < 10; i++) {
    if (i == 5) break;
    if (i % 2 == 0) continue;
    println(i); // 1 3
}
```

---

## Classes

Classes support **single inheritance**. The superclass is listed in parentheses after the class name. The constructor is named `init`.

```javascript
class Animal {
    fn speak() {
        println("...");
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

A subclass inherits all methods from the parent. Override a method by re-declaring it in the subclass. Access the instance with `this`.

---

## Static Members

Use `static fn` for static methods and `static name = value;` for static properties:

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

---

## Arrays

Arrays are dynamic and heterogeneous, using zero-based integer indexing:

```javascript
local list = [1, 2, 3];

list.push(4);           // append
list.pop();             // remove last → returns it
println(list.length()); // 3
println(list[0]);       // 1

// Spread
local more = [...list, 10, 20];
```

### Iteration with `each`

`each(callback)` calls `callback(index, element)` for every item and returns a new array of the return values:

```javascript
const nums = [1, 2, 3, 4, 5];

const doubled = nums.each(fn(i, e) { return e * 2; });
println(doubled); // [2, 4, 6, 8, 10]

// println accepts varargs — index then value
nums.each(println);
// 0 1
// 1 2  ...
```

### Filtering with `keep`

```javascript
const odds = nums.keep(fn(i, e) { return e % 2 != 0; });
println(odds); // [1, 3, 5]
```

---

## Objects

Objects are key-value maps. Access properties with dot or bracket notation:

```javascript
local person = {
    name: "Alice",
    age:  30
};

println(person.name);    // Alice
println(person["age"]);  // 30

// Shorthand: variable name == key name
local x = 10;
local obj = { x };       // equivalent to { x: x }

// Spread
local extended = { ...person, city: "NYC" };
```

---

## Imports

Three import forms are supported.

### Named imports

```javascript
import { println, scan, parseNum } from "core:io";
import { sqrt, pi, pow }           from "core:math";
import { getUser, getCwd }         from "core:os";
import { Array }                   from "core:Array";
import { Date }                    from "core:Date";
import { Promise }                 from "core:Promise";
```

### Wildcard import

The module is bound to its name:

```javascript
import "core:math";
println(math.sqrt(16)); // 4
println(math.pi);       // 3.141592...
```

### Relative file imports

```javascript
import "./utils";                        // runs ./utils.zs
import { helper } from "./helpers/math"; // named import from file
import { shared } from "../common";      // parent directory
```

{: .note }
The `.zs` extension is added automatically. Each module is executed once and cached; circular imports produce a runtime error.

---

## User Libraries (`lib:`)

User-authored `.zs` files placed in the `lib/` directory are importable via the `lib:` prefix. Subdirectories are supported using `/`:

```javascript
import { greet } from "lib:request";   // lib/request.zs
import "lib:nested/mod";               // lib/nested/mod.zs → bound as `mod`
```
