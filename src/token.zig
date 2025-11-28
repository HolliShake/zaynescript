const std = @import("std");
const Position = @import("position.zig").Position;

/// TokenType represents the different types of tokens in the language
pub const TokenType = enum {
    TT_IDN, // Identifiers (variable names, function names)
    TT_KEY, // Keywords (if, while, func, struct, etc.)
    TT_NUM, // Floating-point numbers (0.2, 2.2, 2.2e2)
    TT_INT, // Integers with multiple bases (100, 0xff, 0b1010, 0o232)
    TT_STR, // String literals
    TT_SYM, // Symbols/operators (+, -, *, ==, etc.)
    TT_EOF, // End of file marker

    pub fn format(
        self: TokenType,
        comptime fmt: []const u8,
        _: anytype,
        writer: anytype,
    ) !void {
        _ = fmt;
        try writer.writeAll(@tagName(self));
    }
};

/// Token encapsulates all information about a lexical token
pub const Token = struct {
    type: TokenType,
    value: []const u8,
    position: Position,

    pub fn init(token_type: TokenType, value: []const u8, position: Position) Token {
        return Token{
            .type = token_type,
            .value = value,
            .position = position,
        };
    }

    pub fn format(
        self: Token,
        comptime fmt: []const u8,
        _: anytype,
        writer: anytype,
    ) !void {
        _ = fmt;
        try writer.print("{s}('{s}')", .{
            @tagName(self.type),
            self.value,
        });
    }
};

/// Check if a string is a keyword
pub fn isKeyword(str: []const u8) bool {
    const keywords = [_][]const u8{
        "fn",
        "if",
        "else",
        "while",
        "do",
        "for",
        "return",
        "true",
        "false",
        "null",
        "let",
        "const",
        "struct",
        "class",
        "enum",
        "break",
        "continue",
    };

    for (keywords) |keyword| {
        if (std.mem.eql(u8, str, keyword)) {
            return true;
        }
    }
    return false;
}

