# Language-X Interpreter

A modern interpreted programming language implementation using LL(1) recursive descent parsing, written in Zig.

## Features

- **LL(1) Recursive Descent Parser**: Single-token lookahead, deterministic parsing with no backtracking
- **Expression Support**: Full operator precedence with 11 levels (from logical OR down to array indexing)
- **Data Structures**: First-class arrays and dictionaries
- **Multiple Number Formats**: Integers (decimal, hex, binary, octal) and floating-point (including scientific notation)
- **Unicode Support**: Proper UTF-8 handling for strings and identifiers
- **Comprehensive Error Reporting**: Position tracking for precise error messages

## Language Syntax

### Literals

```javascript
// Integers (multiple bases)
42          // Decimal
0xff        // Hexadecimal
0b1010      // Binary
0o755       // Octal

// Floating-point
3.14
2.5e10      // Scientific notation

// Strings
"hello world"
'single quotes'

// Booleans
true
false

// Null
null
```

### Arrays

```javascript
// Array literals
[1, 2, 3, 4, 5]
["hello", "world"]
[1, "mixed", true, null]
[]  // Empty array

// Nested arrays
[[1, 2], [3, 4], [5, 6]]

// Array indexing
arr[0]
matrix[i][j]  // Multi-dimensional
```

### Dictionaries

```javascript
// Dictionary literals
{"name": "John", "age": 30}
{x: 10, y: 20}  // Shorthand key syntax
{}  // Empty dictionary

// Nested dictionaries
{
    "user": {
        "name": "Alice",
        "active": true
    }
}

// Dictionary access
person["name"]
config["database"]["host"]
```

### Operators

Operators are listed from lowest to highest precedence:

```javascript
// Logical OR (lowest precedence)
a || b

// Logical AND
a && b

// Bitwise OR
a | b

// Bitwise XOR
a ^ b

// Bitwise AND
a & b

// Equality
a == b
a != b

// Comparison
a < b
a <= b
a > b
a >= b

// Bitwise Shift
a << b
a >> b

// Addition/Subtraction
a + b
a - b

// Multiplication/Division/Modulo
a * b
a / b
a % b

// Array/Dictionary Indexing (highest precedence)
arr[index]
dict["key"]
```

### Complex Expressions

```javascript
// Arithmetic with proper precedence
3 + 4 * 5           // 23 (not 35)
(3 + 4) * 5         // 35 (parentheses override)

// Mixed operations
arr[i + 1] * 2
dict["value"] + offset
matrix[i][j] * scale

// Chained indexing
users[0]["name"]
data["items"][index]
```

## Building

Requires Zig 0.16 or later. Tested with Zig 0.16.0.

```bash
# Build the interpreter
zig build

# Run the interpreter
zig build run -- <source-file>

# Run with an expression
zig build run -- -e "3 + 4 * 5"

# Run tests
zig build test
```

## Usage

### From File

Create a source file (e.g., `example.lx`):

```javascript
[1, 2, 3, 4, 5]
```

Run the interpreter:

```bash
zig build run -- example.lx
```

### From Expression

```bash
# Simple expression
zig build run -- -e "3 + 4 * 5"

# Array expression
zig build run -- -e "[1, 2, 3]"

# Dictionary expression
zig build run -- -e '{"name": "John", "age": 30}'

# Complex expression
zig build run -- -e "matrix[i][j] + offset * 2"
```

## Project Structure

```
language-x/
├── src/
│   ├── main.zig        # Main entry point
│   ├── position.zig    # Position tracking for error reporting
│   ├── token.zig       # Token types and definitions
│   ├── tokenizer.zig   # Lexical analyzer
│   ├── ast.zig         # Abstract Syntax Tree definitions
│   └── parser.zig      # LL(1) recursive descent parser
├── build.zig           # Zig build configuration
├── cursor.md           # Project context and specifications
└── README.md           # This file
```

## Architecture

### Tokenizer (Lexical Analysis)

The tokenizer converts source code into a stream of tokens:

- **Position Tracking**: Every token tracks its line and column position
- **Unicode Support**: Proper handling of UTF-8 codepoints
- **Comments**: Support for `//` single-line and `/* */` multi-line comments
- **Number Parsing**: Multiple bases (decimal, hex, binary, octal) and scientific notation

### Parser (Syntactic Analysis)

The parser uses **LL(1) recursive descent** strategy:

- **Single Token Lookahead**: All parsing decisions based on one token ahead
- **No Left Recursion**: Operators use iteration instead of left recursion
- **Precedence Climbing**: 11 levels of operator precedence via function layering
- **Deterministic**: No backtracking, linear O(n) time complexity

#### Parsing Methodology

Each precedence level is implemented as a separate function:

1. `expression()` → Entry point
2. `logicalOr()` → Logical OR (`||`)
3. `logicalAnd()` → Logical AND (`&&`)
4. `bitwiseOr()` → Bitwise OR (`|`)
5. `bitwiseXor()` → Bitwise XOR (`^`)
6. `bitwiseAnd()` → Bitwise AND (`&`)
7. `equality()` → Equality (`==`, `!=`)
8. `comparison()` → Comparison (`<`, `<=`, `>`, `>=`)
9. `shift()` → Bit shifts (`<<`, `>>`)
10. `additive()` → Addition/Subtraction (`+`, `-`)
11. `multiplicative()` → Multiplication/Division/Modulo (`*`, `/`, `%`)
12. `postfix()` → Array/Dict indexing (`[...]`)
13. `primary()` → Literals, identifiers, arrays, dicts, grouped expressions

### AST (Abstract Syntax Tree)

The AST represents the syntactic structure of the program:

- **Node Types**: 30+ AST node types covering all language constructs
- **Tree Structure**: Binary tree with left/right children for operators
- **Linked Lists**: Next pointers for array elements and dictionary entries
- **Memory Management**: Arena allocation for efficient cleanup

## LL(1) Grammar

The language implements this formal LL(1) grammar:

```
expression      → logicalOr
logicalOr       → logicalAnd ( "||" logicalAnd )*
logicalAnd      → bitwiseOr ( "&&" bitwiseOr )*
bitwiseOr       → bitwiseXor ( "|" bitwiseXor )*
bitwiseXor      → bitwiseAnd ( "^" bitwiseAnd )*
bitwiseAnd      → equality ( "&" equality )*
equality        → comparison ( ( "==" | "!=" ) comparison )*
comparison      → shift ( ( "<" | "<=" | ">" | ">=" ) shift )*
shift           → additive ( ( "<<" | ">>" ) additive )*
additive        → multiplicative ( ( "+" | "-" ) multiplicative )*
multiplicative  → postfix ( ( "*" | "/" | "%" ) postfix )*
postfix         → primary ( "[" expression "]" )*

primary         → TT_IDN
                | TT_INT
                | TT_NUM
                | TT_STR
                | "true"
                | "false"
                | "null"
                | array
                | dictionary
                | "(" expression ")"

array           → "[" ( expression ( "," expression )* )? "]"
dictionary      → "{" ( keyValue ( "," keyValue )* )? "}"
keyValue        → ( TT_STR | TT_IDN ) ":" expression
```

## Examples

See the `examples/` directory for sample programs (to be created).

## Testing

The project includes comprehensive unit tests:

```bash
# Run all tests
zig build test
```

Tests cover:
- Simple expressions
- Array literals and indexing
- Dictionary literals and access
- Nested structures
- Operator precedence
- Edge cases

## Future Enhancements

- [ ] Evaluator/Interpreter (runtime execution)
- [ ] Variable declarations and assignments
- [ ] Function definitions and calls
- [ ] Control flow (if/else, while, for)
- [ ] Standard library functions
- [ ] REPL (Read-Eval-Print Loop)
- [ ] Better error recovery
- [ ] Debugger support

## License

MIT License - See LICENSE file for details

## Contributing

Contributions welcome! Please read CONTRIBUTING.md for guidelines.

## Author

Created as a demonstration of LL(1) recursive descent parsing in Zig.

