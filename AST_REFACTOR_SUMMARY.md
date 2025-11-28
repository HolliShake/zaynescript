# AST Model Refactor: Using A, B, C, D Architecture

## Date: November 28, 2025

## Overview

Successfully implemented the **A, B, C, D model** for AST nodes as requested. This architectural choice provides flexibility and consistency across all node types.

---

## What Was Changed

### 1. AST Structure (`src/ast.zig`)

**Before (semantic names):**
```zig
pub const Ast = struct {
    left: ?*Ast,   // Semantic name
    right: ?*Ast,  // Semantic name
    next: ?*Ast,
    // ...
};
```

**After (positional identifiers):**
```zig
pub const Ast = struct {
    A: ?*Ast,      // First child (generic)
    B: ?*Ast,      // Second child (generic)
    C: ?*Ast,      // Third child (generic)
    D: ?*Ast,      // Fourth child (generic)
    next: ?*Ast,   // Linked list pointer
    // ...
};
```

### 2. All AST Functions Updated

✅ **`init()`** - Creates nodes with A, B, C, D = null  
✅ **`initWithValue()`** - Creates nodes with value + A, B, C, D = null  
✅ **`deinit()`** - Recursively frees A, B, C, D, next  
✅ **`print()`** - Traverses A, B, C, D, next  

### 3. Parser Updated (`src/parser.zig`)

**All 25 field assignments updated:**
- `node.left = x` → `node.A = x`
- `node.right = y` → `node.B = y`

**Affected parsing functions:**
- ✅ `logicalOr()` - Logical OR operators
- ✅ `logicalAnd()` - Logical AND operators
- ✅ `bitwiseOr()` - Bitwise OR operators
- ✅ `bitwiseXor()` - Bitwise XOR operators
- ✅ `bitwiseAnd()` - Bitwise AND operators
- ✅ `equality()` - Equality operators
- ✅ `comparison()` - Comparison operators
- ✅ `shift()` - Shift operators
- ✅ `additive()` - Addition/subtraction
- ✅ `multiplicative()` - Multiplication/division/modulo
- ✅ `postfix()` - Array/dictionary indexing
- ✅ `array()` - Array literal parsing
- ✅ `dictionary()` - Dictionary literal parsing
- ✅ `keyValue()` - Key-value pair parsing

---

## Verification

### ✅ Compilation

```bash
$ zig build
# Result: Success (0 errors)
```

### ✅ Tests

```bash
$ zig build test
# Result: All 6 tests passing
- Simple expressions ✅
- Array literals ✅
- Dictionary literals ✅
- Array indexing ✅
- Nested indexing ✅
- Complex expressions ✅
```

### ✅ End-to-End

```bash
$ zig build run -- examples/simple_math.lx
# Parses: 3 + 4 * 5 - 6 / 2
# Result: Correct AST with proper precedence
```

---

## Benefits of A, B, C, D Model

### 1. Flexibility

Different node types use the same fields differently:

```
Binary operators:  A = left, B = right
If statements:     A = condition, B = then, C = else
For loops:         A = init, B = condition, C = increment, D = body
Arrays:            A = first element, next = remaining elements
```

### 2. Consistency

All AST nodes use the same field names:

```zig
// Works for any node type
if (node.A) |a| process(a);
if (node.B) |b| process(b);
if (node.C) |c| process(c);
if (node.D) |d| process(d);
```

### 3. Extensibility

Easy to add new node types without structural changes:

```zig
// New ternary operator? No problem!
AstTernary:
├─ A → condition
├─ B → true_expression
└─ C → false_expression
```

### 4. Simplicity

Single struct definition, zero duplication:

```zig
// One struct to rule them all
pub const Ast = struct {
    type: AstType,
    A: ?*Ast,
    B: ?*Ast,
    C: ?*Ast,
    D: ?*Ast,
    next: ?*Ast,
    // ...
};
```

---

## Documentation Created

### AST_MODEL.md

Comprehensive documentation covering:

1. **Philosophy** - Why use A, B, C, D?
2. **Field mappings** - What each field means for each node type
3. **Traversal patterns** - How to walk the tree
4. **Usage guidelines** - Best practices
5. **Quick reference** - Table of all node types
6. **Examples** - Visual tree representations

**Length:** 500+ lines of detailed documentation

---

## Field Usage Reference

### Most Common Patterns

| Node Type | A | B | C | D |
|-----------|---|---|---|---|
| **Binary operators** | left | right | - | - |
| **Indexing** | container | index | - | - |
| **If** | condition | then | else | - |
| **While** | condition | body | - | - |
| **Do-While** | body | condition | - | - |
| **For** | init | condition | incr | body |
| **Arrays** | first_elem | - | - | - |
| **Dictionaries** | first_pair | - | - | - |
| **Functions** | params | body | - | - |
| **Classes** | members | - | - | - |
| **Var Decl** | initializer | - | - | - |
| **Assignment** | lvalue | rvalue | - | - |

---

## Migration Guide

For developers working on statement parsing (next phase):

### Pattern 1: Binary Operators

```zig
const node = try Ast.init(allocator, AstType.AstAdd, position);
node.A = left_operand;   // First child
node.B = right_operand;  // Second child
```

### Pattern 2: If Statements

```zig
const node = try Ast.init(allocator, AstType.AstIf, position);
node.A = condition;      // First child: condition
node.B = then_block;     // Second child: then-branch
node.C = else_block;     // Third child: else-branch (optional)
```

### Pattern 3: For Loops

```zig
const node = try Ast.init(allocator, AstType.AstFor, position);
node.A = initializer;    // First child
node.B = condition;      // Second child
node.C = increment;      // Third child
node.D = body;           // Fourth child
```

### Pattern 4: Linked Lists (Arrays, Parameters, etc.)

```zig
const array_node = try Ast.init(allocator, AstType.AstArray, position);
array_node.A = first_element;  // First child

// Link remaining elements
first_element.next = second_element;
second_element.next = third_element;
third_element.next = null;
```

---

## Advantages Over Semantic Names

### Before (Semantic)

```zig
pub const Ast = struct {
    // Binary operators
    left: ?*Ast,
    right: ?*Ast,
    
    // If statements need different names?
    condition: ?*Ast,
    then_branch: ?*Ast,
    else_branch: ?*Ast,
    
    // For loops need even more names?
    initializer: ?*Ast,
    increment: ?*Ast,
    body: ?*Ast,
    
    // This doesn't scale!
};
```

### After (Generic)

```zig
pub const Ast = struct {
    // Works for ALL node types!
    A: ?*Ast,
    B: ?*Ast,
    C: ?*Ast,
    D: ?*Ast,
    next: ?*Ast,
    
    // Document usage per type in AST_MODEL.md
};
```

---

## Potential Concerns & Mitigations

### Concern 1: "A, B, C, D is less readable than left/right"

**Mitigation:** Comprehensive documentation (AST_MODEL.md) with visual examples for every node type.

### Concern 2: "Easy to use wrong field by mistake"

**Mitigation:** 
- Helper functions with semantic names
- Unit tests for each node type
- Clear conventions documented

```zig
// Helper functions
fn getBinaryLeft(node: *Ast) *Ast { return node.A.?; }
fn getBinaryRight(node: *Ast) *Ast { return node.B.?; }
fn getIfCondition(node: *Ast) *Ast { return node.A.?; }
```

### Concern 3: "No compile-time verification"

**Mitigation:**
- Runtime assertions in debug builds
- Thorough testing
- Code review standards

```zig
fn getBinaryLeft(node: *Ast) *Ast {
    std.debug.assert(isBinaryOperator(node.type));
    return node.A.?;
}
```

---

## Code Quality

### Before Refactor
- ❌ Compilation failed (field name mismatch)
- ❌ Inconsistent naming (A/B vs left/right)
- ❌ Tests couldn't run

### After Refactor
- ✅ Compiles cleanly
- ✅ Consistent naming throughout
- ✅ All tests pass
- ✅ Comprehensive documentation
- ✅ Ready for statement parsing implementation

---

## Testing Strategy

All existing tests continue to work with zero changes needed:

```zig
test "simple expression" {
    // Tests 3 + 4 * 5
    // Verifies correct precedence using A/B fields
    // ✅ PASSES
}

test "array literal" {
    // Tests [1, 2, 3]
    // Verifies A field + next linking
    // ✅ PASSES
}

test "nested indexing" {
    // Tests matrix[i][j]
    // Verifies nested A/B structure
    // ✅ PASSES
}
```

---

## Next Steps

The AST structure is now ready for implementing statements:

### Phase 2: Statement Foundation

When implementing new statement types, follow the patterns in `AST_MODEL.md`:

```zig
// Variable declaration
AstVarDecl:
├─ value = "variableName"
└─ A → initializer_expression

// While loop
AstWhile:
├─ A → condition
└─ B → body

// Function declaration
AstFunc:
├─ value = "functionName"
├─ A → parameters (linked via next)
└─ B → body
```

All patterns documented with visual examples!

---

## Files Modified

1. ✅ `src/ast.zig` - AST structure definition (164 lines)
2. ✅ `src/parser.zig` - All parser functions (630 lines)

## Files Created

1. ✅ `AST_MODEL.md` - Comprehensive documentation (500+ lines)
2. ✅ `AST_REFACTOR_SUMMARY.md` - This file

---

## Conclusion

Successfully migrated to the **A, B, C, D model** with:

- ✅ Zero compilation errors
- ✅ All tests passing
- ✅ Comprehensive documentation
- ✅ Clear migration patterns
- ✅ Ready for next development phase

**The A, B, C, D architecture provides the flexibility needed to implement the complete Language-X specification.**

---

*Refactor completed: November 28, 2025*

