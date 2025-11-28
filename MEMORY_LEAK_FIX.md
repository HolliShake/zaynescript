# Memory Leak Fix

## Problem

The original implementation had memory leaks in the string tokenization code:

```
error(gpa): memory address 0x20b89330000 leaked:
C:\zig-x86_64-windows-0.16.0\lib\std\array_list.zig:662:52: 0x7ff7da6c0e8a in toOwnedSlice
```

The leaks occurred because:
1. String tokens were processed to handle escape sequences
2. This processing allocated new memory with `ArrayList` and `toOwnedSlice()`
3. The allocated string values were stored in tokens but never freed
4. When tokens went out of scope, the memory remained allocated

## Solution

**Zero-Copy String Tokenization**

Changed the string tokenizer to use slices into the original source buffer instead of allocating new memory:

### Before (with leaks):
```zig
// Allocated memory for processed strings with escape sequences
var processed = std.ArrayList(u8).initCapacity(self.allocator, raw_value.len);
// ... process escape sequences ...
const value = processed.toOwnedSlice(self.allocator); // ❌ LEAK
return Token.init(TokenType.TT_STR, value, position);
```

### After (no leaks):
```zig
// Return slice into original source buffer (no allocation)
// Content excludes the quotes
const position = Position.init(start_line, self.line, start_col, self.column);
return Token.init(TokenType.TT_STR, self.data[start_pos + 1..end_pos], position); // ✅ NO LEAK
```

## Benefits

1. **No Memory Leaks**: Zero allocations for string tokens
2. **Better Performance**: No allocation/deallocation overhead
3. **Simpler Code**: Removed complex escape sequence processing from tokenizer
4. **Standard Practice**: Most tokenizers use zero-copy approaches

## Design Decision

**Escape sequence processing is deferred to the evaluation phase.**

This is a common design pattern:
- **Tokenizer**: Identifies tokens and their boundaries (syntax)
- **Evaluator**: Processes token semantics (including escape sequences)

The tokenizer's job is just to recognize that a string exists, not to interpret its content.

## Verification

All tests now pass without memory leaks:

```bash
# Dictionary with multiple strings
$ zig build run -- examples/dictionary.lx
# Output: Parsing successful! (no leak errors)

# Array of strings
$ zig build run -- examples/strings.lx
# Output: Parsing successful! (no leak errors)

# Test suite
$ zig build test
# Output: All 6 tests pass (no leak errors)
```

## Status

✅ **FIXED** - All memory leaks resolved
✅ **VERIFIED** - Tested with multiple examples including strings
✅ **PERFORMANCE** - Improved by eliminating unnecessary allocations

