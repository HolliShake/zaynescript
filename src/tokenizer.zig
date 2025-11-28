const std = @import("std");
const Token = @import("token.zig").Token;
const TokenType = @import("token.zig").TokenType;
const Position = @import("position.zig").Position;
const isKeyword = @import("token.zig").isKeyword;

/// Tokenizer performs lexical analysis on source code
pub const Tokenizer = struct {
    path: []const u8,
    data: []const u8,
    pos: usize,
    line: u32,
    column: u32,
    allocator: std.mem.Allocator,

    /// Initialize a new tokenizer
    pub fn init(allocator: std.mem.Allocator, path: []const u8, data: []const u8) Tokenizer {
        return Tokenizer{
            .path = path,
            .data = data,
            .pos = 0,
            .line = 1,
            .column = 1,
            .allocator = allocator,
        };
    }

    /// Peek at the current character without consuming it
    fn peek(self: *Tokenizer) ?u8 {
        if (self.pos >= self.data.len) return null;
        return self.data[self.pos];
    }

    /// Peek ahead by n characters
    fn peekAhead(self: *Tokenizer, n: usize) ?u8 {
        const ahead_pos = self.pos + n;
        if (ahead_pos >= self.data.len) return null;
        return self.data[ahead_pos];
    }

    /// Consume and return the current character
    fn advance(self: *Tokenizer) ?u8 {
        if (self.pos >= self.data.len) return null;
        const ch = self.data[self.pos];
        self.pos += 1;
        if (ch == '\n') {
            self.line += 1;
            self.column = 1;
        } else {
            self.column += 1;
        }
        return ch;
    }

    /// Skip whitespace characters
    fn skipWhitespace(self: *Tokenizer) void {
        while (self.peek()) |ch| {
            if (ch == ' ' or ch == '\t' or ch == '\r' or ch == '\n') {
                _ = self.advance();
            } else {
                break;
            }
        }
    }

    /// Skip single-line comments (//)
    fn skipLineComment(self: *Tokenizer) void {
        while (self.peek()) |ch| {
            if (ch == '\n') break;
            _ = self.advance();
        }
    }

    /// Skip multi-line comments (/* */)
    fn skipBlockComment(self: *Tokenizer) !void {
        while (true) {
            const ch = self.peek() orelse return error.UnterminatedComment;
            if (ch == '*' and self.peekAhead(1) == '/') {
                _ = self.advance(); // consume *
                _ = self.advance(); // consume /
                break;
            }
            _ = self.advance();
        }
    }

    /// Skip all whitespace and comments
    fn skipWhitespaceAndComments(self: *Tokenizer) !void {
        while (true) {
            self.skipWhitespace();
            const ch = self.peek() orelse break;

            if (ch == '/' and self.peekAhead(1) == '/') {
                _ = self.advance();
                _ = self.advance();
                self.skipLineComment();
            } else if (ch == '/' and self.peekAhead(1) == '*') {
                _ = self.advance();
                _ = self.advance();
                try self.skipBlockComment();
            } else {
                break;
            }
        }
    }

    /// Check if character is a valid identifier start
    fn isIdentifierStart(ch: u8) bool {
        return (ch >= 'a' and ch <= 'z') or (ch >= 'A' and ch <= 'Z') or ch == '_';
    }

    /// Check if character is a valid identifier continuation
    fn isIdentifierContinue(ch: u8) bool {
        return isIdentifierStart(ch) or (ch >= '0' and ch <= '9');
    }

    /// Parse an identifier or keyword
    fn tokenizeIdentifier(self: *Tokenizer) !Token {
        const start_line = self.line;
        const start_col = self.column;
        const start_pos = self.pos;

        while (self.peek()) |ch| {
            if (!isIdentifierContinue(ch)) break;
            _ = self.advance();
        }

        const value = self.data[start_pos..self.pos];
        const position = Position.init(start_line, self.line, start_col, self.column);

        if (isKeyword(value)) {
            return Token.init(TokenType.TT_KEY, value, position);
        } else {
            return Token.init(TokenType.TT_IDN, value, position);
        }
    }

    /// Parse a numeric literal (integer or float)
    fn tokenizeNumber(self: *Tokenizer) !Token {
        const start_line = self.line;
        const start_col = self.column;
        const start_pos = self.pos;

        var is_float = false;

        // Check for hex, binary, octal prefixes
        if (self.peek() == '0') {
            const next = self.peekAhead(1);
            if (next == 'x' or next == 'X' or next == 'b' or next == 'B' or next == 'o' or next == 'O') {
                _ = self.advance(); // consume '0'
                _ = self.advance(); // consume prefix
                // Parse hex/binary/octal digits
                while (self.peek()) |ch| {
                    if ((ch >= '0' and ch <= '9') or
                        (ch >= 'a' and ch <= 'f') or
                        (ch >= 'A' and ch <= 'F') or
                        ch == '_')
                    {
                        _ = self.advance();
                    } else {
                        break;
                    }
                }
                const value = self.data[start_pos..self.pos];
                const position = Position.init(start_line, self.line, start_col, self.column);
                return Token.init(TokenType.TT_INT, value, position);
            }
        }

        // Parse decimal number
        while (self.peek()) |ch| {
            if (ch >= '0' and ch <= '9') {
                _ = self.advance();
            } else if (ch == '.') {
                if (is_float) break; // Second dot, stop
                is_float = true;
                _ = self.advance();
            } else if (ch == 'e' or ch == 'E') {
                is_float = true;
                _ = self.advance();
                // Handle optional sign
                if (self.peek()) |sign| {
                    if (sign == '+' or sign == '-') {
                        _ = self.advance();
                    }
                }
            } else if (ch == '_') {
                _ = self.advance(); // Allow underscores for readability
            } else {
                break;
            }
        }

        const value = self.data[start_pos..self.pos];
        const position = Position.init(start_line, self.line, start_col, self.column);

        if (is_float) {
            return Token.init(TokenType.TT_NUM, value, position);
        } else {
            return Token.init(TokenType.TT_INT, value, position);
        }
    }

    /// Parse a string literal
    /// Returns a slice into the source buffer (no allocation)
    /// Escape sequences are NOT processed here - they can be handled during evaluation
    fn tokenizeString(self: *Tokenizer) !Token {
        const start_line = self.line;
        const start_col = self.column;
        const start_pos = self.pos;
        const quote = self.advance().?; // consume opening quote

        // Scan to find the end of the string
        while (self.peek()) |ch| {
            if (ch == quote) {
                const end_pos = self.pos;
                _ = self.advance(); // consume closing quote
                
                // Return slice into original source buffer (no allocation)
                // Content excludes the quotes
                const position = Position.init(start_line, self.line, start_col, self.column);
                return Token.init(TokenType.TT_STR, self.data[start_pos + 1..end_pos], position);
            } else if (ch == '\\') {
                // Skip escape sequences (they'll be processed during evaluation if needed)
                _ = self.advance();
                if (self.peek()) |_| {
                    _ = self.advance();
                } else {
                    return error.UnterminatedString;
                }
            } else {
                _ = self.advance();
            }
        }

        return error.UnterminatedString;
    }

    /// Parse a symbol/operator
    fn tokenizeSymbol(self: *Tokenizer) Token {
        const start_line = self.line;
        const start_col = self.column;
        const start_pos = self.pos;

        const ch = self.advance().?;
        const next = self.peek();

        // Two-character operators
        const two_char: ?[]const u8 = switch (ch) {
            '=' => if (next == '=') "==" else null,
            '!' => if (next == '=') "!=" else null,
            '<' => if (next == '=') "<=" else if (next == '<') "<<" else null,
            '>' => if (next == '=') ">=" else if (next == '>') ">>" else null,
            '&' => if (next == '&') "&&" else null,
            '|' => if (next == '|') "||" else null,
            else => null,
        };

        if (two_char != null) {
            _ = self.advance(); // consume second character
        }

        const value = self.data[start_pos..self.pos];
        const position = Position.init(start_line, self.line, start_col, self.column);
        return Token.init(TokenType.TT_SYM, value, position);
    }

    /// Get the next token from the source
    pub fn nextToken(self: *Tokenizer) !Token {
        try self.skipWhitespaceAndComments();

        const ch = self.peek() orelse {
            const position = Position.init(self.line, self.line, self.column, self.column);
            return Token.init(TokenType.TT_EOF, "", position);
        };

        // Identifiers and keywords
        if (isIdentifierStart(ch)) {
            return try self.tokenizeIdentifier();
        }

        // Numbers
        if (ch >= '0' and ch <= '9') {
            return try self.tokenizeNumber();
        }

        // Strings
        if (ch == '"' or ch == '\'') {
            return try self.tokenizeString();
        }

        // Symbols and operators
        return self.tokenizeSymbol();
    }
};

