const std = @import("std");
const Position = @import("position.zig").Position;

/// AstType represents the different types of AST nodes
pub const AstType = enum {
    // Literals
    AstIdn, // Identifier
    AstInt, // Integer literal
    AstNum, // Float/double literal
    AstStr, // String literal
    AstTrue, // Boolean true
    AstFalse, // Boolean false
    AstNull, // Null literal

    // Arithmetic operators
    AstMul, // Multiplication (*)
    AstDiv, // Division (/)
    AstMod, // Modulo (%)
    AstAdd, // Addition (+)
    AstSub, // Subtraction (-)
    AstLShft, // Left shift (<<)
    AstRShft, // Right shift (>>)

    // Comparison operators
    AstLt, // Less than (<)
    AstLte, // Less than or equal (<=)
    AstGt, // Greater than (>)
    AstGte, // Greater than or equal (>=)
    AstEq, // Equal (==)
    AstNeq, // Not equal (!=)

    // Bitwise operators
    AstAnd, // Bitwise AND (&)
    AstOr, // Bitwise OR (|)
    AstXor, // Bitwise XOR (^)

    // Logical operators
    AstLAnd, // Logical AND (&&)
    AstLOr, // Logical OR (||)

    // Data structures
    AstArray, // Array literal [1, 2, 3]
    AstDict, // Dictionary/Object literal {"key": value}
    AstIndex, // Array/dict indexing: arr[0], dict["key"]
    AstKeyValue, // Key-value pair for dictionary entries

    // Complex structures
    AstFunc, // Function definition
    AstCall, // Function call
    AstBlock, // Block statement { }
    AstReturn, // Return statement
    AstBreak, // Break statement
    AstContinue, // Continue statement

    // Control flow
    AstIf, // If statement
    AstElse, // Else clause
    AstWhile, // While loop
    AstDoWhile, // Do-while loop
    AstFor, // For loop

    // Object-oriented
    AstClass, // Class definition
    AstEnum, // Enum definition
    AstMember, // Class member access (.)
    AstMethod, // Method definition

    // Statements
    AstVarDecl, // Variable declaration (let)
    AstAssign, // Assignment statement
    AstExprStmt, // Expression statement

    pub fn format(
        self: AstType,
        comptime fmt: []const u8,
        _: anytype,
        writer: anytype,
    ) !void {
        _ = fmt;
        try writer.writeAll(@tagName(self));
    }
};

/// Ast node represents a node in the abstract syntax tree
pub const Ast = struct {
    type: AstType,
    position: Position,
    value: []const u8, // For literals and identifiers
    A: ?*Ast, // First child (left operand, condition, etc.)
    B: ?*Ast, // Second child (right operand, body, etc.)
    C: ?*Ast, // Third child (else clause, increment, etc.)
    D: ?*Ast, // Fourth child (additional data)
    next: ?*Ast, // For linked lists (array elements, dict entries, etc.)
    allocator: std.mem.Allocator,

    /// Create a new AST node
    pub fn init(allocator: std.mem.Allocator, ast_type: AstType, position: Position) !*Ast {
        const node = try allocator.create(Ast);
        node.* = Ast{
            .type = ast_type,
            .position = position,
            .value = "",
            .A = null,
            .B = null,
            .C = null,
            .D = null,
            .next = null,
            .allocator = allocator,
        };
        return node;
    }

    /// Create a new AST node with a value
    pub fn initWithValue(
        allocator: std.mem.Allocator,
        ast_type: AstType,
        position: Position,
        value: []const u8,
    ) !*Ast {
        const node = try allocator.create(Ast);
        node.* = Ast{
            .type = ast_type,
            .position = position,
            .value = value,
            .A = null,
            .B = null,
            .C = null,
            .D = null,
            .next = null,
            .allocator = allocator,
        };
        return node;
    }

    /// Free the AST node and all its children
    pub fn deinit(self: *Ast) void {
        if (self.A) |a| a.deinit();
        if (self.B) |b| b.deinit();
        if (self.C) |c| c.deinit();
        if (self.D) |d| d.deinit();
        if (self.next) |next| next.deinit();
        self.allocator.destroy(self);
    }

    /// Pretty print the AST for debugging
    pub fn print(self: *const Ast, writer: anytype, indent: usize) !void {
        // Print indentation
        var i: usize = 0;
        while (i < indent) : (i += 1) {
            try writer.writeAll("  ");
        }

        // Print node type and value
        try writer.print("{s}", .{@tagName(self.type)});
        if (self.value.len > 0) {
            try writer.print("('{s}')", .{self.value});
        }
        try writer.writeAll("\n");

        // Print children
        if (self.A) |a| {
            try a.print(writer, indent + 1);
        }
        if (self.B) |b| {
            try b.print(writer, indent + 1);
        }
        if (self.C) |c| {
            try c.print(writer, indent + 1);
        }
        if (self.D) |d| {
            try d.print(writer, indent + 1);
        }
        if (self.next) |next| {
            try next.print(writer, indent);
        }
    }
};
