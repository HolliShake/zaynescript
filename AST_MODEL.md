# Language-X AST Model: A, B, C, D Architecture

## Overview

The Language-X AST uses an **A, B, C, D** field naming convention for child nodes, providing a flexible and extensible structure that can accommodate various node types with different arities.

## Philosophy

Instead of using semantic names like `left`, `right`, `condition`, `body`, etc., we use **generic positional identifiers** (A, B, C, D) that:

1. **Provide consistency** - All AST nodes use the same field names regardless of their semantic meaning
2. **Enable flexibility** - Easy to add new node types without changing the core structure
3. **Simplify traversal** - Generic traversal code works for all node types
4. **Reduce coupling** - Parser doesn't need to know the specific semantic meaning of each field

---

## AST Structure

```zig
pub const Ast = struct {
    type: AstType,              // Discriminator: what kind of node is this?
    position: Position,         // Source location for error reporting
    value: []const u8,          // String data (identifiers, literals)
    A: ?*Ast,                   // First child node
    B: ?*Ast,                   // Second child node
    C: ?*Ast,                   // Third child node
    D: ?*Ast,                   // Fourth child node
    next: ?*Ast,                // Linked list pointer (for sequences)
    allocator: std.mem.Allocator,
};
```

### Field Purposes

| Field | Purpose | Example Uses |
|-------|---------|-------------|
| **A** | First/primary child | Left operand, condition, array elements, dict entries |
| **B** | Second child | Right operand, then-block, function body |
| **C** | Third child | Else-block, for-loop increment |
| **D** | Fourth child | Additional data, for-loop initializer |
| **next** | Linked list | Array elements, function parameters, statements |

---

## Node Type Mappings

### Binary Operators (Arithmetic, Logical, Bitwise, Comparison)

**Pattern:** `A` = left operand, `B` = right operand

```
AstAdd: 3 + 4
├─ A → AstInt(3)      // Left operand
└─ B → AstInt(4)      // Right operand

AstMul: x * y
├─ A → AstIdn("x")
└─ B → AstIdn("y")

AstLAnd: a && b
├─ A → AstIdn("a")
└─ B → AstIdn("b")
```

**Applies to:**
- Arithmetic: `AstAdd`, `AstSub`, `AstMul`, `AstDiv`, `AstMod`
- Bitwise: `AstAnd`, `AstOr`, `AstXor`, `AstLShft`, `AstRShft`
- Logical: `AstLAnd`, `AstLOr`
- Comparison: `AstLt`, `AstLte`, `AstGt`, `AstGte`, `AstEq`, `AstNeq`

### Indexing Operations

**Pattern:** `A` = container, `B` = index

```
AstIndex: arr[0]
├─ A → AstIdn("arr")    // Container
└─ B → AstInt(0)        // Index

AstIndex: matrix[i][j]
├─ A → AstIndex         // Nested: matrix[i]
│      ├─ A → AstIdn("matrix")
│      └─ B → AstIdn("i")
└─ B → AstIdn("j")      // Second index
```

### Arrays

**Pattern:** `A` = first element, elements linked via `next`

```
AstArray: [1, 2, 3]
└─ A → AstInt(1)
       └─ next → AstInt(2)
                 └─ next → AstInt(3)
                           └─ next → null

AstArray: []  (empty)
└─ A → null
```

### Dictionaries

**Pattern:** `A` = first key-value pair, pairs linked via `next`

```
AstDict: {"name": "John", "age": 30}
└─ A → AstKeyValue("name")
       ├─ A → AstStr("John")
       └─ next → AstKeyValue("age")
                 ├─ A → AstInt(30)
                 └─ next → null
```

### Control Flow: If Statements

**Pattern:** `A` = condition, `B` = then-block, `C` = else-block

```
AstIf: if (x > 0) { } else { }
├─ A → AstGt              // Condition
│      ├─ A → AstIdn("x")
│      └─ B → AstInt(0)
├─ B → AstBlock           // Then-block
└─ C → AstBlock           // Else-block (optional)

AstIf: if (x > 0) { } else if (x < 0) { } else { }
├─ A → AstGt              // Condition
├─ B → AstBlock           // Then-block
└─ C → AstIf              // Else-if (recursive)
       ├─ A → AstLt
       ├─ B → AstBlock
       └─ C → AstBlock    // Final else
```

### Control Flow: While Loops

**Pattern:** `A` = condition, `B` = body

```
AstWhile: while (i < 10) { }
├─ A → AstLt              // Condition
│      ├─ A → AstIdn("i")
│      └─ B → AstInt(10)
└─ B → AstBlock           // Body
```

### Control Flow: Do-While Loops

**Pattern:** `A` = body, `B` = condition

```
AstDoWhile: do { } while (x > 0);
├─ A → AstBlock           // Body (executed first)
└─ B → AstGt              // Condition (checked after)
       ├─ A → AstIdn("x")
       └─ B → AstInt(0)
```

### Control Flow: For Loops

**Pattern:** `A` = initializer, `B` = condition, `C` = increment, `D` = body

```
AstFor: for (let i = 0; i < 10; i = i + 1) { }
├─ A → AstVarDecl         // Initializer
│      ├─ value = "i"
│      └─ A → AstInt(0)
├─ B → AstLt              // Condition
│      ├─ A → AstIdn("i")
│      └─ B → AstInt(10)
├─ C → AstAssign          // Increment
│      ├─ A → AstIdn("i")
│      └─ B → AstAdd
│             ├─ A → AstIdn("i")
│             └─ B → AstInt(1)
└─ D → AstBlock           // Body

AstFor: for (;;) { }  (infinite loop)
├─ A → null               // No initializer
├─ B → null               // No condition
├─ C → null               // No increment
└─ D → AstBlock           // Body
```

### Variables

**Pattern:** `A` = initializer expression

```
AstVarDecl: let x = 10;
├─ value = "x"            // Variable name
└─ A → AstInt(10)         // Initializer (optional)

AstAssign: x = 20;
├─ A → AstIdn("x")        // L-value
└─ B → AstInt(20)         // R-value
```

### Functions

**Pattern:** `A` = parameters (linked list), `B` = body

```
AstFunc: fn Add(x, y) { return x + y; }
├─ value = "Add"          // Function name
├─ A → AstIdn("x")        // First parameter
│      └─ next → AstIdn("y")  // Second parameter
│               └─ next → null
└─ B → AstBlock           // Function body
       └─ A → AstReturn
              └─ A → AstAdd
                     ├─ A → AstIdn("x")
                     └─ B → AstIdn("y")

AstCall: Add(5, 10)
├─ A → AstIdn("Add")      // Function being called
└─ B → AstInt(5)          // First argument
       └─ next → AstInt(10)  // Second argument
                 └─ next → null
```

### Classes

**Pattern:** `A` = members (linked list)

```
AstClass: class Point { X = 0; Y = 0; }
├─ value = "Point"        // Class name
└─ A → AstVarDecl("X")    // First member
       ├─ A → AstInt(0)
       └─ next → AstVarDecl("Y")
                 ├─ A → AstInt(0)
                 └─ next → null

AstMember: obj.field
├─ A → AstIdn("obj")      // Object
└─ value = "field"        // Member name
```

### Enums

**Pattern:** `A` = values (linked list)

```
AstEnum: enum Status { Pending, Active, Completed }
├─ value = "Status"       // Enum name
└─ A → AstIdn("Pending")  // First value
       └─ next → AstIdn("Active")
                 └─ next → AstIdn("Completed")
                           └─ next → null
```

### Statements

**Pattern:** Varies by statement type

```
AstBlock: { stmt1; stmt2; stmt3; }
└─ A → stmt1              // First statement
       └─ next → stmt2    // Second statement
                 └─ next → stmt3
                           └─ next → null

AstReturn: return expr;
└─ A → expr               // Return value (optional)

AstBreak: break;
└─ (no children)

AstContinue: continue;
└─ (no children)

AstExprStmt: expr;
└─ A → expr               // Expression to evaluate
```

---

## Traversal Patterns

### Recursive Traversal

```zig
fn traverse(node: *Ast) void {
    // Process current node
    processNode(node);
    
    // Visit all children
    if (node.A) |a| traverse(a);
    if (node.B) |b| traverse(b);
    if (node.C) |c| traverse(c);
    if (node.D) |d| traverse(d);
    
    // Visit linked list
    if (node.next) |next| traverse(next);
}
```

### Memory Cleanup

```zig
pub fn deinit(self: *Ast) void {
    // Recursively free all children
    if (self.A) |a| a.deinit();
    if (self.B) |b| b.deinit();
    if (self.C) |c| c.deinit();
    if (self.D) |d| d.deinit();
    if (self.next) |next| next.deinit();
    
    // Free this node
    self.allocator.destroy(self);
}
```

### Type-Specific Access

```zig
// For binary operators: use A and B
fn evalBinaryOp(node: *Ast) i64 {
    const left = eval(node.A.?);
    const right = eval(node.B.?);
    return left + right;  // Example for addition
}

// For if statements: use A, B, C
fn evalIf(node: *Ast) void {
    if (eval(node.A.?)) {  // Condition
        eval(node.B.?);     // Then-block
    } else if (node.C) |else_block| {
        eval(else_block);   // Else-block
    }
}

// For loops through linked lists
fn evalArray(node: *Ast) []Value {
    var values = ArrayList(Value).init(allocator);
    var current = node.A;  // First element
    while (current) |elem| {
        values.append(eval(elem));
        current = elem.next;
    }
    return values.toOwnedSlice();
}
```

---

## Advantages of A, B, C, D Model

### 1. **Consistency**

All AST nodes use the same field names, making generic code easier to write:

```zig
// Generic child visitor
fn visitChildren(node: *Ast, visitor: anytype) void {
    if (node.A) |a| visitor.visit(a);
    if (node.B) |b| visitor.visit(b);
    if (node.C) |c| visitor.visit(c);
    if (node.D) |d| visitor.visit(d);
}
```

### 2. **Flexibility**

Easy to repurpose fields for different node types without changing structure:

- Binary operators: A = left, B = right
- If statements: A = condition, B = then, C = else
- For loops: A = init, B = condition, C = increment, D = body

### 3. **Extensibility**

Adding new node types doesn't require structural changes:

```zig
// New node type? Just document which fields you use
AstTernary: condition ? true_expr : false_expr
├─ A → condition
├─ B → true_expr
└─ C → false_expr
```

### 4. **Simplicity**

Single struct definition handles all node types:

```zig
// One struct, many uses
pub const Ast = struct {
    type: AstType,
    position: Position,
    value: []const u8,
    A: ?*Ast,
    B: ?*Ast,
    C: ?*Ast,
    D: ?*Ast,
    next: ?*Ast,
    allocator: std.mem.Allocator,
};
```

---

## Disadvantages and Mitigations

### Disadvantage 1: Less Self-Documenting

**Problem:** Code like `node.A` is less clear than `node.condition`

**Mitigation:** 
- Comprehensive documentation (this file!)
- Helper functions with semantic names:

```zig
fn getCondition(node: *Ast) ?*Ast { return node.A; }
fn getThenBlock(node: *Ast) ?*Ast { return node.B; }
fn getElseBlock(node: *Ast) ?*Ast { return node.C; }
```

### Disadvantage 2: No Compile-Time Verification

**Problem:** Accessing wrong field won't cause compilation error

**Mitigation:**
- Well-documented conventions
- Helper functions for type-specific access
- Runtime assertions in debug mode:

```zig
fn getBinaryLeft(node: *Ast) *Ast {
    std.debug.assert(isBinaryOp(node.type));
    return node.A.?;
}
```

### Disadvantage 3: Potential Misuse

**Problem:** Easy to accidentally use wrong field

**Mitigation:**
- Code review standards
- Documentation for each node type
- Unit tests verifying correct field usage

---

## Quick Reference

### Most Common Patterns

| Node Type | A | B | C | D | next |
|-----------|---|---|---|---|------|
| Binary operators | left | right | - | - | - |
| Arrays | first elem | - | - | - | elements |
| Dictionaries | first pair | - | - | - | pairs |
| Indexing | container | index | - | - | - |
| If | condition | then | else | - | - |
| While | condition | body | - | - | - |
| Do-While | body | condition | - | - | - |
| For | init | condition | incr | body | - |
| Var Decl | initializer | - | - | - | - |
| Assignment | lvalue | rvalue | - | - | - |
| Function | params | body | - | - | - |
| Call | callee | args | - | - | - |
| Class | members | - | - | - | - |
| Enum | values | - | - | - | - |
| Block | first stmt | - | - | - | stmts |
| Return | value | - | - | - | - |
| Member | object | - | - | - | - |

---

## Usage Guidelines

### 1. Document Your Usage

When implementing a new node type, document which fields you use:

```zig
// AstMyNewNode uses:
// - A: primary data
// - B: secondary data
// - value: identifier name
// - next: linked list of related nodes
```

### 2. Be Consistent

If you use A for "left operand" in one binary operator, use it for ALL binary operators.

### 3. Use Helper Functions

Wrap field access in semantic functions:

```zig
fn getLoopCondition(node: *Ast) *Ast {
    return switch (node.type) {
        .AstWhile => node.A.?,
        .AstDoWhile => node.B.?,
        .AstFor => node.B,
        else => unreachable,
    };
}
```

### 4. Test Field Usage

Write tests to verify correct field access:

```zig
test "binary operator uses A and B" {
    const node = try Ast.init(allocator, .AstAdd, position);
    node.A = left_operand;
    node.B = right_operand;
    try testing.expect(node.A == left_operand);
    try testing.expect(node.B == right_operand);
}
```

---

## Conclusion

The **A, B, C, D model** provides a flexible, extensible AST structure that:

- ✅ Works well for diverse node types
- ✅ Simplifies generic traversal code
- ✅ Reduces structural coupling
- ✅ Makes adding new features easier

While it trades some self-documentation for flexibility, proper documentation and helper functions mitigate this concern.

**Recommendation:** Use this model when building interpreters/compilers that need to support many diverse node types with different arities.

---

*AST Model Documentation - Language-X Interpreter*

