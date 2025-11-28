# Project Context: Language-X Interpreter Development

## Agent Role & Expertise

You are a **Zig Programming Language Expert** and **Language Design Specialist** with deep expertise in:

- **Zig Language Mastery**: Advanced knowledge of Zig's syntax, standard library, memory management, comptime features, and idiomatic patterns
- **LL(1) Recursive Descent Parsing**: Expert-level understanding of LL(1) grammar design, single-token lookahead parsing, left-recursion elimination, and precedence climbing
- **Interpreter Development**: Extensive experience building interpreters, including lexical analysis, LL(1) parsing, AST construction, semantic analysis, and runtime execution
- **Language Design**: Strong understanding of programming language theory, formal grammar design (especially LL(1)), type systems, and language feature implementation
- **Parser Implementation**: Proven ability to transform EBNF/BNF grammars into efficient recursive descent parsers with proper error handling and recovery
- **Systems Programming**: Proficiency in low-level programming concepts, memory management, and performance optimization

## Technical Focus

When working on this project:

1. **LL(1) Parsing**: Implement strict LL(1) recursive descent parsing with single-token lookahead, no backtracking, and deterministic grammar
2. **Code Quality**: Write idiomatic, performant Zig code following best practices
3. **Memory Safety**: Leverage Zig's compile-time safety features and explicit allocator patterns (especially arena allocators for AST)
4. **Interpreter Architecture**: Design clean, modular interpreter components (lexer, LL(1) parser, evaluator, runtime)
5. **Grammar Design**: Ensure all grammar rules are LL(1) compliant (no left recursion, left-factored, disjoint first sets)
6. **Language Features**: Implement language constructs with careful attention to semantics and edge cases
7. **Precedence**: Implement operator precedence correctly through layered parsing functions
8. **Error Handling**: Provide clear, informative error messages with position tracking and panic-mode recovery
9. **Testing**: Write comprehensive tests for all language features, especially parser edge cases
10. **Documentation**: Maintain clear documentation for language syntax, grammar rules, and implementation details
11. **Naming Conventions**: Enforce strict naming conventions - **camelCase** for local scope (variables, parameters), **PascalCase** for global scope (functions, classes, attributes, enums)

## Project Goals

This project involves developing an interpreter for a custom programming language using Zig. Key considerations:

- **Performance**: Optimize for execution speed while maintaining code clarity
- **Correctness**: Ensure proper handling of all language semantics
- **Extensibility**: Design for easy addition of new language features
- **Developer Experience**: Create helpful error messages and debugging capabilities

## Coding Standards

- Use Zig 0.11+ features and conventions
- Prefer explicit over implicit behavior
- Use comptime features where appropriate for performance and type safety
- Implement proper error handling with Zig's error unions
- Write tests for all public APIs and language features
- Document complex algorithms and design decisions
- Follow Zig's standard library naming conventions (camelCase for functions, PascalCase for types)

### Language-X Naming Conventions

The Language-X language enforces strict naming conventions for clarity and consistency:

#### camelCase (for local scope)
- **Local variables**: `let userName = "John";`, `let itemCount = 0;`
- **Function parameters**: `fn processUser(userId, accountType) { }`
- **Loop variables**: `for (let i = 0; i < 10; i = i + 1) { }`
- **Private/local identifiers**: Any identifier with local scope

#### PascalCase (for global/type scope)
- **Global variables**: `let GlobalConfig = {...};`, `let DatabaseConnection = null;`
- **Struct/Class names**: `class UserAccount { }`, `class HttpRequest { }`
- **Struct/Class attributes/fields**: `x = 0;`, `Name = "";`, `IsActive = true;`
- **Enum names**: `enum StatusCode { }`, `enum Color { }`
- **Enum values**: `Red`, `Green`, `Blue`, `Active`, `Pending`
- **Function/Method names**: `fn ProcessData() { }`, `fn CalculateTotal() { }`
- **Constants**: `let MaxRetries = 3;`, `let ApiEndpoint = "...";`

#### Examples

```javascript
// Global variables and constants (PascalCase)
let MaxConnections = 100;
let DefaultTimeout = 5000;

// Enum definition (PascalCase for name and values)
enum HttpMethod {
    Get,
    Post,
    Put,
    Delete
}

// Class definition (PascalCase)
class UserProfile {
    // Class attributes (PascalCase)
    FirstName = "";
    LastName = "";
    Age = 0;
    IsActive = true;
    
    // Methods (PascalCase)
    fn Initialize(firstName, lastName, age) {
        // Parameters (camelCase)
        // Local variable (camelCase)
        let fullName = firstName + " " + lastName;
        
        FirstName = firstName;  // Attribute (PascalCase)
        LastName = lastName;
        Age = age;
        IsActive = true;
    }
    
    fn GetDisplayName() {
        return FirstName + " " + LastName;
    }
    
    fn UpdateAge(newAge) {
        // Parameter (camelCase)
        Age = newAge;  // Attribute (PascalCase)
    }
}

// Function definition (PascalCase)
fn CalculateTotalPrice(items, taxRate) {
    // Parameters (camelCase)
    let subtotal = 0;  // Local variable (camelCase)
    let index = 0;
    
    // Loop variable (camelCase)
    for (let i = 0; i < items.length; i = i + 1) {
        let item = items[i];  // Local variable (camelCase)
        let price = item.Price;  // Attribute access (PascalCase)
        subtotal = subtotal + price;
    }
    
    let tax = subtotal * taxRate;
    let total = subtotal + tax;
    
    return total;
}

// Usage
let currentUser = UserProfile;  // Local variable (camelCase)
currentUser.Initialize("John", "Doe", 30);

let shoppingCart = [...];  // Local variable (camelCase)
let finalPrice = CalculateTotalPrice(shoppingCart, 0.08);
```

#### Naming Convention Rules Summary

| Identifier Type | Convention | Examples |
|----------------|------------|----------|
| Local variables | camelCase | `userName`, `itemCount`, `isValid` |
| Function parameters | camelCase | `userId`, `maxRetries`, `callback` |
| Loop variables | camelCase | `i`, `index`, `counter` |
| Global variables | PascalCase | `GlobalConfig`, `AppSettings` |
| Functions/Methods | PascalCase | `ProcessData()`, `GetUser()` |
| Classes | PascalCase | `UserAccount`, `HttpClient` |
| Class attributes | PascalCase | `FirstName`, `IsActive`, `TotalCount` |
| Enums | PascalCase | `StatusCode`, `Color`, `Direction` |
| Enum values | PascalCase | `Red`, `Active`, `North` |
| Constants | PascalCase | `MaxRetries`, `DefaultPort` |

#### Why These Conventions?

1. **Visual Distinction**: Immediately distinguish between local scope (camelCase) and global/type scope (PascalCase)
2. **Scope Clarity**: Know at a glance whether an identifier is local or has broader scope
3. **Type Recognition**: PascalCase signals types, classes, and global definitions
4. **Consistency**: Aligns with common conventions in C#, Pascal, and other languages
5. **Readability**: Clear differentiation improves code comprehension

## Architecture Principles

- **Modularity**: Separate concerns (lexer, parser, AST, evaluator, runtime)
- **Testability**: Design components to be independently testable
- **Clarity**: Prioritize readable code that expresses intent clearly
- **Efficiency**: Use appropriate data structures and algorithms for interpreter performance

## Current Language Design & Implementation

### Tokenizer/Lexer Architecture

The language-x interpreter uses a structured tokenization system with the following core components:

#### Position Tracking
```zig
struct Position {
    // Public attributes
    LineStart: u32,
    LineEnded: u32,
    ColmStart: u32,
    ColmEnded: u32,
    
    pub func Merge(other: Position) -> Position {
        // Merges two position ranges for compound tokens
        // Returns a new Position spanning from this.Start to other.End
    }
}
```

**Purpose**: Track precise source code locations for error reporting and debugging. Each token maintains its start/end line and column positions.

#### Token Types
```zig
enum TokenType {
    TT_IDN,  // Identifiers (variable names, function names)
    TT_KEY,  // Keywords (if, while, func, struct, etc.)
    TT_NUM,  // Floating-point numbers (0.2, 2.2, 2.2e2)
    TT_INT,  // Integers with multiple bases (100, 0xff, 0b1010, 0o232)
    TT_STR,  // String literals
    TT_SYM,  // Symbols/operators (+, -, *, ==, etc.)
    TT_EOF   // End of file marker
}
```

**Design Notes**:
- Separate INT and NUM types for performance and type system clarity
- Support for multiple integer literal formats (decimal, hex, binary, octal)
- Scientific notation support for floating-point numbers (e.g., 2.2e2)

#### Token Structure
```zig
struct Token {
    Type: TokenType,
    Value: []u8,        // Raw lexeme from source
    Position: Position  // Location information
}
```

**Purpose**: Encapsulates all information about a lexical token including its type, textual value, and source location.

#### Tokenizer/Lexer
```zig
struct Tokenizer {
    // Public fields
    Path: []u8,  // File path for error messages
    Data: []u8,  // Source code buffer
    
    pub func NextToken() -> Token {
        // Lexical analysis logic:
        // - Skip whitespace and comments
        // - Recognize keywords vs identifiers
        // - Parse numeric literals (int/float, various bases)
        // - Parse string literals with escape sequences
        // - Recognize symbols/operators
        // - Track position for each token
        // - Handle Unicode codepoints correctly
    }
}
```

**Key Requirements**:
- **Unicode Support**: Must correctly handle Unicode codepoints for both string literals and identifiers
- **Incremental Tokenization**: NextToken() yields one token at a time for efficient parsing
- **Error Recovery**: Should provide meaningful error messages with position information
- **Performance**: Minimize allocations, use efficient string scanning techniques

### Implementation Guidelines

When implementing or modifying the tokenizer:

1. **Unicode Handling**: Use Zig's standard library UTF-8 functions for proper codepoint iteration
2. **Memory Management**: Pass an allocator for token value storage; consider arena allocation for tokens
3. **Peek Support**: Consider adding peek functionality for lookahead during parsing
4. **Comments**: Define and implement single-line (//) and multi-line (/* */) comment handling
5. **String Escapes**: Support standard escape sequences (\n, \t, \", \\, \xHH, \uHHHH)
6. **Number Parsing**: Validate numeric literals during tokenization, handle edge cases (leading zeros, invalid digits for base)
7. **Keyword Recognition**: Use a hash map or perfect hash for efficient keyword lookup
8. **Error Messages**: Include position, context (surrounding code), and suggestions when reporting errors

### Token Flow

```
Source Code → Tokenizer.NextToken() → Token Stream → Parser → AST → Evaluator → Result
```

The tokenizer is the first stage in the interpreter pipeline and must be robust, fast, and provide accurate position information for all downstream components.

#### Tokenizer Initialization
```zig
pub func TokenizerInit(path: []u8, data: []u8) -> Tokenizer {
    // Initialize tokenizer with file path and source code
    // Set up internal state (current position, line/column tracking)
    // Return initialized Tokenizer struct (value type, not pointer)
}
```

**Design Note**: Returns a `Tokenizer` by value (non-pointer type) for stack allocation and clear ownership semantics.

---

### Parser & AST Architecture

The parser transforms the token stream into an Abstract Syntax Tree (AST) using **LL(1) Recursive Descent Parsing**.

#### Parsing Strategy: LL(1) Recursive Descent

**LL(1)** stands for:
- **L**: Left-to-right scanning of input
- **L**: Leftmost derivation
- **(1)**: One token lookahead

**Recursive Descent** means:
- Each grammar rule is implemented as a function
- Functions call each other recursively to parse nested structures

**Key Advantages**:
- Simple and intuitive to implement
- Easy to debug and maintain
- Natural mapping from grammar to code
- Efficient for most programming language constructs
- Direct control over error messages

**LL(1) Requirements**:
1. **No Left Recursion**: Grammar must not have left-recursive rules
2. **Deterministic**: Must be able to choose production based on single lookahead token
3. **Left-Factored**: Common prefixes must be factored out
4. **Disjoint First Sets**: Alternative productions must have disjoint starting tokens

#### LL(1) Parsing Example

Consider parsing the expression: `3 + 4 * 5`

**Token Stream**: `[TT_INT:3] [TT_SYM:+] [TT_INT:4] [TT_SYM:*] [TT_INT:5] [TT_EOF]`

**LL(1) Parse Trace**:
```
1. expression() 
   - Calls logicalOr() → ... → additive()
   - current_token = TT_INT:3

2. additive()
   - Calls multiplicative()
   - current_token = TT_INT:3

3. multiplicative()
   - Calls postfix() → primary()
   - current_token = TT_INT:3

4. primary()
   - peek() returns TT_INT
   - Match! Create AstInt(3)
   - advance() → current_token = TT_SYM:+
   - Return AstInt(3)

5. Back in multiplicative()
   - Check for *, /, % → No (current = +)
   - Return AstInt(3)

6. Back in additive()
   - left = AstInt(3)
   - Check for +, - → Yes! (current = +)
   - advance() → current_token = TT_INT:4
   - Call multiplicative() for right side

7. multiplicative()
   - Calls postfix() → primary()
   - Creates AstInt(4)
   - Check for *, /, % → Yes! (current = *)
   - advance() → current_token = TT_INT:5
   - Call postfix() → primary()
   - Creates AstInt(5)
   - Build AstMul(AstInt(4), AstInt(5))
   - Return AstMul node

8. Back in additive()
   - right = AstMul(4, 5)
   - Build AstAdd(AstInt(3), AstMul(4, 5))
   - Check for more +, - → No (EOF)
   - Return AstAdd node

Final AST:
    AstAdd
    ├─ AstInt(3)
    └─ AstMul
       ├─ AstInt(4)
       └─ AstInt(5)

Result: (3 + (4 * 5)) = 23 ✓ Correct precedence!
```

**LL(1) Key Observations**:
- Only ONE token lookahead needed at each decision point
- No backtracking - each decision is final
- Precedence naturally handled by function call depth
- Linear time O(n) - single pass through tokens

---

#### AST Node Types
```zig
enum AstType {
    AstIdn,      // Identifier
    AstInt,      // Integer literal
    AstNum,      // Float/double literal
    AstTrue,     // Boolean true
    AstFalse,    // Boolean false
    AstNull,     // Null literal
    
    // Arithmetic operators
    AstMul,      // Multiplication (*)
    AstDiv,      // Division (/)
    AstMod,      // Modulo (%)
    AstAdd,      // Addition (+)
    AstSub,      // Subtraction (-)
    AstLShft,    // Left shift (<<)
    AstRShft,    // Right shift (>>)
    
    // Comparison operators
    AstLt,       // Less than (<)
    AstLte,      // Less than or equal (<=)
    AstGt,       // Greater than (>)
    AstGte,      // Greater than or equal (>=)
    AstEq,       // Equal (==)
    AstNeq,      // Not equal (!=)
    
    // Bitwise operators
    AstAnd,      // Bitwise AND (&)
    AstOr,       // Bitwise OR (|)
    AstXor,      // Bitwise XOR (^)
    
    // Logical operators
    AstLAnd,     // Logical AND (&&)
    AstLOr,      // Logical OR (||)
    
    // Data structures
    AstArray,    // Array literal [1, 2, 3]
    AstDict,     // Dictionary/Object literal {"key": value}
    AstIndex,    // Array/dict indexing: arr[0], dict["key"]
    AstKeyValue, // Key-value pair for dictionary entries
    
    // Complex structures
    AstFunc,     // Function definition
    AstCall,     // Function call
    AstBlock,    // Block statement { }
    AstReturn,   // Return statement
    AstBreak,    // Break statement
    AstContinue, // Continue statement
    
    // Control flow
    AstIf,       // If statement
    AstElse,     // Else clause
    AstWhile,    // While loop
    AstDoWhile,  // Do-while loop
    AstFor,      // For loop
    
    // Object-oriented
    AstClass,    // Class definition
    AstEnum,     // Enum definition
    AstMember,   // Class member access (.)
    AstMethod,   // Method definition
    
    // Statements
    AstVarDecl,  // Variable declaration
    AstAssign,   // Assignment statement
    AstExprStmt, // Expression statement
    
    // Add other syntax types as needed
}
```

**Design Philosophy**: Each operator and construct has its own AST type for clear semantic representation and easy pattern matching during evaluation.

#### AST Node Structure
```zig
struct Ast {
    Type: AstType,      // The kind of AST node
    Position: Position, // Source location for error reporting
    Str: []u8,          // String value (for identifiers, literals)
    Next: *Ast,         // Linked list for siblings/children
}
```

**Structure Design**:
- **Type**: Discriminates between different AST node types
- **Position**: Preserves source location for runtime error messages
- **Str**: Stores textual content (variable names, literal values)
- **Next**: Links to child/sibling nodes forming tree structure

**Note**: This is a simplified linked-list-based AST. Consider adding:
- Left/Right child pointers for binary operators
- Children array for variable-arity nodes
- Value union for typed literals (avoiding string conversion overhead)

#### AST Node Initialization
```zig
pub func GenericAstInit(type: AstType, pos: Position) -> *Ast {
    // Allocate new AST node on heap
    // Initialize with given type and position
    // Set Str to empty, Next to null
    // Return pointer to allocated node
}
```

**Memory Model**: Returns a heap-allocated AST node (`*Ast` pointer type). The caller is responsible for managing the lifetime (consider arena allocator for parse trees).

#### Parser Structure (LL(1) Implementation)
```zig
struct Parser {
    // Private state - essential for LL(1) parsing
    tokenizer: Tokenizer,     // Token source
    current_token: Token,      // Current lookahead token (the "1" in LL(1))
    allocator: Allocator,      // Memory allocator for AST nodes
    
    // === Public API ===
    
    pub func Parse() -> *Ast {
        // Main entry point for parsing
        // 1. Initialize by reading first token
        // 2. Parse top-level expression/statement
        // 3. Expect EOF token at end
        // Returns root of AST tree
    }
    
    // === LL(1) Helper Functions ===
    
    private func advance() {
        // Consume current token and fetch next
        // Updates current_token with tokenizer.NextToken()
        // Essential for moving through token stream
    }
    
    private func peek() -> TokenType {
        // Return type of current lookahead token
        // Does NOT consume token (non-destructive)
        // Used for LL(1) decision making
    }
    
    private func expect(expected: TokenType) -> bool {
        // Check if current token matches expected type
        // If match: consume token (advance) and return true
        // If mismatch: report error and return false
        // Critical for grammar enforcement
    }
    
    private func match(types: []TokenType) -> bool {
        // Check if current token matches any in given list
        // Used for productions with multiple alternatives
        // Returns true if match found, false otherwise
    }
    
    // === Recursive Descent Parsing Methods ===
    // Each method corresponds to a grammar non-terminal
    // Methods return null on parse failure
    
    private func expression() -> *Ast {
        // Top-level expression parsing
        // Entry point for expression grammar
        // Calls logicalOr() for precedence handling
    }
    
    private func logicalOr() -> *Ast {
        // Precedence level 11: ||
        // Grammar: logicalAnd ( "||" logicalAnd )*
        // Left-associative via iteration (not left recursion)
    }
    
    private func logicalAnd() -> *Ast {
        // Precedence level 10: &&
        // Grammar: bitwiseOr ( "&&" bitwiseOr )*
    }
    
    private func bitwiseOr() -> *Ast {
        // Precedence level 9: |
        // Grammar: bitwiseXor ( "|" bitwiseXor )*
    }
    
    private func bitwiseXor() -> *Ast {
        // Precedence level 8: ^
        // Grammar: bitwiseAnd ( "^" bitwiseAnd )*
    }
    
    private func bitwiseAnd() -> *Ast {
        // Precedence level 7: &
        // Grammar: equality ( "&" equality )*
    }
    
    private func equality() -> *Ast {
        // Precedence level 6: ==, !=
        // Grammar: comparison ( ("==" | "!=") comparison )*
    }
    
    private func comparison() -> *Ast {
        // Precedence level 5: <, <=, >, >=
        // Grammar: shift ( ("<" | "<=" | ">" | ">=") shift )*
    }
    
    private func shift() -> *Ast {
        // Precedence level 4: <<, >>
        // Grammar: additive ( ("<<" | ">>") additive )*
    }
    
    private func additive() -> *Ast {
        // Precedence level 3: +, -
        // Grammar: multiplicative ( ("+" | "-") multiplicative )*
    }
    
    private func multiplicative() -> *Ast {
        // Precedence level 2: *, /, %
        // Grammar: postfix ( ("*" | "/" | "%") postfix )*
    }
    
    private func postfix() -> *Ast {
        // Precedence level 1: array/dict indexing
        // Grammar: primary ( "[" expression "]" )*
        // Handles chained indexing: arr[i][j][k]
    }
    
    private func primary() -> *Ast {
        // Precedence level 0: atomic/base expressions
        // Grammar: TT_IDN | TT_INT | TT_NUM | "true" | "false" | "null" 
        //        | array | dictionary | "(" expression ")"
        // Uses lookahead (peek) to determine which production to use
        // Handles all atomic values and collection literals
        // Returns AST node or null on error
    }
    
    private func array() -> *Ast {
        // Parse array literals
        // Grammar: "[" ( expression ( "," expression )* )? "]"
        // Returns AstArray node with elements linked via Next
    }
    
    private func dictionary() -> *Ast {
        // Parse dictionary literals
        // Grammar: "{" ( keyValue ( "," keyValue )* )? "}"
        // Returns AstDict node with entries linked via Next
    }
    
    private func keyValue() -> *Ast {
        // Parse dictionary key-value pair
        // Grammar: (TT_STR | TT_IDN) ":" expression
        // Returns AstKeyValue node
    }
    
    private func function() -> *Ast {
        // Parse function definitions
        // Grammar: "fn" TT_IDN "(" params ")" "{" body "}"
        // Returns AstFunc node
    }
}
```

**LL(1) Design Principles Applied**:

1. **One Token Lookahead**: `current_token` holds the lookahead; decisions based on `peek()`
2. **Left Recursion Eliminated**: 
   - Bad (left-recursive): `expr → expr + term`
   - Good (iterative): `expr → term ("+" term)*`
   - Implementation uses while loops, not recursion
3. **Left Factoring**: Common prefixes extracted
4. **Top-Down**: Start from `expression()`, descend to `primary()` (atomic expressions)
5. **Precedence Climbing**: Each precedence level is a separate function

### Supported Language Syntax

The parser implements the following grammar:

#### Primary Expressions
```
primary:
    identifier  |   // TT_IDN
    integer     |   // TT_INT
    number      |   // TT_NUM (float or double)
    true        |   // TT_KEY "true"
    false       |   // TT_KEY "false"
    null        |   // TT_KEY "null"
    array       |   // [elements]
    dictionary  |   // {key: value, ...}
    "(" expression ")"  // Grouped expression
```

**Parsing Rule**: 
- **Primary expressions** are atomic/base expressions - the building blocks of all expressions
- They form the leaves of the expression tree (except grouped expressions which contain sub-expressions)
- These form the base of expressions before operators and indexing are applied
- Use `peek()` on current token to determine which production to parse
- Returns AST node or null if current token doesn't match any primary expression

#### Expression Precedence (Complete)
```
Expression Precedence (highest to lowest):
    0. Primary: literals, identifiers, arrays, dictionaries, ( )
    1. Postfix: array/dict indexing [...]
    2. Multiplicative: *, /, %
    3. Additive: +, -
    4. Bitwise Shift: <<, >>
    5. Comparison: <, <=, >, >=
    6. Equality: ==, !=
    7. Bitwise AND: &
    8. Bitwise XOR: ^
    9. Bitwise OR: |
   10. Logical AND: &&
   11. Logical OR: ||
```

**Implementation Strategy**: 
- Use precedence climbing with separate parsing methods for each precedence level
- Each level implements **LL(1) recursive descent** pattern
- **Postfix operators** (indexing) bind tighter than any infix operator
- **Examples**:
  - `arr[0] + 1` parses as `(arr[0]) + 1`
  - `dict["key"] * 2` parses as `(dict["key"]) * 2`
  - `matrix[i][j]` parses as `(matrix[i])[j]` (left-to-right)

---

### LL(1) Grammar Design & Implementation Patterns

#### Pattern 1: Binary Operators (Left-Associative)

**Grammar (LL(1) compliant)**:
```
additive → multiplicative ( ("+" | "-") multiplicative )*
```

**Implementation**:
```zig
private func additive() -> *Ast {
    var left = multiplicative();  // Parse left operand
    if (left == null) return null;
    
    // Iteration replaces left recursion
    while (match([TT_SYM]) and (current_token.Value == "+" or current_token.Value == "-")) {
        var op = current_token.Value;
        var pos = current_token.Position;
        advance();  // Consume operator
        
        var right = multiplicative();  // Parse right operand
        if (right == null) {
            // Error: expected expression after operator
            return null;
        }
        
        // Build AST node
        var node = GenericAstInit(op == "+" ? AstAdd : AstSub, pos);
        node.Next = left;        // Left child
        left.Next = right;       // Right child (or use separate field)
        left = node;             // New left for next iteration
    }
    
    return left;
}
```

**Key Points**:
- Uses **iteration** instead of left recursion
- **LL(1) decision**: Look at current token to decide whether to continue loop
- **Left-associative**: `a + b + c` parses as `(a + b) + c`
- **No backtracking**: Greedy consumption of operators

#### Pattern 2: Postfix Operators (Indexing)

**Grammar (LL(1) compliant)**:
```
postfix → primary ( "[" expression "]" )*
```

**Implementation**:
```zig
private func postfix() -> *Ast {
    var base = primary();  // Parse base expression
    if (base == null) return null;
    
    // Handle chained indexing: arr[i][j][k]
    while (match([TT_SYM]) and current_token.Value == "[") {
        var pos = current_token.Position;
        advance();  // Consume "["
        
        var index = expression();  // Parse index expression
        if (index == null) {
            // Error: expected expression inside brackets
            return null;
        }
        
        if (!expect(TT_SYM) or current_token.Value != "]") {
            // Error: expected closing "]"
            return null;
        }
        advance();  // Consume "]"
        
        // Build index node
        var node = GenericAstInit(AstIndex, pos);
        node.Next = base;      // Container
        base.Next = index;     // Index expression
        base = node;           // New base for next iteration
    }
    
    return base;
}
```

**Key Points**:
- **Postfix** means operator comes after operand
- Enables **chained indexing**: `matrix[i][j]` → `Index(Index(matrix, i), j)`
- **LL(1) decision**: Check for `[` token to continue

#### Pattern 3: Primary Expressions (Alternatives)

**Grammar (LL(1) compliant)**:
```
primary → TT_IDN 
        | TT_INT 
        | TT_NUM 
        | "true" 
        | "false" 
        | "null"
        | "[" ...          // Array
        | "{" ...          // Dictionary
        | "(" expression ")"
```

**Implementation**:
```zig
private func primary() -> *Ast {
    var type = peek();  // LL(1) lookahead
    var pos = current_token.Position;
    
    // Use lookahead to determine which production
    switch (type) {
        TT_IDN => {
            var node = GenericAstInit(AstIdn, pos);
            node.Str = current_token.Value;
            advance();
            return node;
        },
        TT_INT => {
            var node = GenericAstInit(AstInt, pos);
            node.Str = current_token.Value;
            advance();
            return node;
        },
        TT_NUM => {
            var node = GenericAstInit(AstNum, pos);
            node.Str = current_token.Value;
            advance();
            return node;
        },
        TT_KEY => {
            if (current_token.Value == "true") {
                advance();
                return GenericAstInit(AstTrue, pos);
            } else if (current_token.Value == "false") {
                advance();
                return GenericAstInit(AstFalse, pos);
            } else if (current_token.Value == "null") {
                advance();
                return GenericAstInit(AstNull, pos);
            }
            return null;
        },
        TT_SYM => {
            if (current_token.Value == "[") {
                return array();  // Delegate to array parser
            } else if (current_token.Value == "{") {
                return dictionary();  // Delegate to dict parser
            } else if (current_token.Value == "(") {
                advance();  // Consume "("
                var expr = expression();  // Parse inner expression
                if (!expect(TT_SYM) or current_token.Value != ")") {
                    // Error: expected ")"
                    return null;
                }
                advance();  // Consume ")"
                return expr;  // Return inner expression (parentheses just group)
            }
            return null;
        },
        else => return null;  // Not a primary expression
    }
}
```

**Key Points**:
- **LL(1) decision**: Single lookahead determines production
- **Disjoint First Sets**: Each alternative starts with unique token
- **No backtracking**: Decision made immediately
- **Delegation**: Complex cases (array, dict) delegated to specialized functions

#### Pattern 4: Lists (Arrays, Function Arguments)

**Grammar (LL(1) compliant)**:
```
array → "[" ( expression ( "," expression )* )? "]"
```

**Implementation**:
```zig
private func array() -> *Ast {
    var pos = current_token.Position;
    advance();  // Consume "["
    
    var arrayNode = GenericAstInit(AstArray, pos);
    
    // Check for empty array
    if (peek() == TT_SYM and current_token.Value == "]") {
        advance();  // Consume "]"
        return arrayNode;  // Empty array
    }
    
    // Parse first element
    var first = expression();
    if (first == null) {
        // Error: expected expression
        return null;
    }
    
    var current = first;
    
    // Parse remaining elements
    while (peek() == TT_SYM and current_token.Value == ",") {
        advance();  // Consume ","
        
        // Optional: allow trailing comma
        if (peek() == TT_SYM and current_token.Value == "]") {
            break;  // Trailing comma before "]"
        }
        
        var elem = expression();
        if (elem == null) {
            // Error: expected expression after comma
            return null;
        }
        
        current.Next = elem;  // Link elements
        current = elem;
    }
    
    // Expect closing bracket
    if (!expect(TT_SYM) or current_token.Value != "]") {
        // Error: expected "]"
        return null;
    }
    advance();  // Consume "]"
    
    arrayNode.Next = first;  // Link first element to array node
    return arrayNode;
}
```

**Key Points**:
- **Optional production**: `?` handled by checking for `]` immediately
- **Repetition**: `*` implemented with while loop
- **Trailing commas**: Optional feature, check before parsing next element
- **Empty case**: Special handling for `[]`

#### Pattern 5: Error Recovery in LL(1)

**Synchronization Points**:
```zig
private func synchronize() {
    // After error, skip tokens until we reach a statement boundary
    var sync_tokens = [TT_EOF, TT_SYM];  // ; } EOF
    
    while (!match(sync_tokens)) {
        advance();
    }
    
    // Now at safe point to continue parsing
}
```

**Error Reporting with Context**:
```zig
private func reportError(message: []u8, pos: Position) {
    // Error: {message}
    // At line {pos.LineStart}, column {pos.ColmStart}
    // Show source code context with position highlighted
}
```

**Key Points**:
- **Panic Mode Recovery**: Skip to synchronization point
- **Sync Tokens**: Statement boundaries, blocks, EOF
- **Continue Parsing**: Don't stop at first error (collect multiple errors)

---

### LL(1) Parsing Decision Table

For each non-terminal, document which production to use based on lookahead:

| Non-terminal | Lookahead Token | Production |
|--------------|-----------------|------------|
| `primary` | `TT_IDN` | identifier |
| `primary` | `TT_INT` | integer literal |
| `primary` | `TT_NUM` | number literal |
| `primary` | `"true"` / `"false"` / `"null"` | boolean/null |
| `primary` | `"["` | array literal |
| `primary` | `"{"` | dictionary literal |
| `primary` | `"("` | grouped expression |
| `additive` | any | `multiplicative (("+" \| "-") multiplicative)*` |
| `postfix` | any | `primary ("[" expression "]")*` |

**Usage**: Before parsing a non-terminal, check lookahead token to determine which production to use. If token doesn't match any production, report syntax error.

---

### Naming Convention Enforcement

The parser **does not enforce** naming conventions during parsing - this is the responsibility of a semantic analyzer or linter. However, developers should be aware of the conventions when writing Language-X code:

#### Parser's Role
- **Accepts**: Any valid identifier (alphanumeric + underscore, starting with letter/underscore)
- **Tokenizes**: All identifiers as `TT_IDN` regardless of casing
- **Passes through**: Identifier names unchanged to AST

#### Semantic Analyzer's Role (Future)
- **Validates**: Naming conventions based on scope and context
- **Warns**: When conventions are violated (e.g., local variable using PascalCase)
- **Enforces**: Consistent naming across the codebase

#### Convention Detection Heuristics

When building a semantic analyzer, use these heuristics:

```zig
fn validateIdentifierNaming(name: []const u8, context: IdentifierContext) !void {
    const isGlobalScope = context.scope == .Global;
    const isTypeDefinition = context.kind == .Class or context.kind == .Enum;
    const isFunctionOrMethod = context.kind == .Function or context.kind == .Method;
    const isClassAttribute = context.kind == .ClassAttribute;
    
    const isPascalCase = std.ascii.isUpper(name[0]);
    const isCamelCase = std.ascii.isLower(name[0]);
    
    // Validate based on context
    if (isGlobalScope or isTypeDefinition or isFunctionOrMethod or isClassAttribute) {
        if (!isPascalCase) {
            // Warning: Expected PascalCase for global/type identifier
            return error.InvalidNamingConvention;
        }
    } else {
        if (!isCamelCase) {
            // Warning: Expected camelCase for local identifier
            return error.InvalidNamingConvention;
        }
    }
}
```

#### Examples of Convention Violations

```javascript
// ❌ BAD: Local variable with PascalCase
fn ProcessData() {
    let UserName = "John";  // Should be: userName
    let ItemCount = 0;       // Should be: itemCount
}

// ❌ BAD: Function with camelCase
fn processData() {  // Should be: ProcessData
    return true;
}

// ❌ BAD: Class attribute with camelCase
class User {
    firstName = "";  // Should be: FirstName
    lastName = "";   // Should be: LastName
}

// ❌ BAD: Parameter with PascalCase
fn Calculate(ItemPrice, TaxRate) {  // Should be: itemPrice, taxRate
    return ItemPrice * TaxRate;
}

// ✓ GOOD: Correct conventions
fn CalculateTotal(itemPrice, taxRate) {
    let subtotal = itemPrice;  // Local: camelCase
    let tax = subtotal * taxRate;
    return subtotal + tax;
}
```

---

### Statement Parsing

Language-X is a **statement-based language** where programs consist of sequences of statements. The parser must distinguish between:
- **Expression statements**: Expressions followed by semicolon
- **Declaration statements**: Variable, function, class, enum declarations
- **Control flow statements**: if, for, while, do-while, return, break, continue
- **Block statements**: Compound statements enclosed in `{` `}`

#### Statement vs Expression Parsing

**Expression Parsing** (already implemented):
- Returns a value
- Can be nested arbitrarily
- Parsed by `expression()` and precedence functions
- Examples: `3 + 4`, `arr[0]`, `func(x)`

**Statement Parsing** (to be added):
- Performs an action
- Forms the top-level structure of programs
- May contain expressions
- Examples: `let x = 5;`, `if (x > 0) { }`, `return x;`

#### Parser Extension for Statements

Add to Parser struct:

```zig
// === Statement Parsing Methods ===

pub fn parseProgram(self: *Parser) !?*Ast {
    // Parse top-level statements until EOF
    // Returns AstBlock containing all statements
}

private func statement(self: *Parser) -> *Ast {
    // Entry point for statement parsing
    // Determines statement type based on lookahead token
    // Delegates to specific statement parsers
}

private func varDeclaration(self: *Parser) -> *Ast {
    // Parse: "let" identifier "=" expression ";"
}

private func ifStatement(self: *Parser) -> *Ast {
    // Parse: "if" "(" condition ")" block ("else" (ifStatement | block))?
}

private func whileStatement(self: *Parser) -> *Ast {
    // Parse: "while" "(" condition ")" block
}

private func doWhileStatement(self: *Parser) -> *Ast {
    // Parse: "do" block "while" "(" condition ")" ";"
}

private func forStatement(self: *Parser) -> *Ast {
    // Parse: "for" "(" init ";" condition ";" increment ")" block
}

private func returnStatement(self: *Parser) -> *Ast {
    // Parse: "return" expression? ";"
}

private func breakStatement(self: *Parser) -> *Ast {
    // Parse: "break" ";"
}

private func continueStatement(self: *Parser) -> *Ast {
    // Parse: "continue" ";"
}

private func block(self: *Parser) -> *Ast {
    // Parse: "{" statement* "}"
}

private func expressionStatement(self: *Parser) -> *Ast {
    // Parse: expression ";"
}

private func functionDeclaration(self: *Parser) -> *Ast {
    // Parse: "fn" identifier "(" params ")" block
}

private func classDeclaration(self: *Parser) -> *Ast {
    // Parse: "class" identifier "{" members "}"
}

private func enumDeclaration(self: *Parser) -> *Ast {
    // Parse: "enum" identifier "{" values "}"
}
```

---

### Control Flow Statements

#### If/Else Statement Syntax

```
if_statement:
    "if" "(" condition ")" block ("else" (if_statement | block))?

Where:
    - "if", "else" are keywords (TT_KEY)
    - condition is any expression
    - block is "{" statement* "}"
    - else clause is optional
    - else-if chains: else + if_statement
    
Examples:
    // Simple if (local variables use camelCase)
    if (value > 0) {
        Print("positive");  // Function (PascalCase)
    }
    
    // If-else
    if (value > 0) {
        Print("positive");
    } else {
        Print("non-positive");
    }
    
    // Else-if chain
    if (value > 0) {
        Print("positive");
    } else if (value < 0) {
        Print("negative");
    } else {
        Print("zero");
    }
    
    // Nested if
    if (value > 0) {
        if (value > 10) {
            Print("large");
        }
    }
```

**Parsing Steps**:
1. Expect "if" keyword
2. Expect "(" symbol
3. Parse condition expression
4. Expect ")" symbol
5. Parse then-block (call `block()`)
6. Check for optional "else" keyword
7. If "else" present:
   - Peek next token
   - If "if", recursively parse `ifStatement()` (else-if)
   - Otherwise, parse `block()` (final else)
8. Construct AstIf node with:
   - Condition in `left`
   - Then-block in `right`
   - Else-block (if any) in `next`

**AST Structure**:
```
AstIf
  ├─ left → condition_expression
  ├─ right → then_block (AstBlock)
  └─ next → else_clause (AstIf or AstBlock or null)
```

**LL(1) Compliance**:
- First set: {"if"}
- Deterministic: "if" keyword uniquely identifies if statement
- No left recursion
- Else clause: optional, determined by lookahead for "else" keyword

#### While Loop Syntax

```
while_statement:
    "while" "(" condition ")" block

Where:
    - "while" is keyword (TT_KEY)
    - condition is any expression
    - block is "{" statement* "}"
    
Examples:
    // Simple while
    while (x < 10) {
        x = x + 1;
    }
    
    // Infinite loop with break
    while (true) {
        if (done) {
            break;
        }
    }
    
    // Nested while
    while (i < n) {
        while (j < m) {
            process(i, j);
            j = j + 1;
        }
        i = i + 1;
    }
```

**Parsing Steps**:
1. Expect "while" keyword
2. Expect "(" symbol
3. Parse condition expression
4. Expect ")" symbol
5. Parse body block
6. Construct AstWhile node with:
   - Condition in `left`
   - Body block in `right`

**AST Structure**:
```
AstWhile
  ├─ left → condition_expression
  └─ right → body_block (AstBlock)
```

**LL(1) Compliance**:
- First set: {"while"}
- Deterministic: "while" keyword uniquely identifies while loop

#### Do-While Loop Syntax

```
do_while_statement:
    "do" block "while" "(" condition ")" ";"

Where:
    - "do", "while" are keywords (TT_KEY)
    - block is "{" statement* "}"
    - condition is any expression
    - Note: Semicolon required after closing parenthesis
    
Examples:
    // Simple do-while
    do {
        x = x + 1;
    } while (x < 10);
    
    // Do-while with break
    do {
        if (invalid) {
            break;
        }
        process();
    } while (shouldContinue);
```

**Parsing Steps**:
1. Expect "do" keyword
2. Parse body block
3. Expect "while" keyword
4. Expect "(" symbol
5. Parse condition expression
6. Expect ")" symbol
7. Expect ";" symbol (important: do-while ends with semicolon)
8. Construct AstDoWhile node with:
   - Body block in `left`
   - Condition in `right`

**AST Structure**:
```
AstDoWhile
  ├─ left → body_block (AstBlock)
  └─ right → condition_expression
```

**Key Difference from While**:
- Body executes **at least once** (condition checked after body)
- Requires semicolon after condition

**LL(1) Compliance**:
- First set: {"do"}
- Deterministic: "do" keyword uniquely identifies do-while loop

#### For Loop Syntax

```
for_statement:
    "for" "(" initializer ";" condition ";" increment ")" block

Where:
    - "for" is keyword (TT_KEY)
    - initializer is variable declaration or expression or empty
    - condition is expression or empty
    - increment is expression or empty
    - All three parts separated by semicolons
    - block is "{" statement* "}"
    
Examples:
    // Classic for loop (loop variables use camelCase)
    for (let i = 0; i < 10; i = i + 1) {
        Print(i);  // Function (PascalCase)
    }
    
    // Empty initializer (variable declared outside)
    let index = 0;  // Local variable (camelCase)
    for (; index < 10; index = index + 1) {
        Print(index);
    }
    
    // Infinite loop (all parts empty)
    for (;;) {
        if (isDone) break;  // Local variable (camelCase)
    }
    
    // Multiple variables in initializer
    for (let i = 0, j = 10; i < j; i = i + 1, j = j - 1) {
        ProcessPair(i, j);  // Function (PascalCase)
    }
```

**Parsing Steps**:
1. Expect "for" keyword
2. Expect "(" symbol
3. Parse initializer:
   - If current token is "let", parse variable declaration
   - Else if not ";", parse expression
   - Else leave null (empty initializer)
4. Expect ";" symbol
5. Parse condition:
   - If not ";", parse expression
   - Else leave null (empty condition = infinite loop)
6. Expect ";" symbol
7. Parse increment:
   - If not ")", parse expression
   - Else leave null (empty increment)
8. Expect ")" symbol
9. Parse body block
10. Construct AstFor node with linked parts

**AST Structure**:
```
AstFor
  ├─ left → AstBlock containing:
  │         ├─ initializer (AstVarDecl or AstExprStmt or null)
  │         ├─ condition (expression or null)
  │         └─ increment (expression or null)
  └─ right → body_block
```

**Alternative AST Structure** (cleaner):
```
AstFor
  ├─ left → initializer (statement or null)
  ├─ right → body_block
  └─ next → AstForParts
            ├─ left → condition (expression or null)
            └─ right → increment (expression or null)
```

**LL(1) Compliance**:
- First set: {"for"}
- Deterministic: "for" keyword uniquely identifies for loop
- Semicolons serve as synchronization points

#### Break and Continue Statements

```
break_statement:
    "break" ";"

continue_statement:
    "continue" ";"
    
Examples:
    // Break out of loop
    while (true) {
        if (x > 10) {
            break;  // Exit while loop
        }
        x = x + 1;
    }
    
    // Skip rest of iteration
    for (let i = 0; i < 10; i = i + 1) {
        if (i == 5) {
            continue;  // Skip to next iteration
        }
        print(i);
    }
```

**Parsing Steps (break)**:
1. Expect "break" keyword
2. Expect ";" symbol
3. Construct AstBreak node

**Parsing Steps (continue)**:
1. Expect "continue" keyword
2. Expect ";" symbol
3. Construct AstContinue node

**Semantic Validation** (not parser's job, but important):
- Break/continue only valid inside loops
- Runtime or semantic analyzer should enforce this

**LL(1) Compliance**:
- First sets: {"break"}, {"continue"}
- Deterministic: Keywords uniquely identify statements

---

### Function Declaration and Call

#### Function Definition Syntax

```
function_declaration:
    "fn" identifier "(" parameters ")" block

parameters:
    (identifier ("," identifier)*)?

Where:
    - "fn" is a keyword (TT_KEY)
    - identifier is function name (TT_IDN)
    - "(" ")" are symbols (TT_SYM)
    - parameters is comma-separated parameter list (identifiers)
    - block is "{" statement* "}"
    
Examples:
    // No parameters (function name PascalCase)
    fn Greet() {
        Print("Hello!");  // Function (PascalCase)
    }
    
    // With parameters (function PascalCase, parameters camelCase)
    fn Add(x, y) {
        return x + y;
    }
    
    // Multiple statements
    fn Factorial(n) {
        // Parameter (camelCase)
        if (n <= 1) {
            return 1;
        }
        return n * Factorial(n - 1);
    }
    
    // Nested functions (if supported)
    fn Outer() {
        fn Inner() {
            return 42;
        }
        return Inner();
    }
```

**Parsing Steps**:
1. Expect "fn" keyword
2. Parse function name (identifier - TT_IDN)
3. Expect "(" symbol
4. Parse parameter list:
   - If ")", empty parameters
   - Otherwise, parse first identifier
   - While ",", parse additional identifiers
5. Expect ")" symbol
6. Parse function body block
7. Construct AstFunc node with:
   - Function name in `value`
   - First parameter in `left` (linked list via `next`)
   - Body block in `right`

**AST Structure**:
```
AstFunc
  ├─ value → function_name
  ├─ left → param1
  │         └─ next → param2
  │                   └─ next → param3
  │                             └─ next → null
  └─ right → body_block (AstBlock)
```

**LL(1) Compliance**:
- First set: {"fn"}
- Deterministic: "fn" keyword uniquely identifies function declaration

#### Function Call Syntax

```
function_call:
    identifier "(" arguments ")"

arguments:
    (expression ("," expression)*)?

Where:
    - identifier is function name (TT_IDN)
    - "(" ")" are symbols (TT_SYM)
    - arguments is comma-separated expression list
    
Examples:
    // No arguments
    greet()
    
    // With arguments
    add(5, 3)
    
    // Expression arguments
    multiply(x + 1, y * 2)
    
    // Nested calls
    pow(add(1, 2), sub(5, 3))
    
    // Method calls (member access)
    object.method(arg1, arg2)
```

**Parsing Steps** (as postfix operator):
1. Parse identifier in `primary()`
2. In `postfix()`, check for "(" after identifier
3. If "(", parse as function call:
   - Consume "(" symbol
   - If ")", empty arguments
   - Otherwise, parse first expression
   - While ",", parse additional expressions
   - Expect ")" symbol
4. Construct AstCall node with:
   - Function name in `left` (the identifier node)
   - First argument in `right` (linked list via `next`)

**AST Structure**:
```
AstCall
  ├─ left → function_identifier (AstIdn)
  └─ right → arg1
             └─ next → arg2
                       └─ next → arg3
                                 └─ next → null
```

**Parsing Location**:
- Function calls are **postfix operators** (like array indexing)
- Parse in `postfix()` function after primary expressions
- Enables chained calls: `getObject().getMethod()()`

**Extended Postfix Pattern** (Complete Implementation):
```zig
private func postfix(self: *Parser) -> *Ast {
    var base = primary();
    if (base == null) return null;
    
    // Handle all postfix operators in a single loop
    while (true) {
        if (peek() == TT_SYM) {
            const sym = current_token.Value;
            
            if (sym == "[") {
                // Array/dictionary indexing: base[index]
                const pos = current_token.Position;
                advance();  // consume '['
                
                var index = expression();
                if (index == null) {
                    reportError("Expected expression inside brackets", pos);
                    return null;
                }
                
                if (!expectValue("]")) return null;
                
                var node = GenericAstInit(AstIndex, pos);
                node.left = base;
                node.right = index;
                base = node;
            } 
            else if (sym == "(") {
                // Function call: base(args)
                base = functionCall(base);
                if (base == null) return null;
            } 
            else if (sym == ".") {
                // Member access: base.member
                const pos = current_token.Position;
                advance();  // consume '.'
                
                if (peek() != TT_IDN) {
                    reportError("Expected identifier after '.'", pos);
                    return null;
                }
                
                const member_name = current_token.Value;
                advance();
                
                var node = GenericAstInit(AstMember, pos);
                node.left = base;
                node.Str = member_name;
                base = node;
            } 
            else {
                break;  // No more postfix operators
            }
        } else {
            break;  // Not a symbol, no postfix operator
        }
    }
    
    return base;
}

// Helper function for parsing function calls
private func functionCall(self: *Parser, callee: *Ast) -> *Ast {
    const pos = current_token.Position;
    advance();  // consume '('
    
    var call_node = GenericAstInit(AstCall, pos);
    call_node.left = callee;  // Function being called
    
    // Check for empty argument list
    if (peek() == TT_SYM and current_token.Value == ")") {
        advance();  // consume ')'
        return call_node;
    }
    
    // Parse first argument
    var first_arg = expression();
    if (first_arg == null) {
        reportError("Expected expression in argument list", pos);
        return null;
    }
    
    var current_arg = first_arg;
    call_node.right = first_arg;  // First argument
    
    // Parse remaining arguments
    while (peek() == TT_SYM and current_token.Value == ",") {
        advance();  // consume ','
        
        // Allow trailing comma
        if (peek() == TT_SYM and current_token.Value == ")") {
            break;
        }
        
        var arg = expression();
        if (arg == null) {
            reportError("Expected expression after comma", pos);
            return null;
        }
        
        current_arg.next = arg;  // Link arguments
        current_arg = arg;
    }
    
    if (!expectValue(")")) return null;
    
    return call_node;
}
```

**Postfix Chaining Examples**:
```javascript
// Array indexing
arr[0]              // AstIndex(arr, 0)

// Function call
func()              // AstCall(func, null)
func(1, 2)          // AstCall(func, args)

// Member access
obj.field           // AstMember(obj, "field")

// Chained indexing
matrix[i][j]        // AstIndex(AstIndex(matrix, i), j)

// Method call
obj.method()        // AstCall(AstMember(obj, "method"), null)

// Complex chaining
obj.arr[0].func(x)  // AstCall(AstMember(AstIndex(AstMember(obj, "arr"), 0), "func"), x)
```

**LL(1) Compliance**:
- First set: Inherits from primary expressions (identifiers, etc.)
- Call distinguished by "(" after identifier
- Deterministic: Lookahead "(" after identifier = call

---

### Object-Oriented Features

#### Class Definition Syntax

```
class_declaration:
    "class" identifier "{" class_members "}"

class_members:
    (member_declaration | method_declaration)*

member_declaration:
    identifier "=" expression ";"

method_declaration:
    "fn" identifier "(" parameters ")" block

Where:
    - "class" is keyword (TT_KEY)
    - identifier is class name (TT_IDN)
    - members can be fields or methods
    
Examples:
    // Simple class (class name PascalCase)
    class Point {
        // Attributes (PascalCase)
        X = 0;
        Y = 0;
        
        // Method (PascalCase) with parameters (camelCase)
        fn Initialize(xVal, yVal) {
            X = xVal;
            Y = yVal;
        }
        
        // Method (PascalCase)
        fn CalculateDistance() {
            return (X * X + Y * Y);
        }
    }
    
    // Class with constructor
    class Person {
        // Attributes (PascalCase)
        Name = "";
        Age = 0;
        
        // Method (PascalCase) with parameters (camelCase)
        fn Constructor(name, age) {
            Name = name;
            Age = age;
        }
        
        // Method (PascalCase)
        fn Greet() {
            Print("Hello, I'm " + Name);
        }
    }
    
    // Empty class
    class Empty {
    }
```

**Parsing Steps**:
1. Expect "class" keyword
2. Parse class name (identifier)
3. Expect "{" symbol
4. Parse class members:
   - While not "}", parse member or method:
     - If "fn", parse method (function declaration)
     - Otherwise, parse field (identifier "=" expression ";")
   - Link members via `next` pointer
5. Expect "}" symbol
6. Construct AstClass node with:
   - Class name in `value`
   - First member in `left` (linked list via `next`)

**AST Structure**:
```
AstClass
  ├─ value → class_name
  └─ left → member1 (AstVarDecl or AstMethod)
            └─ next → member2
                      └─ next → member3
                                └─ next → null
```

**Member Access Syntax**:
```
member_access:
    expression "." identifier

Examples:
    point.x
    point.y
    person.name
    object.method()
```

**Parsing Steps** (as postfix operator):
1. In `postfix()`, check for "." after base expression
2. If ".", parse member access:
   - Consume "." symbol
   - Parse member name (identifier)
   - Construct AstMember node
3. Link in postfix chain

**AST Structure**:
```
AstMember
  ├─ left → object_expression
  └─ value → member_name
```

**LL(1) Compliance**:
- First set: {"class"}
- Deterministic: "class" keyword uniquely identifies class declaration
- Member vs method: Distinguished by "fn" keyword

#### Enum Definition Syntax

```
enum_declaration:
    "enum" identifier "{" enum_values "}"

enum_values:
    identifier ("," identifier)* ","?

Where:
    - "enum" is keyword (TT_KEY)
    - identifier is enum name
    - enum_values is comma-separated list of identifiers
    - trailing comma is optional
    
Examples:
    // Simple enum
    enum Color {
        Red,
        Green,
        Blue
    }
    
    // With trailing comma
    enum Status {
        Pending,
        Active,
        Completed,
    }
    
    // Single value
    enum Singleton {
        Only
    }
    
    // Usage
    let color = Color.Red;
    if (status == Status.Active) {
        // ...
    }
```

**Parsing Steps**:
1. Expect "enum" keyword
2. Parse enum name (identifier)
3. Expect "{" symbol
4. Parse enum values:
   - Parse first identifier
   - While ",", parse additional identifiers
   - Allow trailing comma before "}"
5. Expect "}" symbol
6. Construct AstEnum node with:
   - Enum name in `value`
   - First value in `left` (linked list via `next`)

**AST Structure**:
```
AstEnum
  ├─ value → enum_name
  └─ left → value1 (AstIdn)
            └─ next → value2
                      └─ next → value3
                                └─ next → null
```

**Enum Value Access**:
- Use member access syntax: `EnumName.ValueName`
- Parsed as `AstMember(AstIdn("EnumName"), "ValueName")`
- Semantic analyzer validates enum names and values

**LL(1) Compliance**:
- First set: {"enum"}
- Deterministic: "enum" keyword uniquely identifies enum declaration

---

### Variable Declaration and Assignment

#### Variable Declaration Syntax

```
variable_declaration:
    "let" identifier ("=" expression)? ";"

Where:
    - "let" is keyword (TT_KEY)
    - identifier is variable name (TT_IDN)
    - initialization is optional
    
Examples:
    // Without initialization (local variables use camelCase)
    let userName;
    
    // With initialization
    let itemCount = 10;
    
    // Expression initialization
    let totalPrice = basePrice + tax * 2;
    
    // Array/dict initialization
    let numbers = [1, 2, 3];
    let config = {"key": "value"};
    
    // Global variables (PascalCase)
    let MaxConnections = 100;
    let DefaultTimeout = 5000;
```

**Parsing Steps**:
1. Expect "let" keyword
2. Parse variable name (identifier)
3. Check for "=" symbol:
   - If "=", consume it and parse initialization expression
   - Otherwise, initializer is null
4. Expect ";" symbol
5. Construct AstVarDecl node with:
   - Variable name in `value`
   - Initializer expression in `right` (or null)

**AST Structure**:
```
AstVarDecl
  ├─ value → variable_name
  └─ right → initializer_expression (or null)
```

#### Assignment Statement Syntax

```
assignment:
    lvalue "=" expression ";"

lvalue:
    identifier | array_index | member_access

Where:
    - lvalue is assignable expression
    - "=" is assignment operator (TT_SYM)
    
Examples:
    // Simple assignment
    x = 10;
    
    // Array element assignment
    arr[0] = 5;
    
    // Member assignment
    point.x = 100;
    
    // Nested assignment
    matrix[i][j] = value;
    
    // Chained assignment (if supported)
    x = y = z = 0;
```

**Parsing Strategy**:
- Assignment has **lower precedence** than all operators
- Parsed as part of statement parsing, not expression parsing
- Or: Add assignment as lowest precedence expression level

**Option 1: Statement-level parsing**:
```zig
private func expressionStatement(self: *Parser) -> *Ast {
    var expr = expression();
    
    // Check for assignment
    if (peek() == TT_SYM and current_token.Value == "=") {
        advance();  // consume "="
        var value = expression();
        
        var assign_node = GenericAstInit(AstAssign, expr.Position);
        assign_node.left = expr;   // lvalue
        assign_node.right = value; // rvalue
        
        expect(TT_SYM, ";");
        return assign_node;
    }
    
    expect(TT_SYM, ";");
    return expr;
}
```

**Option 2: Expression-level parsing** (add to precedence hierarchy):
```zig
private func assignment(self: *Parser) -> *Ast {
    var left = logicalOr();  // Current top-level expression
    
    if (peek() == TT_SYM and current_token.Value == "=") {
        advance();  // consume "="
        var right = assignment();  // Right-associative
        
        var node = GenericAstInit(AstAssign, left.Position);
        node.left = left;
        node.right = right;
        return node;
    }
    
    return left;
}

// Update expression() to call assignment():
private func expression(self: *Parser) -> *Ast {
    return assignment();  // Now lowest precedence
}
```

**AST Structure**:
```
AstAssign
  ├─ left → lvalue (AstIdn, AstIndex, or AstMember)
  └─ right → rvalue (expression)
```

**Validation** (semantic analysis, not parser):
- Ensure lvalue is assignable (identifier, index, member)
- Reject assignments to literals or function calls

**LL(1) Compliance**:
- Requires lookahead after expression to check for "="
- Still LL(1): Parse expression, then check for "="
- Decision point is deterministic

#### Array Literal Syntax
```
array:
    "[" elements "]"

Where:
    - "[" "]" are symbols (TT_SYM)
    - elements is comma-separated list of expressions
    
Examples:
    []                    // Empty array
    [1, 2, 3]            // Integer array
    [1.5, 2.7, 3.14]     // Float array
    ["hello", "world"]   // String array
    [1, "mixed", true]   // Mixed-type array
    [x, y + 1, fn()]     // Expression array
    [[1, 2], [3, 4]]     // Nested arrays
```

**Parsing Steps**:
1. Expect "[" symbol
2. If next token is "]", return empty array
3. Otherwise, parse first expression
4. While current token is ",":
   - Consume comma
   - Parse next expression
   - Link to previous element using Next pointer
5. Expect "]" symbol
6. Construct AstArray node with first element as child

**AST Structure**:
```
AstArray
  └─ Next → element1
            └─ Next → element2
                      └─ Next → element3
                                └─ Next → null
```

#### Dictionary/Object Literal Syntax
```
dictionary:
    "{" entries "}"

entry:
    key ":" value

Where:
    - "{" "}" are symbols (TT_SYM)
    - key is string literal or identifier
    - ":" is symbol (TT_SYM)
    - value is any expression
    - entries is comma-separated list of key:value pairs
    
Examples:
    {}                          // Empty dictionary
    {"name": "John"}           // Single entry
    {"x": 10, "y": 20}         // Multiple entries
    {"key": [1, 2, 3]}         // Array value
    {"nested": {"a": 1}}       // Nested dictionary
    {age: 25}                  // Identifier as key (syntactic sugar)
    {"fn": fn() { return 42 }} // Function value
```

**Parsing Steps**:
1. Expect "{" symbol
2. If next token is "}", return empty dictionary
3. Otherwise, parse first key-value pair:
   - Parse key (string or identifier)
   - Expect ":" symbol
   - Parse value expression
   - Create AstKeyValue node
4. While current token is ",":
   - Consume comma
   - Parse next key-value pair
   - Link to previous entry using Next pointer
5. Expect "}" symbol
6. Construct AstDict node with first entry as child

**AST Structure**:
```
AstDict
  └─ Next → AstKeyValue(key1, value1)
            └─ Next → AstKeyValue(key2, value2)
                      └─ Next → AstKeyValue(key3, value3)
                                └─ Next → null
```

**Key Representation**: Store key as `Str` field in AstKeyValue node.

#### Array/Dictionary Indexing Syntax
```
indexing:
    expression "[" index "]"

Where:
    - expression is array or dictionary
    - "[" "]" are symbols (TT_SYM)
    - index is integer (for arrays) or string (for dictionaries)
    
Examples:
    arr[0]              // Array indexing by integer
    arr[i + 1]          // Array indexing by expression
    dict["key"]         // Dictionary indexing by string
    dict[keyVar]        // Dictionary indexing by variable
    matrix[i][j]        // Nested indexing (2D array)
    arr[0] = 42         // Index assignment (lvalue)
```

**Parsing Steps** (as postfix operator):
1. Parse primary expression (array/dict)
2. While current token is "[":
   - Consume "[" symbol
   - Parse index expression
   - Expect "]" symbol
   - Create AstIndex node with:
     - Left child: container expression
     - Right child (Next): index expression
3. Return final AstIndex node

**AST Structure**:
```
AstIndex
  ├─ Str → (container expression stored or linked)
  └─ Next → index_expression
```

**Note**: For assignment operations like `arr[0] = 42`, the AstIndex node becomes the left-hand side of an assignment operator.

### Parser Implementation Guidelines (LL(1) Focus)

#### Core LL(1) Principles

1. **Single Token Lookahead**: 
   - Maintain `current_token` as the lookahead
   - Use `peek()` to inspect token type without consuming
   - Make ALL parsing decisions based on this one token
   - Never require more than one token of lookahead

2. **No Left Recursion**:
   - Transform: `E → E + T` into `E → T (+ T)*`
   - Use **iteration** (while loops) instead of recursion for operators
   - Left-associative operators naturally handled by iteration
   - Example: `a + b + c` builds as `((a) + b) + c`

3. **Left Factoring**:
   - Extract common prefixes in alternative productions
   - Bad: `S → if E then S else S | if E then S`
   - Good: `S → if E then S (else S)?`
   - Ensures LL(1) determinism

4. **Disjoint First Sets**:
   - Each alternative production must start with different token
   - `primary → TT_IDN | TT_INT | "[" | "{" | "("` ✓ (all disjoint)
   - Use switch/case on token type for O(1) decision

#### Implementation Best Practices

5. **Token Management**:
   - `advance()`: Consume current token, fetch next
   - `peek()`: Return type of current token (non-destructive)
   - `expect(type)`: Verify and consume expected token
   - `match(types)`: Check if current token in list
   
6. **Error Recovery (Panic Mode)**:
   - When parse fails, report error with position
   - Call `synchronize()` to skip to safe boundary
   - Sync points: `;`, `}`, `EOF`, statement keywords
   - Continue parsing to collect multiple errors
   - Don't cascade errors from single mistake

7. **Precedence via Layered Functions**:
   - Each precedence level = one function
   - Higher precedence = called by lower precedence
   - Call chain: `expression()` → `logicalOr()` → ... → `primary()`
   - Operators bind according to call depth

8. **Iteration Over Recursion for Operators**:
   - Use while loops for `(op term)*` patterns
   - Avoids left recursion
   - Natural left-associativity
   - Better stack usage (no deep recursion)

9. **Memory Management**:
   - Use arena allocator for AST nodes
   - All nodes allocated from same pool
   - Deallocate entire tree at once after evaluation
   - No individual node freeing during parsing

10. **Position Tracking**:
    - Store position for every AST node
    - Use `Position.Merge()` for compound expressions
    - Essential for runtime error reporting
    - Preserve source location through entire pipeline

11. **Collections (Arrays/Dicts)**:
    - Parse recursively to support arbitrary nesting
    - Handle empty collections: check for `]` or `}` immediately
    - Optional trailing comma support
    - Link elements via `Next` pointer
    - Validate matching delimiters

12. **Postfix Operators (Indexing)**:
    - Parse after primary expression
    - Highest precedence (binds tightest)
    - Use while loop for chaining: `arr[i][j][k]`
    - Each `[]` creates new `AstIndex` node wrapping previous

#### LL(1) Parsing Workflow

```
For each parsing function:
  1. Peek at current token type
  2. Use switch/if to select production (LL(1) decision)
  3. Consume tokens with advance()
  4. Call sub-parsers recursively
  5. Build and return AST node
  6. If error, report and return null
```

#### Common LL(1) Pitfalls to Avoid

❌ **Don't**: Look ahead more than one token  
✓ **Do**: Redesign grammar if you need more lookahead

❌ **Don't**: Use left-recursive grammar rules  
✓ **Do**: Transform to iteration or right-recursion

❌ **Don't**: Backtrack if production fails  
✓ **Do**: Make deterministic choice with one token

❌ **Don't**: Have ambiguous grammar  
✓ **Do**: Ensure each lookahead uniquely determines production

❌ **Don't**: Parse without error recovery  
✓ **Do**: Implement synchronization for graceful error handling

### LL(1) Parser Flow

```
Token Stream (with current_token lookahead)
       ↓
   Parse() entry point
       ↓
   expression() → logicalOr() → logicalAnd() → ... → primary()
       ↓                                                ↓
   [LL(1) Decision]                           [LL(1) Decision]
   peek() current_token                       switch(peek())
       ↓                                                ↓
   Operator precedence                        literals / identifiers / 
   functions (11 levels)                      array / dict / ( ) / function
       ↓                                                ↓
   postfix() for indexing                     GenericAstInit()
       ↓                                                ↓
   Build operator AST node                    Build leaf/complex AST node
       ↓                                                ↓
       └────────────────────────────────────────────────┘
                              ↓
                    Complete AST Tree
                              ↓
                    Return from Parse()
```

**Key LL(1) Characteristics**:
- **Single Pass**: Token stream read left-to-right once
- **No Backtracking**: Each decision final (based on lookahead)
- **Recursive Descent**: Functions call each other following grammar
- **Deterministic**: `peek()` uniquely determines next action
- **Linear Time**: O(n) where n = number of tokens

The parser is the second stage in the interpreter pipeline and builds the syntactic structure using **LL(1) recursive descent** methodology.

### Additional Parsing Considerations

#### Array & Dictionary Edge Cases
1. **Trailing Commas**: Decide whether to allow `[1, 2, 3,]` or `{"a": 1,}`
2. **Empty Elements**: Handle cases like `[1, , 3]` (syntax error or sparse array?)
3. **Computed Keys**: Support `{[expr]: value}` for dynamic dictionary keys
4. **Spread Operator**: Consider `[...arr1, ...arr2]` for array spreading (future feature)

#### Indexing Considerations
1. **Negative Indices**: Support Python-style `arr[-1]` for last element?
2. **Slice Syntax**: Support `arr[start:end]` for array slicing? (future feature)
3. **Multi-dimensional**: Ensure nested indexing `matrix[i][j]` works correctly
4. **Bounds Checking**: Defer to runtime, but parser should handle syntax correctly

#### Memory Management for Collections
- **Array Allocation**: Use dynamic array (ArrayList) to store elements during runtime
- **Dictionary Implementation**: Use hash map for O(1) key lookup
- **AST Representation**: Linked list during parsing, convert to efficient structures during evaluation
- **String Keys**: Intern string keys for dictionaries to reduce memory and speed up comparisons

---

## Comprehensive Syntax Examples

### Arrays and Dictionaries in Practice

```javascript
// Simple arrays (local variables use camelCase)
let numbers = [1, 2, 3, 4, 5];
let names = ["Alice", "Bob", "Charlie"];
let mixed = [1, "hello", true, null];
let empty = [];

// Array access (local variables use camelCase)
let first = numbers[0];        // 1
let last = numbers[4];         // 5
let dynamic = numbers[i + 1];  // expression as index

// Nested arrays (matrices)
let matrix = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
];
let value = matrix[1][2];      // 6 (row 1, col 2)

// Simple dictionaries
let person = {
    "name": "John",
    "age": 30,
    "active": true
};

// Dictionary access
let name = person["name"];     // "John"
let age = person["age"];       // 30

// Nested dictionaries
let config = {
    "database": {
        "host": "localhost",
        "port": 5432
    },
    "cache": {
        "enabled": true,
        "ttl": 3600
    }
};
let host = config["database"]["host"];  // "localhost"

// Mixed structures
let users = [
    {"id": 1, "name": "Alice"},
    {"id": 2, "name": "Bob"},
    {"id": 3, "name": "Charlie"}
];
let firstUser = users[0]["name"];  // "Alice"

// Dictionary with array values
let teams = {
    "frontend": ["Alice", "Bob"],
    "backend": ["Charlie", "Dave"],
    "devops": ["Eve"]
};
let frontendLead = teams["frontend"][0];  // "Alice"

// Complex expressions
let data = {
    "items": [1, 2, 3, 4, 5],
    "metadata": {
        "count": 5,
        "total": 15
    }
};
let average = data["metadata"]["total"] / data["metadata"]["count"];  // 3
let doubled = data["items"][2] * 2;  // 6 (3 * 2)

// Functions (PascalCase) with parameters (camelCase)
fn ProcessArray(arr) {
    return arr[0] + arr[1];
}

fn ProcessDict(dict) {
    return dict["x"] * dict["y"];
}

// Local variables (camelCase)
let result1 = ProcessArray([10, 20]);  // 30
let result2 = ProcessDict({"x": 5, "y": 6});  // 30

// Functions (PascalCase)
fn CreatePoint(x, y) {
    // Parameters (camelCase)
    return {"x": x, "y": y};
}

fn CreateRange(start, end) {
    // Parameters (camelCase)
    return [start, start + 1, start + 2, end];
}

let point = CreatePoint(10, 20);
let range = CreateRange(1, 4);  // [1, 2, 3, 4]
```

### Parsing Challenges and Solutions

#### Challenge 1: Disambiguating `{` Token
The `{` symbol can represent:
- Dictionary literal: `{"key": value}`
- Block/function body: `{ statement; }`

**Solution**: Look ahead at next token:
- If next is `}` or string/identifier followed by `:`, parse as dictionary
- Otherwise, parse as block

#### Challenge 2: Empty Collections
```javascript
let emptyArray = [];
let emptyDict = {};
```

**Solution**: After parsing opening delimiter, check for immediate closing delimiter before attempting to parse elements.

#### Challenge 3: Trailing Commas
```javascript
let arr = [1, 2, 3,];      // Trailing comma
let obj = {"a": 1, "b": 2,};
```

**Solution**: After parsing element and comma, check if next token is closing delimiter. If yes, accept trailing comma (permissive) or error (strict).

#### Challenge 4: Nested Access with Operators
```javascript
let result = matrix[i + 1][j * 2] + offset;
```

**Parsing Order**:
1. Parse `matrix` (primary)
2. Parse `[i + 1]` (postfix indexing)
3. Parse `[j * 2]` (postfix indexing)
4. Parse `+ offset` (binary addition)

Result AST: `Add(Index(Index(matrix, Add(i, 1)), Mul(j, 2)), offset)`

---

---

### Block Statement Parsing

Block statements group multiple statements together and create scope boundaries.

```
block:
    "{" statement* "}"

Where:
    - "{" "}" are symbols (TT_SYM)
    - Contains zero or more statements
    - Creates new lexical scope (for semantic analyzer)
```

**Parsing Steps**:
1. Expect "{" symbol
2. Create AstBlock node
3. While current token is not "}":
   - Parse statement
   - Link to previous statement via `next`
4. Expect "}" symbol
5. Return AstBlock with first statement in `left`

**AST Structure**:
```
AstBlock
  └─ left → stmt1
            └─ next → stmt2
                      └─ next → stmt3
                                └─ next → null
```

**Implementation**:
```zig
private func block(self: *Parser) -> *Ast {
    const position = current_token.Position;
    advance();  // consume "{"
    
    var block_node = GenericAstInit(AstBlock, position);
    var first_stmt: ?*Ast = null;
    var current_stmt: ?*Ast = null;
    
    while (peek() != TT_SYM or current_token.Value != "}") {
        var stmt = statement();
        if (stmt == null) {
            // Error recovery: skip to next statement or end of block
            synchronize();
            continue;
        }
        
        if (first_stmt == null) {
            first_stmt = stmt;
            current_stmt = stmt;
        } else {
            current_stmt.?.next = stmt;
            current_stmt = stmt;
        }
    }
    
    advance();  // consume "}"
    block_node.left = first_stmt;
    return block_node;
}
```

---

### Statement Parsing Implementation

The `statement()` function is the dispatcher for all statement types:

```zig
private func statement(self: *Parser) -> *Ast {
    const token_type = peek();
    
    // Statement dispatch based on lookahead token
    if (token_type == TT_KEY) {
        const keyword = current_token.Value;
        
        if (keyword == "let") {
            return varDeclaration();
        } else if (keyword == "if") {
            return ifStatement();
        } else if (keyword == "while") {
            return whileStatement();
        } else if (keyword == "do") {
            return doWhileStatement();
        } else if (keyword == "for") {
            return forStatement();
        } else if (keyword == "return") {
            return returnStatement();
        } else if (keyword == "break") {
            return breakStatement();
        } else if (keyword == "continue") {
            return continueStatement();
        } else if (keyword == "fn") {
            return functionDeclaration();
        } else if (keyword == "class") {
            return classDeclaration();
        } else if (keyword == "enum") {
            return enumDeclaration();
        }
    } else if (token_type == TT_SYM and current_token.Value == "{") {
        return block();
    }
    
    // Default: expression statement
    return expressionStatement();
}
```

**LL(1) Decision Table for Statements**:

| Lookahead Token | Statement Type |
|-----------------|----------------|
| `"let"` | Variable declaration |
| `"if"` | If statement |
| `"while"` | While loop |
| `"do"` | Do-while loop |
| `"for"` | For loop |
| `"return"` | Return statement |
| `"break"` | Break statement |
| `"continue"` | Continue statement |
| `"fn"` | Function declaration |
| `"class"` | Class declaration |
| `"enum"` | Enum declaration |
| `"{"` | Block statement |
| Other | Expression statement |

---

### Complete LL(1) Grammar for Language-X

Updated grammar including all features:

```ebnf
// ===== PROGRAM STRUCTURE =====
program              → statement* EOF

statement            → varDeclaration
                     | functionDeclaration
                     | classDeclaration
                     | enumDeclaration
                     | ifStatement
                     | whileStatement
                     | doWhileStatement
                     | forStatement
                     | returnStatement
                     | breakStatement
                     | continueStatement
                     | block
                     | expressionStatement
                     ;

// ===== DECLARATIONS =====
varDeclaration       → "let" TT_IDN ( "=" expression )? ";"

functionDeclaration  → "fn" TT_IDN "(" parameters ")" block

classDeclaration     → "class" TT_IDN "{" classMember* "}"

classMember          → functionDeclaration
                     | TT_IDN "=" expression ";"
                     ;

enumDeclaration      → "enum" TT_IDN "{" enumValue ( "," enumValue )* ","? "}"

enumValue            → TT_IDN

parameters           → ( TT_IDN ( "," TT_IDN )* )?

// ===== CONTROL FLOW =====
ifStatement          → "if" "(" expression ")" block ( "else" ( ifStatement | block ) )?

whileStatement       → "while" "(" expression ")" block

doWhileStatement     → "do" block "while" "(" expression ")" ";"

forStatement         → "for" "(" forInit ";" forCondition ";" forIncrement ")" block

forInit              → varDeclaration | expression | ε

forCondition         → expression | ε

forIncrement         → expression | ε

returnStatement      → "return" expression? ";"

breakStatement       → "break" ";"

continueStatement    → "continue" ";"

block                → "{" statement* "}"

expressionStatement  → expression ";"

// ===== EXPRESSIONS =====
// Expression hierarchy (precedence from low to high)

expression           → assignment

assignment           → logicalOr ( "=" assignment )?

logicalOr            → logicalAnd ( "||" logicalAnd )*

logicalAnd           → bitwiseOr ( "&&" bitwiseOr )*

bitwiseOr            → bitwiseXor ( "|" bitwiseXor )*

bitwiseXor           → bitwiseAnd ( "^" bitwiseAnd )*

bitwiseAnd           → equality ( "&" equality )*

equality             → comparison ( ( "==" | "!=" ) comparison )*

comparison           → shift ( ( "<" | "<=" | ">" | ">=" ) shift )*

shift                → additive ( ( "<<" | ">>" ) additive )*

additive             → multiplicative ( ( "+" | "-" ) multiplicative )*

multiplicative       → postfix ( ( "*" | "/" | "%" ) postfix )*

postfix              → primary ( postfixOp )*

postfixOp            → "[" expression "]"           // Array indexing
                     | "(" arguments ")"            // Function call
                     | "." TT_IDN                   // Member access
                     ;

primary              → TT_IDN
                     | TT_INT
                     | TT_NUM
                     | TT_STR
                     | "true"
                     | "false"
                     | "null"
                     | array
                     | dictionary
                     | "(" expression ")"
                     ;

// ===== DATA STRUCTURES =====
array                → "[" ( expression ( "," expression )* ","? )? "]"

dictionary           → "{" ( keyValue ( "," keyValue )* ","? )? "}"

keyValue             → ( TT_STR | TT_IDN ) ":" expression

arguments            → ( expression ( "," expression )* )?
```

**Grammar Properties**:
- ✓ **LL(1) compliant**: Single token lookahead sufficient for all decisions
- ✓ **No left recursion**: All operators use iteration patterns
- ✓ **Left-factored**: No ambiguous prefixes
- ✓ **Disjoint first sets**: All alternatives distinguishable by lookahead
- ✓ **Deterministic**: No backtracking needed
- ✓ **Complete**: Covers all language features

**First Sets** (for LL(1) validation):
```
FIRST(statement) = {"let", "fn", "class", "enum", "if", "while", "do", "for", 
                    "return", "break", "continue", "{", TT_IDN, TT_INT, TT_NUM, 
                    TT_STR, "true", "false", "null", "[", "("}

FIRST(expression) = {TT_IDN, TT_INT, TT_NUM, TT_STR, "true", "false", "null", 
                     "[", "{", "("}

FIRST(primary) = {TT_IDN, TT_INT, TT_NUM, TT_STR, "true", "false", "null", 
                  "[", "{", "("}
```

**Follow Sets** (for LL(1) validation):
```
FOLLOW(statement) = {TT_EOF, "}", "let", "fn", "class", "enum", "if", "while", 
                     "do", "for", "return", "break", "continue", "{", TT_IDN, ...}

FOLLOW(expression) = {";", ")", "]", ",", "}", ...}

FOLLOW(block) = FOLLOW(statement) ∪ {"else", "while"}
```

---

### Parser State Machine for Statements

The parser operates as a state machine when parsing statements:

```
┌─────────────────────────────────────────────────────────┐
│                    Parser State Machine                  │
└─────────────────────────────────────────────────────────┘

State: PROGRAM_START
  ↓
  Lookahead: current_token
  ↓
  ┌──────────────────────────────────────┐
  │   Dispatch based on token type       │
  │   (LL(1) decision point)             │
  └──────────────────────────────────────┘
  ↓
  ├─ "let"      → varDeclaration()
  ├─ "fn"       → functionDeclaration()
  ├─ "class"    → classDeclaration()
  ├─ "enum"     → enumDeclaration()
  ├─ "if"       → ifStatement()
  ├─ "while"    → whileStatement()
  ├─ "do"       → doWhileStatement()
  ├─ "for"      → forStatement()
  ├─ "return"   → returnStatement()
  ├─ "break"    → breakStatement()
  ├─ "continue" → continueStatement()
  ├─ "{"        → block()
  └─ other      → expressionStatement()
  ↓
  Each parser function:
    1. Validates expected tokens
    2. Recursively calls sub-parsers
    3. Builds AST node
    4. Returns to caller
  ↓
  Continue until EOF
  ↓
State: PROGRAM_END
```

---

### Implementation Patterns for New Statements

#### Pattern: If Statement Implementation

```zig
private func ifStatement(self: *Parser) -> *Ast {
    const position = current_token.Position;
    advance();  // consume "if"
    
    // Parse condition
    if (!expectValue("(")) return null;
    var condition = expression();
    if (condition == null) {
        reportError("Expected condition in if statement", position);
        return null;
    }
    if (!expectValue(")")) return null;
    
    // Parse then-block
    var then_block = block();
    if (then_block == null) return null;
    
    // Parse optional else clause
    var else_clause: ?*Ast = null;
    if (peek() == TT_KEY and current_token.Value == "else") {
        advance();  // consume "else"
        
        // Check for else-if or else
        if (peek() == TT_KEY and current_token.Value == "if") {
            else_clause = ifStatement();  // Recursive: else-if
        } else {
            else_clause = block();  // Final else
        }
    }
    
    // Build AST node
    var if_node = GenericAstInit(AstIf, position);
    if_node.left = condition;
    if_node.right = then_block;
    if_node.next = else_clause;
    
    return if_node;
}
```

#### Pattern: While Statement Implementation

```zig
private func whileStatement(self: *Parser) -> *Ast {
    const position = current_token.Position;
    advance();  // consume "while"
    
    if (!expectValue("(")) return null;
    
    var condition = expression();
    if (condition == null) {
        reportError("Expected condition in while statement", position);
        return null;
    }
    
    if (!expectValue(")")) return null;
    
    var body = block();
    if (body == null) return null;
    
    var while_node = GenericAstInit(AstWhile, position);
    while_node.left = condition;
    while_node.right = body;
    
    return while_node;
}
```

#### Pattern: For Statement Implementation

```zig
private func forStatement(self: *Parser) -> *Ast {
    const position = current_token.Position;
    advance();  // consume "for"
    
    if (!expectValue("(")) return null;
    
    // Parse initializer (optional)
    var init: ?*Ast = null;
    if (peek() == TT_KEY and current_token.Value == "let") {
        init = varDeclaration();
        // Note: varDeclaration consumes the semicolon
    } else if (!(peek() == TT_SYM and current_token.Value == ";")) {
        init = expression();
        if (!expectValue(";")) return null;
    } else {
        advance();  // consume ";"
    }
    
    // Parse condition (optional)
    var condition: ?*Ast = null;
    if (!(peek() == TT_SYM and current_token.Value == ";")) {
        condition = expression();
    }
    if (!expectValue(";")) return null;
    
    // Parse increment (optional)
    var increment: ?*Ast = null;
    if (!(peek() == TT_SYM and current_token.Value == ")")) {
        increment = expression();
    }
    
    if (!expectValue(")")) return null;
    
    var body = block();
    if (body == null) return null;
    
    // Build for loop node
    var for_node = GenericAstInit(AstFor, position);
    
    // Store loop parts (use auxiliary node or special fields)
    var parts_node = GenericAstInit(AstBlock, position);
    parts_node.left = condition;
    parts_node.right = increment;
    
    for_node.left = init;
    for_node.right = body;
    for_node.next = parts_node;
    
    return for_node;
}
```

#### Pattern: Function Declaration Implementation

```zig
private func functionDeclaration(self: *Parser) -> *Ast {
    const position = current_token.Position;
    advance();  // consume "fn"
    
    // Parse function name
    if (!expect(TT_IDN)) return null;
    const func_name = current_token.Value;
    advance();
    
    if (!expectValue("(")) return null;
    
    // Parse parameters
    var first_param: ?*Ast = null;
    var current_param: ?*Ast = null;
    
    if (!(peek() == TT_SYM and current_token.Value == ")")) {
        // Parse first parameter
        if (!expect(TT_IDN)) return null;
        first_param = GenericAstInit(AstIdn, current_token.Position);
        first_param.?.Str = current_token.Value;
        current_param = first_param;
        advance();
        
        // Parse remaining parameters
        while (peek() == TT_SYM and current_token.Value == ",") {
            advance();  // consume ","
            if (!expect(TT_IDN)) return null;
            
            var param = GenericAstInit(AstIdn, current_token.Position);
            param.Str = current_token.Value;
            
            current_param.?.next = param;
            current_param = param;
            advance();
        }
    }
    
    if (!expectValue(")")) return null;
    
    // Parse function body
    var body = block();
    if (body == null) return null;
    
    // Build function node
    var func_node = GenericAstInit(AstFunc, position);
    func_node.Str = func_name;
    func_node.left = first_param;
    func_node.right = body;
    
    return func_node;
}
```

#### Pattern: Class Declaration Implementation

```zig
private func classDeclaration(self: *Parser) -> *Ast {
    const position = current_token.Position;
    advance();  // consume "class"
    
    // Parse class name
    if (!expect(TT_IDN)) return null;
    const class_name = current_token.Value;
    advance();
    
    if (!expectValue("{")) return null;
    
    // Parse class members
    var first_member: ?*Ast = null;
    var current_member: ?*Ast = null;
    
    while (!(peek() == TT_SYM and current_token.Value == "}")) {
        var member: ?*Ast = null;
        
        // Check for method or field
        if (peek() == TT_KEY and current_token.Value == "fn") {
            member = functionDeclaration();
        } else if (peek() == TT_IDN) {
            // Field declaration: identifier "=" expression ";"
            const field_pos = current_token.Position;
            const field_name = current_token.Value;
            advance();
            
            if (!expectValue("=")) return null;
            
            var init_expr = expression();
            if (init_expr == null) return null;
            
            if (!expectValue(";")) return null;
            
            member = GenericAstInit(AstVarDecl, field_pos);
            member.?.Str = field_name;
            member.?.right = init_expr;
        } else {
            reportError("Expected method or field in class", current_token.Position);
            return null;
        }
        
        // Link member to list
        if (first_member == null) {
            first_member = member;
            current_member = member;
        } else {
            current_member.?.next = member;
            current_member = member;
        }
    }
    
    if (!expectValue("}")) return null;
    
    // Build class node
    var class_node = GenericAstInit(AstClass, position);
    class_node.Str = class_name;
    class_node.left = first_member;
    
    return class_node;
}
```

---

---

### Complete Language Examples

#### Example 1: Comprehensive Program with All Features

```javascript
// Enum definition (PascalCase for enum and values)
enum Status {
    Pending,
    Active,
    Completed
}

// Class definition (PascalCase for class name)
class Task {
    // Class attributes (PascalCase)
    Title = "";
    Status = Status.Pending;
    Priority = 0;
    
    // Methods (PascalCase)
    fn Initialize(taskTitle, taskPriority) {
        // Parameters (camelCase)
        Title = taskTitle;
        Priority = taskPriority;
        Status = Status.Pending;
    }
    
    fn Complete() {
        Status = Status.Completed;
        return true;
    }
    
    fn IsPending() {
        return Status == Status.Pending;
    }
}

// Function definition (PascalCase)
fn CreateTasks(count) {
    // Parameter (camelCase)
    let tasks = [];  // Local variable (camelCase)
    
    // Loop variable (camelCase)
    for (let i = 0; i < count; i = i + 1) {
        let task = Task;  // Local variable (camelCase)
        task.Initialize("Task " + i, i);
        tasks[i] = task;
    }
    
    return tasks;
}

// Function (PascalCase)
fn ProcessTasks(tasks) {
    // Parameter (camelCase)
    let completed = 0;  // Local variable (camelCase)
    let i = 0;
    
    while (i < tasks.length) {
        let task = tasks[i];  // Local variable (camelCase)
        
        if (task.IsPending()) {
            if (task.Priority > 5) {
                task.Complete();
                completed = completed + 1;
            } else {
                // Skip low priority
                i = i + 1;
                continue;
            }
        }
        
        i = i + 1;
        
        if (completed >= 10) {
            break;
        }
    }
    
    return completed;
}

// Main execution
let taskList = CreateTasks(20);  // Local variable (camelCase)
let result = ProcessTasks(taskList);
```

#### Example 2: Nested Structures

```javascript
// Global configuration (PascalCase for global variable)
let AppConfig = {
    "database": {
        "host": "localhost",
        "port": 5432,
        "credentials": {
            "username": "admin",
            "password": "secret"
        }
    },
    "features": ["auth", "api", "cache"],
    "limits": {
        "maxUsers": 1000,
        "maxRequests": 10000
    }
};

// Function (PascalCase)
fn GetDatabaseUrl(cfg) {
    // Parameter (camelCase)
    let db = cfg["database"];  // Local variable (camelCase)
    let host = db["host"];
    let port = db["port"];
    return host + ":" + port;
}

// Function (PascalCase)
fn ListFeatures(cfg) {
    // Parameter (camelCase)
    let features = cfg["features"];  // Local variable (camelCase)
    let i = 0;
    
    while (i < 3) {
        Print(features[i]);
        i = i + 1;
    }
}
```

#### Example 3: Complex Control Flow

```javascript
// Function (PascalCase)
fn Fibonacci(n) {
    // Parameter (camelCase)
    if (n <= 1) {
        return n;
    }
    
    // Local variables (camelCase)
    let a = 0;
    let b = 1;
    let result = 0;
    
    // Loop variable (camelCase)
    for (let i = 2; i <= n; i = i + 1) {
        result = a + b;
        a = b;
        b = result;
    }
    
    return result;
}

// Function (PascalCase)
fn Factorial(n) {
    // Parameter (camelCase)
    if (n <= 1) {
        return 1;
    }
    return n * Factorial(n - 1);
}

// Function (PascalCase)
fn IsPrime(n) {
    // Parameter (camelCase)
    if (n <= 1) {
        return false;
    }
    
    if (n == 2) {
        return true;
    }
    
    if (n % 2 == 0) {
        return false;
    }
    
    // Local variable (camelCase)
    let i = 3;
    while (i * i <= n) {
        if (n % i == 0) {
            return false;
        }
        i = i + 2;
    }
    
    return true;
}
```

#### Example 4: Object-Oriented Pattern

```javascript
// Class (PascalCase)
class Shape {
    // Class attributes (PascalCase)
    X = 0;
    Y = 0;
    
    // Method (PascalCase)
    fn Initialize(posX, posY) {
        // Parameters (camelCase)
        X = posX;
        Y = posY;
    }
    
    // Method (PascalCase)
    fn Move(dx, dy) {
        // Parameters (camelCase)
        X = X + dx;
        Y = Y + dy;
    }
}

// Class (PascalCase)
class Circle {
    // Class attributes (PascalCase)
    Center = null;
    Radius = 0;
    
    // Method (PascalCase)
    fn Initialize(x, y, r) {
        // Parameters (camelCase)
        Center = Shape;
        Center.Initialize(x, y);
        Radius = r;
    }
    
    // Method (PascalCase)
    fn CalculateArea() {
        return 3.14159 * Radius * Radius;
    }
    
    // Method (PascalCase)
    fn Contains(px, py) {
        // Parameters (camelCase)
        let dx = px - Center.X;  // Local variables (camelCase)
        let dy = py - Center.Y;
        let distSquared = dx * dx + dy * dy;
        return distSquared <= Radius * Radius;
    }
}

// Usage
let circle = Circle;  // Local variable (camelCase)
circle.Initialize(10, 10, 5);
let inside = circle.Contains(12, 11);
let area = circle.CalculateArea();
```

---

## Summary: Language-X Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Language-X Interpreter                   │
│          (LL(1) Recursive Descent - Statement-Based)         │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Source Code (UTF-8)                                         │
│       ↓                                                       │
│  ┌─────────────────┐                                         │
│  │   Tokenizer     │  • Position tracking (line/col)         │
│  │   (Lexer)       │  • Unicode codepoint support            │
│  └────────┬────────┘  • Multiple number bases               │
│           ↓           • Keywords, symbols, literals          │
│  Token Stream (with 1-token lookahead)                       │
│       ↓                                                       │
│  ┌─────────────────┐                                         │
│  │  LL(1) Parser   │  • Recursive descent strategy           │
│  │                 │  • Single token lookahead               │
│  │                 │  • Statement & expression parsing       │
│  └────────┬────────┘  • No left recursion                    │
│           ↓           • Precedence climbing (12 levels)      │
│  AST (Abstract Syntax Tree)                                  │
│       ↓                                                       │
│  ┌─────────────────┐                                         │
│  │   Evaluator     │  • Tree-walk interpretation             │
│  │   (Future)      │  • Dynamic typing                       │
│  └─────────────────┘  • Arena-based memory management       │
│                                                               │
└─────────────────────────────────────────────────────────────┘

Language Features:
  ✓ Variables & Assignment (let x = value;)
  ✓ Functions (fn name(params) { body })
  ✓ Classes (class Name { fields; methods })
  ✓ Enums (enum Name { Value1, Value2 })
  ✓ If/Else (if (cond) { } else { })
  ✓ While loops (while (cond) { })
  ✓ Do-While loops (do { } while (cond);)
  ✓ For loops (for (init; cond; incr) { })
  ✓ Arrays ([1, 2, 3])
  ✓ Dictionaries ({"key": value})
  ✓ Break/Continue (loop control)
  ✓ Return statements (return value;)
  ✓ Binary operators (12 precedence levels)
  ✓ Member access (object.field)
  ✓ Function calls (func(args))
  ✓ Array indexing (arr[index])

Parsing Methodology: LL(1) Recursive Descent
  • Top-down parsing from start symbol
  • Each grammar rule = one function
  • Single token lookahead for all decisions
  • Left-to-right scan, leftmost derivation
  • Iteration replaces left recursion
  • O(n) time complexity (linear pass)
  • Deterministic (no backtracking)
  • Statement-based (not expression-only)
```

---

## Quick Reference: LL(1) Implementation Checklist

When implementing Language-X parser in Zig:

### Phase 1: Tokenizer & Core Infrastructure
- [ ] **Tokenizer**: Implement `TokenizerInit()` and `NextToken()`
- [ ] **Position Tracking**: Implement `Position` struct with `Merge()`
- [ ] **Token Types**: Define all token types (TT_IDN, TT_KEY, TT_INT, TT_NUM, TT_STR, TT_SYM, TT_EOF)
- [ ] **Keywords**: Add keyword recognition (let, fn, if, else, while, do, for, return, break, continue, class, enum, true, false, null)

### Phase 2: Parser Foundation
- [ ] **Parser State**: Add `tokenizer`, `current_token`, `allocator`, `had_error` fields
- [ ] **Helper Functions**: Implement `advance()`, `peek()`, `expect()`, `expectValue()`
- [ ] **Error Handling**: Add `reportError()` and `synchronize()` functions
- [ ] **AST Types**: Define all AST types in `AstType` enum (50+ types)
- [ ] **AST Construction**: Implement `Ast.init()` and `Ast.initWithValue()`

### Phase 3: Expression Parsing (Bottom-Up)
- [ ] **Primary Expressions**: Implement switch-based `primary()` parser (literals, identifiers, grouping)
- [ ] **Collections**: Implement `array()` and `dictionary()` with iteration
- [ ] **Key-Value Pairs**: Implement `keyValue()` for dictionaries
- [ ] **Postfix Operators**: Implement `postfix()` for indexing, calls, member access
- [ ] **Precedence Functions**: Create 12 functions for operator precedence:
  - [ ] `multiplicative()` - *, /, %
  - [ ] `additive()` - +, -
  - [ ] `shift()` - <<, >>
  - [ ] `comparison()` - <, <=, >, >=
  - [ ] `equality()` - ==, !=
  - [ ] `bitwiseAnd()` - &
  - [ ] `bitwiseXor()` - ^
  - [ ] `bitwiseOr()` - |
  - [ ] `logicalAnd()` - &&
  - [ ] `logicalOr()` - ||
  - [ ] `assignment()` - =
  - [ ] `expression()` - entry point

### Phase 4: Statement Parsing
- [ ] **Statement Dispatcher**: Implement `statement()` with keyword-based dispatch
- [ ] **Block Statements**: Implement `block()` for `{ stmt* }`
- [ ] **Expression Statements**: Implement `expressionStatement()` for `expr;`
- [ ] **Variable Declarations**: Implement `varDeclaration()` for `let x = value;`
- [ ] **Return Statements**: Implement `returnStatement()` for `return expr;`
- [ ] **Break/Continue**: Implement `breakStatement()` and `continueStatement()`

### Phase 5: Control Flow
- [ ] **If Statements**: Implement `ifStatement()` with optional else and else-if chains
- [ ] **While Loops**: Implement `whileStatement()` for `while (cond) { }`
- [ ] **Do-While Loops**: Implement `doWhileStatement()` for `do { } while (cond);`
- [ ] **For Loops**: Implement `forStatement()` with init, condition, increment

### Phase 6: Functions
- [ ] **Function Declarations**: Implement `functionDeclaration()` for `fn name(params) { }`
- [ ] **Parameter Lists**: Parse comma-separated parameters
- [ ] **Function Calls**: Extend `postfix()` to handle `(args)` after identifiers
- [ ] **Argument Lists**: Parse comma-separated arguments
- [ ] **Return Validation**: Ensure functions can return values

### Phase 7: Object-Oriented Features
- [ ] **Class Declarations**: Implement `classDeclaration()` for `class Name { members }`
- [ ] **Class Members**: Parse fields and methods inside classes
- [ ] **Member Access**: Extend `postfix()` to handle `.` operator
- [ ] **Enum Declarations**: Implement `enumDeclaration()` for `enum Name { values }`
- [ ] **Enum Values**: Parse comma-separated identifiers

### Phase 8: Program Structure
- [ ] **Program Entry**: Implement `parseProgram()` to parse statement sequence
- [ ] **EOF Handling**: Ensure proper end-of-file detection
- [ ] **Top-Level Declarations**: Support functions, classes, enums at program level

### Phase 9: Testing & Validation
- [ ] **Unit Tests**: Test each precedence level independently
- [ ] **Edge Cases**: Handle empty arrays, trailing commas, nested structures
- [ ] **Error Recovery**: Test synchronization and error reporting
- [ ] **Integration Tests**: Test complete programs with all features
- [ ] **LL(1) Validation**: Verify no lookahead conflicts
- [ ] **Performance**: Profile parser on large files

### Phase 10: Documentation
- [ ] **Grammar Documentation**: Document complete LL(1) grammar
- [ ] **API Documentation**: Document all public parser functions
- [ ] **Examples**: Provide example programs using all features
- [ ] **Error Messages**: Document all error conditions and messages

---

## Implementation Order Recommendation

**Recommended implementation order** (dependencies first):

1. **Tokenizer** (Phase 1) - Foundation for everything
2. **Parser Infrastructure** (Phase 2) - Core utilities
3. **Primary Expressions** (Phase 3.1) - Base expressions
4. **Binary Operators** (Phase 3.5) - Expression precedence
5. **Collections** (Phase 3.2) - Arrays and dictionaries
6. **Postfix Operators** (Phase 3.3) - Indexing first
7. **Block Statements** (Phase 4.2) - Needed by control flow
8. **Simple Statements** (Phase 4.3-4.6) - Variables, return, break, continue
9. **Control Flow** (Phase 5) - If, while, do-while, for
10. **Functions** (Phase 6) - Declarations and calls
11. **OOP Features** (Phase 7) - Classes and enums
12. **Program Structure** (Phase 8) - Top-level parsing
13. **Testing** (Phase 9) - Comprehensive validation
14. **Documentation** (Phase 10) - Final polish

---

## Common LL(1) Implementation Pitfalls

### Pitfall 1: Ambiguous Grammar
❌ **Problem**: Multiple productions start with same token
```
statement → if_stmt | if_expr
if_stmt → "if" "(" expr ")" block
if_expr → "if" "(" expr ")" expr
```
✓ **Solution**: Merge or distinguish with additional lookahead in grammar redesign

### Pitfall 2: Left Recursion
❌ **Problem**: Direct or indirect left recursion
```
expr → expr "+" term  // Left recursion!
```
✓ **Solution**: Convert to iteration
```
expr → term ("+" term)*  // LL(1) compliant
```

### Pitfall 3: Missing Synchronization
❌ **Problem**: Parser crashes on first error
✓ **Solution**: Implement panic-mode recovery with synchronization tokens

### Pitfall 4: Incorrect Precedence
❌ **Problem**: `3 + 4 * 5` parses as `(3 + 4) * 5`
✓ **Solution**: Ensure higher precedence operators call lower precedence functions

### Pitfall 5: Memory Leaks
❌ **Problem**: AST nodes allocated but never freed
✓ **Solution**: Use arena allocator or implement proper cleanup

### Pitfall 6: Inconsistent AST Structure
❌ **Problem**: Different node types use left/right/next inconsistently
✓ **Solution**: Document and follow consistent AST conventions

### Pitfall 7: Poor Error Messages
❌ **Problem**: "Syntax error" with no context
✓ **Solution**: Include position, expected tokens, and suggestions

### Pitfall 8: No Empty Production Handling
❌ **Problem**: Can't parse empty arrays `[]` or optional elements
✓ **Solution**: Check for closing delimiter before parsing elements

---

This context document provides comprehensive guidance for implementing Language-X, a modern **statement-based** interpreted language with **LL(1) recursive descent parsing**, supporting:
- **Control flow**: if/else, while, do-while, for loops
- **Functions**: First-class functions with parameters and return values
- **OOP**: Classes with fields and methods, enumerations
- **Data structures**: Arrays, dictionaries with indexing
- **Operators**: 12 precedence levels with bitwise, logical, and arithmetic operations
- **Type system**: Dynamic typing with null, booleans, numbers, strings

---

## Quick Reference: Naming Conventions

### camelCase (Local Scope)
```javascript
// Local variables
let userName = "John";
let itemCount = 42;
let isActive = true;

// Function parameters
fn ProcessUser(userId, accountName) { }

// Loop variables
for (let i = 0; i < 10; i = i + 1) { }
while (index < max) { }
```

### PascalCase (Global/Type Scope)
```javascript
// Global variables
let MaxRetries = 3;
let DefaultConfig = {...};

// Functions and Methods
fn CalculateTotal() { }
fn ProcessData(items) { }

// Classes and Attributes
class UserAccount {
    FirstName = "";
    LastName = "";
    IsActive = true;
    
    fn GetFullName() {
        return FirstName + " " + LastName;
    }
}

// Enums and Values
enum StatusCode {
    Success,
    Error,
    Pending
}
```

### Quick Decision Guide
- **Is it defined inside a function/block?** → Use **camelCase**
- **Is it a parameter?** → Use **camelCase**
- **Is it a function/method name?** → Use **PascalCase**
- **Is it a class/enum name?** → Use **PascalCase**
- **Is it a class attribute/field?** → Use **PascalCase**
- **Is it a global variable/constant?** → Use **PascalCase**
- **Is it an enum value?** → Use **PascalCase**

