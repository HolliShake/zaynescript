const std = @import("std");
const Tokenizer = @import("tokenizer.zig").Tokenizer;
const Parser = @import("parser.zig").Parser;
const Ast = @import("ast.zig").Ast;
const Token = @import("token.zig").Token;

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    if (args.len < 2) {
        std.debug.print("Usage: {s} <source-file>\n", .{args[0]});
        std.debug.print("   or: {s} -e <expression>\n", .{args[0]});
        return;
    }

    // Check for expression mode
    if (std.mem.eql(u8, args[1], "-e")) {
        if (args.len < 3) {
            std.debug.print("Error: -e flag requires an expression\n", .{});
            return;
        }
        try runExpression(allocator, args[2]);
        return;
    }

    // File mode
    try runFile(allocator, args[1]);
}

fn runFile(allocator: std.mem.Allocator, path: []const u8) !void {
    // Read the source file using absolute path
    const source = try std.fs.cwd().readFileAlloc(path, allocator, @enumFromInt(1024 * 1024));
    defer allocator.free(source);

    std.debug.print("Parsing file: {s}\n", .{path});
    std.debug.print("Source length: {d} bytes\n\n", .{source.len});

    try parseAndPrint(allocator, path, source);
}

fn runExpression(allocator: std.mem.Allocator, expr: []const u8) !void {
    std.debug.print("Parsing expression: {s}\n\n", .{expr});
    try parseAndPrint(allocator, "<expression>", expr);
}

fn parseAndPrint(allocator: std.mem.Allocator, path: []const u8, source: []const u8) !void {
    // Create tokenizer
    var tokenizer = Tokenizer.init(allocator, path, source);

    // Tokenization phase
    std.debug.print("=== TOKENIZATION ===\n", .{});
    var token_count: usize = 0;

    while (true) {
        const token = try tokenizer.nextToken();
        std.debug.print("Token {d}: {s}\n", .{ token_count + 1, @tagName(token.type) });
        token_count += 1;

        if (token.type == @import("token.zig").TokenType.TT_EOF) break;
    }
    std.debug.print("\nTotal tokens: {d}\n\n", .{token_count});

    // Reset tokenizer for parsing
    tokenizer = Tokenizer.init(allocator, path, source);

    // Parsing phase
    std.debug.print("=== PARSING ===\n", .{});
    var parser = try Parser.init(allocator, tokenizer);

    const ast = try parser.parse();
    if (ast) |root| {
        defer root.deinit();

        if (!parser.had_error) {
            std.debug.print("Parsing successful!\n\n", .{});
            std.debug.print("=== AST ===\n", .{});
            std.debug.print("AST root node type: {s}\n\n", .{@tagName(root.type)});
        } else {
            std.debug.print("\nParsing completed with errors.\n", .{});
        }
    } else {
        std.debug.print("Parsing failed!\n", .{});
    }
}

// Tests
test "simple expression" {
    const allocator = std.testing.allocator;
    const source = "3 + 4 * 5";

    const tokenizer = Tokenizer.init(allocator, "<test>", source);
    var parser = try Parser.init(allocator, tokenizer);

    const ast = try parser.parse();
    try std.testing.expect(ast != null);
    if (ast) |root| {
        defer root.deinit();
        try std.testing.expect(!parser.had_error);
    }
}

test "array literal" {
    const allocator = std.testing.allocator;
    const source = "[1, 2, 3]";

    const tokenizer = Tokenizer.init(allocator, "<test>", source);
    var parser = try Parser.init(allocator, tokenizer);

    const ast = try parser.parse();
    try std.testing.expect(ast != null);
    if (ast) |root| {
        defer root.deinit();
        try std.testing.expect(!parser.had_error);
        try std.testing.expect(root.type == @import("ast.zig").AstType.AstArray);
    }
}

test "dictionary literal" {
    const allocator = std.testing.allocator;
    const source = "{\"key\": 42}";

    const tokenizer = Tokenizer.init(allocator, "<test>", source);
    var parser = try Parser.init(allocator, tokenizer);

    const ast = try parser.parse();
    try std.testing.expect(ast != null);
    if (ast) |root| {
        defer root.deinit();
        try std.testing.expect(!parser.had_error);
        try std.testing.expect(root.type == @import("ast.zig").AstType.AstDict);
    }
}

test "array indexing" {
    const allocator = std.testing.allocator;
    const source = "arr[0]";

    const tokenizer = Tokenizer.init(allocator, "<test>", source);
    var parser = try Parser.init(allocator, tokenizer);

    const ast = try parser.parse();
    try std.testing.expect(ast != null);
    if (ast) |root| {
        defer root.deinit();
        try std.testing.expect(!parser.had_error);
        try std.testing.expect(root.type == @import("ast.zig").AstType.AstIndex);
    }
}

test "nested array indexing" {
    const allocator = std.testing.allocator;
    const source = "matrix[i][j]";

    const tokenizer = Tokenizer.init(allocator, "<test>", source);
    var parser = try Parser.init(allocator, tokenizer);

    const ast = try parser.parse();
    try std.testing.expect(ast != null);
    if (ast) |root| {
        defer root.deinit();
        try std.testing.expect(!parser.had_error);
        try std.testing.expect(root.type == @import("ast.zig").AstType.AstIndex);
    }
}

test "complex expression" {
    const allocator = std.testing.allocator;
    const source = "3 + 4 * 5 - 6 / 2";

    const tokenizer = Tokenizer.init(allocator, "<test>", source);
    var parser = try Parser.init(allocator, tokenizer);

    const ast = try parser.parse();
    try std.testing.expect(ast != null);
    if (ast) |root| {
        defer root.deinit();
        try std.testing.expect(!parser.had_error);
    }
}

