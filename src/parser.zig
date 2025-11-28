const std = @import("std");
const Token = @import("token.zig").Token;
const TokenType = @import("token.zig").TokenType;
const Tokenizer = @import("tokenizer.zig").Tokenizer;
const Ast = @import("ast.zig").Ast;
const AstType = @import("ast.zig").AstType;
const Position = @import("position.zig").Position;

/// Parser implements LL(1) recursive descent parsing
pub const Parser = struct {
    tokenizer: Tokenizer,
    current_token: Token,
    allocator: std.mem.Allocator,
    had_error: bool,

    /// Initialize a new parser
    pub fn init(allocator: std.mem.Allocator, tokenizer: Tokenizer) !Parser {
        var parser = Parser{
            .tokenizer = tokenizer,
            .current_token = undefined,
            .allocator = allocator,
            .had_error = false,
        };
        // Prime the parser with the first token
        parser.current_token = try parser.tokenizer.nextToken();
        return parser;
    }

    /// Parse the input and return the root AST node (program = statement sequence)
    pub fn parse(self: *Parser) anyerror!?*Ast {
        // Parse all statements until EOF
        const program_node = try Ast.init(self.allocator, AstType.AstBlock, self.current_token.position);

        var first_stmt: ?*Ast = null;
        var current_stmt: ?*Ast = null;

        while (self.current_token.type != TokenType.TT_EOF) {
            const stmt = try self.statement();
            if (stmt == null) {
                // Error recovery - skip to next statement boundary
                try self.synchronize();
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

        program_node.A = first_stmt;
        return program_node;
    }

    /// Error recovery: skip tokens until we reach a safe synchronization point
    fn synchronize(self: *Parser) !void {
        try self.advance();

        while (self.current_token.type != TokenType.TT_EOF) {
            // Semicolon marks end of statement
            if (self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ";")) {
                try self.advance(); // consume the semicolon
                return;
            }

            // Keywords that start new statements
            if (self.peek() == TokenType.TT_KEY) {
                const kw = self.current_token.value;
                if (std.mem.eql(u8, kw, "let") or
                    std.mem.eql(u8, kw, "fn") or
                    std.mem.eql(u8, kw, "class") or
                    std.mem.eql(u8, kw, "enum") or
                    std.mem.eql(u8, kw, "if") or
                    std.mem.eql(u8, kw, "while") or
                    std.mem.eql(u8, kw, "for") or
                    std.mem.eql(u8, kw, "do") or
                    std.mem.eql(u8, kw, "return"))
                {
                    return; // Don't consume the keyword, let next statement parse it
                }
            }

            try self.advance();
        }
    }

    // === LL(1) Helper Functions ===

    /// Consume current token and fetch next
    fn advance(self: *Parser) !void {
        self.current_token = try self.tokenizer.nextToken();
    }

    /// Return type of current lookahead token (non-destructive)
    fn peek(self: *Parser) TokenType {
        return self.current_token.type;
    }

    /// Check if current token matches expected type
    fn expect(self: *Parser, expected: TokenType) !bool {
        if (self.current_token.type != expected) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "Expected {s}, got {s}",
                .{ @tagName(expected), @tagName(self.current_token.type) },
            );
            defer self.allocator.free(msg);
            self.reportError(msg, self.current_token.position);
            return false;
        }
        try self.advance();
        return true;
    }

    /// Check if current token value matches expected string
    fn expectValue(self: *Parser, expected: []const u8) !bool {
        if (!std.mem.eql(u8, self.current_token.value, expected)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "Expected '{s}', got '{s}'",
                .{ expected, self.current_token.value },
            );
            defer self.allocator.free(msg);
            self.reportError(msg, self.current_token.position);
            return false;
        }
        try self.advance();
        return true;
    }

    /// Report a parsing error
    fn reportError(self: *Parser, message: []const u8, position: Position) void {
        std.debug.print("Parse Error: {s}\n", .{message});
        std.debug.print("  at {s}:{d}:{d}\n", .{ self.tokenizer.path, position.line_start, position.colm_start });
        self.had_error = true;
    }

    // === Statement Parsing Methods ===

    /// Statement dispatcher - routes to specific statement parsers based on lookahead
    fn statement(self: *Parser) anyerror!?*Ast {
        const token_type = self.peek();

        if (token_type == TokenType.TT_KEY) {
            const keyword = self.current_token.value;

            if (std.mem.eql(u8, keyword, "let")) {
                return try self.varDeclaration();
            } else if (std.mem.eql(u8, keyword, "if")) {
                return try self.ifStatement();
            } else if (std.mem.eql(u8, keyword, "while")) {
                return try self.whileStatement();
            } else if (std.mem.eql(u8, keyword, "do")) {
                return try self.doWhileStatement();
            } else if (std.mem.eql(u8, keyword, "for")) {
                return try self.forStatement();
            } else if (std.mem.eql(u8, keyword, "return")) {
                return try self.returnStatement();
            } else if (std.mem.eql(u8, keyword, "break")) {
                return try self.breakStatement();
            } else if (std.mem.eql(u8, keyword, "continue")) {
                return try self.continueStatement();
            } else if (std.mem.eql(u8, keyword, "fn")) {
                return try self.functionDeclaration();
            } else if (std.mem.eql(u8, keyword, "class")) {
                return try self.classDeclaration();
            } else if (std.mem.eql(u8, keyword, "enum")) {
                return try self.enumDeclaration();
            }
        } else if (token_type == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, "{")) {
            return try self.block();
        }

        // Default: expression statement
        return try self.expressionStatement();
    }

    /// Parse block: "{" statement* "}"
    fn block(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "{"

        const block_node = try Ast.init(self.allocator, AstType.AstBlock, position);
        var first_stmt: ?*Ast = null;
        var current_stmt: ?*Ast = null;

        while (self.peek() != TokenType.TT_SYM or !std.mem.eql(u8, self.current_token.value, "}")) {
            if (self.peek() == TokenType.TT_EOF) {
                self.reportError("Expected '}' before end of file", position);
                return null;
            }

            const stmt = try self.statement();
            if (stmt == null) continue; // Error recovery

            if (first_stmt == null) {
                first_stmt = stmt;
                current_stmt = stmt;
            } else {
                current_stmt.?.next = stmt;
                current_stmt = stmt;
            }
        }

        try self.advance(); // consume "}"
        block_node.A = first_stmt;
        return block_node;
    }

    /// Parse expression statement: expression ";"
    fn expressionStatement(self: *Parser) anyerror!?*Ast {
        const expr = try self.assignment();
        if (expr == null) return null;

        if (!(try self.expectValue(";"))) {
            return null;
        }

        const stmt_node = try Ast.init(self.allocator, AstType.AstExprStmt, expr.?.position);
        stmt_node.A = expr;
        return stmt_node;
    }

    /// Parse variable declaration: "let" identifier ("=" expression)? ";"
    fn varDeclaration(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "let"

        if (self.peek() != TokenType.TT_IDN) {
            self.reportError("Expected identifier after 'let'", position);
            return null;
        }
        const var_name = self.current_token.value;
        const var_position = self.current_token.position;
        try self.advance(); // consume identifier

        var initializer: ?*Ast = null;
        if (self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, "=")) {
            try self.advance(); // consume "="
            initializer = try self.assignment();
            if (initializer == null) {
                self.reportError("Expected expression after '='", var_position);
                return null;
            }
        }

        if (!(try self.expectValue(";"))) {
            return null;
        }

        const var_node = try Ast.initWithValue(self.allocator, AstType.AstVarDecl, position, var_name);
        var_node.A = initializer;
        return var_node;
    }

    /// Parse if statement: "if" "(" expression ")" block ("else" (ifStatement | block))?
    fn ifStatement(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "if"

        if (!(try self.expectValue("("))) return null;

        const condition = try self.assignment();
        if (condition == null) {
            self.reportError("Expected condition in if statement", position);
            return null;
        }

        if (!(try self.expectValue(")"))) return null;

        const then_block = try self.block();
        if (then_block == null) return null;

        var else_clause: ?*Ast = null;
        if (self.peek() == TokenType.TT_KEY and std.mem.eql(u8, self.current_token.value, "else")) {
            try self.advance(); // consume "else"

            // Check for else-if or else
            if (self.peek() == TokenType.TT_KEY and std.mem.eql(u8, self.current_token.value, "if")) {
                else_clause = try self.ifStatement(); // Recursive: else-if
            } else {
                else_clause = try self.block(); // Final else
            }
        }

        const if_node = try Ast.init(self.allocator, AstType.AstIf, position);
        if_node.A = condition;
        if_node.B = then_block;
        if_node.C = else_clause;
        return if_node;
    }

    /// Parse while statement: "while" "(" expression ")" block
    fn whileStatement(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "while"

        if (!(try self.expectValue("("))) return null;

        const condition = try self.assignment();
        if (condition == null) {
            self.reportError("Expected condition in while statement", position);
            return null;
        }

        if (!(try self.expectValue(")"))) return null;

        const body = try self.block();
        if (body == null) return null;

        const while_node = try Ast.init(self.allocator, AstType.AstWhile, position);
        while_node.A = condition;
        while_node.B = body;
        return while_node;
    }

    /// Parse do-while statement: "do" block "while" "(" expression ")" ";"
    fn doWhileStatement(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "do"

        const body = try self.block();
        if (body == null) return null;

        if (!(try self.expectValue("while"))) return null;
        if (!(try self.expectValue("("))) return null;

        const condition = try self.assignment();
        if (condition == null) {
            self.reportError("Expected condition in do-while statement", position);
            return null;
        }

        if (!(try self.expectValue(")"))) return null;
        if (!(try self.expectValue(";"))) return null;

        const do_while_node = try Ast.init(self.allocator, AstType.AstDoWhile, position);
        do_while_node.A = body;
        do_while_node.B = condition;
        return do_while_node;
    }

    /// Parse for statement: "for" "(" forInit ";" forCondition ";" forIncrement ")" block
    fn forStatement(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "for"

        if (!(try self.expectValue("("))) return null;

        // Parse initializer (optional)
        var initializer: ?*Ast = null;
        if (self.peek() == TokenType.TT_KEY and std.mem.eql(u8, self.current_token.value, "let")) {
            initializer = try self.varDeclaration();
            // varDeclaration consumes the semicolon
        } else if (!(self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ";"))) {
            initializer = try self.assignment();
            if (!(try self.expectValue(";"))) return null;
        } else {
            try self.advance(); // consume ";"
        }

        // Parse condition (optional)
        var condition: ?*Ast = null;
        if (!(self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ";"))) {
            condition = try self.assignment();
        }
        if (!(try self.expectValue(";"))) return null;

        // Parse increment (optional)
        var increment: ?*Ast = null;
        if (!(self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ")"))) {
            increment = try self.assignment();
        }

        if (!(try self.expectValue(")"))) return null;

        const body = try self.block();
        if (body == null) return null;

        // Build for loop node: A=initializer, B=condition, C=increment, D=body
        const for_node = try Ast.init(self.allocator, AstType.AstFor, position);
        for_node.A = initializer;
        for_node.B = condition;
        for_node.C = increment;
        for_node.D = body;
        return for_node;
    }

    /// Parse return statement: "return" expression? ";"
    fn returnStatement(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "return"

        var return_value: ?*Ast = null;
        if (!(self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ";"))) {
            return_value = try self.assignment();
        }

        if (!(try self.expectValue(";"))) return null;

        const return_node = try Ast.init(self.allocator, AstType.AstReturn, position);
        return_node.A = return_value;
        return return_node;
    }

    /// Parse break statement: "break" ";"
    fn breakStatement(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "break"

        if (!(try self.expectValue(";"))) return null;

        return try Ast.init(self.allocator, AstType.AstBreak, position);
    }

    /// Parse continue statement: "continue" ";"
    fn continueStatement(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "continue"

        if (!(try self.expectValue(";"))) return null;

        return try Ast.init(self.allocator, AstType.AstContinue, position);
    }

    /// Parse function declaration: "fn" identifier "(" parameters ")" block
    fn functionDeclaration(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "fn"

        if (self.peek() != TokenType.TT_IDN) {
            self.reportError("Expected identifier after 'fn'", position);
            return null;
        }
        const func_name = self.current_token.value;
        try self.advance(); // consume identifier

        if (!(try self.expectValue("("))) return null;

        // Parse parameters
        var first_param: ?*Ast = null;
        var current_param: ?*Ast = null;

        if (!(self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ")"))) {
            // Parse first parameter
            if (self.peek() != TokenType.TT_IDN) {
                self.reportError("Expected parameter name", position);
                return null;
            }
            first_param = try Ast.initWithValue(self.allocator, AstType.AstIdn, self.current_token.position, self.current_token.value);
            current_param = first_param;
            try self.advance(); // consume identifier

            // Parse remaining parameters
            while (self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ",")) {
                try self.advance(); // consume ","
                if (self.peek() != TokenType.TT_IDN) {
                    self.reportError("Expected parameter name", position);
                    return null;
                }

                const param = try Ast.initWithValue(self.allocator, AstType.AstIdn, self.current_token.position, self.current_token.value);
                current_param.?.next = param;
                current_param = param;
                try self.advance(); // consume identifier
            }
        }

        if (!(try self.expectValue(")"))) return null;

        const body = try self.block();
        if (body == null) return null;

        const func_node = try Ast.initWithValue(self.allocator, AstType.AstFunc, position, func_name);
        func_node.A = first_param;
        func_node.B = body;
        return func_node;
    }

    /// Parse class declaration: "class" identifier "{" classMember* "}"
    fn classDeclaration(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "class"

        if (self.peek() != TokenType.TT_IDN) {
            self.reportError("Expected identifier after 'class'", position);
            return null;
        }
        const class_name = self.current_token.value;
        try self.advance(); // consume identifier

        if (!(try self.expectValue("{"))) return null;

        // Parse class members
        var first_member: ?*Ast = null;
        var current_member: ?*Ast = null;

        while (!(self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, "}"))) {
            if (self.peek() == TokenType.TT_EOF) {
                self.reportError("Expected '}' before end of file", position);
                return null;
            }

            var member: ?*Ast = null;

            // Check for method or field
            if (self.peek() == TokenType.TT_KEY and std.mem.eql(u8, self.current_token.value, "fn")) {
                member = try self.functionDeclaration();
            } else if (self.peek() == TokenType.TT_IDN) {
                // Field declaration: identifier "=" expression ";"
                const field_pos = self.current_token.position;
                const field_name = self.current_token.value;
                try self.advance();

                if (!(try self.expectValue("="))) return null;

                const init_expr = try self.assignment();
                if (init_expr == null) return null;

                if (!(try self.expectValue(";"))) return null;

                member = try Ast.initWithValue(self.allocator, AstType.AstVarDecl, field_pos, field_name);
                member.?.A = init_expr;
            } else {
                self.reportError("Expected method or field in class", self.current_token.position);
                return null;
            }

            if (first_member == null) {
                first_member = member;
                current_member = member;
            } else {
                current_member.?.next = member;
                current_member = member;
            }
        }

        if (!(try self.expectValue("}"))) return null;

        const class_node = try Ast.initWithValue(self.allocator, AstType.AstClass, position, class_name);
        class_node.A = first_member;
        return class_node;
    }

    /// Parse enum declaration: "enum" identifier "{" enumValue ("," enumValue)* ","? "}"
    fn enumDeclaration(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume "enum"

        if (self.peek() != TokenType.TT_IDN) {
            self.reportError("Expected identifier after 'enum'", position);
            return null;
        }
        const enum_name = self.current_token.value;
        try self.advance(); // consume identifier

        if (!(try self.expectValue("{"))) return null;

        // Parse enum values
        var first_value: ?*Ast = null;
        var current_value: ?*Ast = null;

        if (!(self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, "}"))) {
            // Parse first value
            if (self.peek() != TokenType.TT_IDN) {
                self.reportError("Expected enum value", position);
                return null;
            }
            first_value = try Ast.initWithValue(self.allocator, AstType.AstIdn, self.current_token.position, self.current_token.value);
            current_value = first_value;
            try self.advance(); // consume identifier

            // Parse remaining values
            while (self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ",")) {
                try self.advance(); // consume ","

                // Allow trailing comma
                if (self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, "}")) {
                    break;
                }

                if (self.peek() != TokenType.TT_IDN) {
                    self.reportError("Expected enum value", position);
                    return null;
                }
                const value = try Ast.initWithValue(self.allocator, AstType.AstIdn, self.current_token.position, self.current_token.value);
                current_value.?.next = value;
                current_value = value;
                try self.advance(); // consume identifier
            }
        }

        if (!(try self.expectValue("}"))) return null;

        const enum_node = try Ast.initWithValue(self.allocator, AstType.AstEnum, position, enum_name);
        enum_node.A = first_value;
        return enum_node;
    }

    // === Expression Parsing Methods ===
    // Each method corresponds to a grammar non-terminal

    /// Assignment (lowest precedence expression)
    fn assignment(self: *Parser) !?*Ast {
        const left = try self.logicalOr();
        if (left == null) return null;

        if (self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, "=")) {
            const position = self.current_token.position;
            try self.advance(); // consume "="

            const right = try self.assignment(); // Right-associative
            if (right == null) {
                self.reportError("Expected expression after '='", position);
                return null;
            }

            const assign_node = try Ast.init(self.allocator, AstType.AstAssign, position);
            assign_node.A = left;
            assign_node.B = right;
            return assign_node;
        }

        return left;
    }

    /// Top-level expression parsing
    fn expression(self: *Parser) !?*Ast {
        return try self.assignment();
    }

    /// Precedence level 11: ||
    fn logicalOr(self: *Parser) !?*Ast {
        var left = try self.logicalAnd();
        if (left == null) return null;

        while (self.peek() == TokenType.TT_SYM and
            std.mem.eql(u8, self.current_token.value, "||"))
        {
            const position = self.current_token.position;
            try self.advance();

            const right = try self.logicalAnd();
            if (right == null) {
                self.reportError("Expected expression after '||'", position);
                return null;
            }

            const node = try Ast.init(self.allocator, AstType.AstLOr, position);
            node.A = left;
            node.B = right;
            left = node;
        }

        return left;
    }

    /// Precedence level 10: &&
    fn logicalAnd(self: *Parser) !?*Ast {
        var left = try self.bitwiseOr();
        if (left == null) return null;

        while (self.peek() == TokenType.TT_SYM and
            std.mem.eql(u8, self.current_token.value, "&&"))
        {
            const position = self.current_token.position;
            try self.advance();

            const right = try self.bitwiseOr();
            if (right == null) {
                self.reportError("Expected expression after '&&'", position);
                return null;
            }

            const node = try Ast.init(self.allocator, AstType.AstLAnd, position);
            node.A = left;
            node.B = right;
            left = node;
        }

        return left;
    }

    /// Precedence level 9: |
    fn bitwiseOr(self: *Parser) !?*Ast {
        var left = try self.bitwiseXor();
        if (left == null) return null;

        while (self.peek() == TokenType.TT_SYM and
            std.mem.eql(u8, self.current_token.value, "|"))
        {
            const position = self.current_token.position;
            try self.advance();

            const right = try self.bitwiseXor();
            if (right == null) {
                self.reportError("Expected expression after '|'", position);
                return null;
            }

            const node = try Ast.init(self.allocator, AstType.AstOr, position);
            node.A = left;
            node.B = right;
            left = node;
        }

        return left;
    }

    /// Precedence level 8: ^
    fn bitwiseXor(self: *Parser) !?*Ast {
        var left = try self.bitwiseAnd();
        if (left == null) return null;

        while (self.peek() == TokenType.TT_SYM and
            std.mem.eql(u8, self.current_token.value, "^"))
        {
            const position = self.current_token.position;
            try self.advance();

            const right = try self.bitwiseAnd();
            if (right == null) {
                self.reportError("Expected expression after '^'", position);
                return null;
            }

            const node = try Ast.init(self.allocator, AstType.AstXor, position);
            node.A = left;
            node.B = right;
            left = node;
        }

        return left;
    }

    /// Precedence level 7: &
    fn bitwiseAnd(self: *Parser) !?*Ast {
        var left = try self.equality();
        if (left == null) return null;

        while (self.peek() == TokenType.TT_SYM and
            std.mem.eql(u8, self.current_token.value, "&"))
        {
            const position = self.current_token.position;
            try self.advance();

            const right = try self.equality();
            if (right == null) {
                self.reportError("Expected expression after '&'", position);
                return null;
            }

            const node = try Ast.init(self.allocator, AstType.AstAnd, position);
            node.A = left;
            node.B = right;
            left = node;
        }

        return left;
    }

    /// Precedence level 6: ==, !=
    fn equality(self: *Parser) !?*Ast {
        var left = try self.comparison();
        if (left == null) return null;

        while (self.peek() == TokenType.TT_SYM) {
            const op = self.current_token.value;
            if (std.mem.eql(u8, op, "==") or std.mem.eql(u8, op, "!=")) {
                const position = self.current_token.position;
                try self.advance();

                const right = try self.comparison();
                if (right == null) {
                    self.reportError("Expected expression after comparison operator", position);
                    return null;
                }

                const ast_type = if (std.mem.eql(u8, op, "==")) AstType.AstEq else AstType.AstNeq;
                const node = try Ast.init(self.allocator, ast_type, position);
                node.A = left;
                node.B = right;
                left = node;
            } else {
                break;
            }
        }

        return left;
    }

    /// Precedence level 5: <, <=, >, >=
    fn comparison(self: *Parser) !?*Ast {
        var left = try self.shift();
        if (left == null) return null;

        while (self.peek() == TokenType.TT_SYM) {
            const op = self.current_token.value;
            const ast_type: ?AstType = if (std.mem.eql(u8, op, "<"))
                AstType.AstLt
            else if (std.mem.eql(u8, op, "<="))
                AstType.AstLte
            else if (std.mem.eql(u8, op, ">"))
                AstType.AstGt
            else if (std.mem.eql(u8, op, ">="))
                AstType.AstGte
            else
                null;

            if (ast_type) |at| {
                const position = self.current_token.position;
                try self.advance();

                const right = try self.shift();
                if (right == null) {
                    self.reportError("Expected expression after comparison operator", position);
                    return null;
                }

                const node = try Ast.init(self.allocator, at, position);
                node.A = left;
                node.B = right;
                left = node;
            } else {
                break;
            }
        }

        return left;
    }

    /// Precedence level 4: <<, >>
    fn shift(self: *Parser) !?*Ast {
        var left = try self.additive();
        if (left == null) return null;

        while (self.peek() == TokenType.TT_SYM) {
            const op = self.current_token.value;
            if (std.mem.eql(u8, op, "<<") or std.mem.eql(u8, op, ">>")) {
                const position = self.current_token.position;
                try self.advance();

                const right = try self.additive();
                if (right == null) {
                    self.reportError("Expected expression after shift operator", position);
                    return null;
                }

                const ast_type = if (std.mem.eql(u8, op, "<<")) AstType.AstLShft else AstType.AstRShft;
                const node = try Ast.init(self.allocator, ast_type, position);
                node.A = left;
                node.B = right;
                left = node;
            } else {
                break;
            }
        }

        return left;
    }

    /// Precedence level 3: +, -
    fn additive(self: *Parser) !?*Ast {
        var left = try self.multiplicative();
        if (left == null) return null;

        while (self.peek() == TokenType.TT_SYM) {
            const op = self.current_token.value;
            if (std.mem.eql(u8, op, "+") or std.mem.eql(u8, op, "-")) {
                const position = self.current_token.position;
                try self.advance();

                const right = try self.multiplicative();
                if (right == null) {
                    self.reportError("Expected expression after arithmetic operator", position);
                    return null;
                }

                const ast_type = if (std.mem.eql(u8, op, "+")) AstType.AstAdd else AstType.AstSub;
                const node = try Ast.init(self.allocator, ast_type, position);
                node.A = left;
                node.B = right;
                left = node;
            } else {
                break;
            }
        }

        return left;
    }

    /// Precedence level 2: *, /, %
    fn multiplicative(self: *Parser) !?*Ast {
        var left = try self.postfix();
        if (left == null) return null;

        while (self.peek() == TokenType.TT_SYM) {
            const op = self.current_token.value;
            const ast_type: ?AstType = if (std.mem.eql(u8, op, "*"))
                AstType.AstMul
            else if (std.mem.eql(u8, op, "/"))
                AstType.AstDiv
            else if (std.mem.eql(u8, op, "%"))
                AstType.AstMod
            else
                null;

            if (ast_type) |at| {
                const position = self.current_token.position;
                try self.advance();

                const right = try self.postfix();
                if (right == null) {
                    self.reportError("Expected expression after arithmetic operator", position);
                    return null;
                }

                const node = try Ast.init(self.allocator, at, position);
                node.A = left;
                node.B = right;
                left = node;
            } else {
                break;
            }
        }

        return left;
    }

    /// Precedence level 1: array/dict indexing, function calls, member access
    fn postfix(self: *Parser) anyerror!?*Ast {
        var base = try self.primary();
        if (base == null) return null;

        // Handle all postfix operators in a loop
        while (true) {
            if (self.peek() == TokenType.TT_SYM) {
                const sym = self.current_token.value;

                if (std.mem.eql(u8, sym, "[")) {
                    // Array/dictionary indexing: base[index]
                    const position = self.current_token.position;
                    try self.advance(); // consume '['

                    const index = try self.assignment();
                    if (index == null) {
                        self.reportError("Expected expression inside brackets", position);
                        return null;
                    }

                    if (!(try self.expectValue("]"))) {
                        return null;
                    }

                    const node = try Ast.init(self.allocator, AstType.AstIndex, position);
                    node.A = base;
                    node.B = index;
                    base = node;
                } else if (std.mem.eql(u8, sym, "(")) {
                    // Function call: base(args)
                    base = try self.functionCall(base.?);
                    if (base == null) return null;
                } else if (std.mem.eql(u8, sym, ".")) {
                    // Member access: base.member
                    const position = self.current_token.position;
                    try self.advance(); // consume '.'

                    if (self.peek() != TokenType.TT_IDN) {
                        self.reportError("Expected identifier after '.'", position);
                        return null;
                    }

                    const member_name = self.current_token.value;
                    try self.advance(); // consume identifier

                    const node = try Ast.initWithValue(self.allocator, AstType.AstMember, position, member_name);
                    node.A = base;
                    base = node;
                } else {
                    break; // No more postfix operators
                }
            } else {
                break; // Not a symbol, no postfix operator
            }
        }

        return base;
    }

    /// Helper function for parsing function calls
    fn functionCall(self: *Parser, callee: *Ast) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume '('

        const call_node = try Ast.init(self.allocator, AstType.AstCall, position);
        call_node.A = callee; // Function being called

        // Check for empty argument list
        if (self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ")")) {
            try self.advance(); // consume ')'
            return call_node;
        }

        // Parse first argument
        const first_arg = try self.assignment();
        if (first_arg == null) {
            self.reportError("Expected expression in argument list", position);
            return null;
        }

        var current_arg = first_arg;
        call_node.B = first_arg; // First argument

        // Parse remaining arguments
        while (self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ",")) {
            try self.advance(); // consume ','

            // Allow trailing comma
            if (self.peek() == TokenType.TT_SYM and std.mem.eql(u8, self.current_token.value, ")")) {
                break;
            }

            const arg = try self.assignment();
            if (arg == null) {
                self.reportError("Expected expression after comma", position);
                return null;
            }

            current_arg.?.next = arg; // Link arguments
            current_arg = arg;
        }

        if (!(try self.expectValue(")"))) return null;

        return call_node;
    }

    /// Precedence level 0: atomic/base expressions
    fn primary(self: *Parser) anyerror!?*Ast {
        const token_type = self.peek();
        const position = self.current_token.position;

        switch (token_type) {
            TokenType.TT_IDN => {
                const value = self.current_token.value;
                try self.advance();
                return try Ast.initWithValue(self.allocator, AstType.AstIdn, position, value);
            },
            TokenType.TT_INT => {
                const value = self.current_token.value;
                try self.advance();
                return try Ast.initWithValue(self.allocator, AstType.AstInt, position, value);
            },
            TokenType.TT_NUM => {
                const value = self.current_token.value;
                try self.advance();
                return try Ast.initWithValue(self.allocator, AstType.AstNum, position, value);
            },
            TokenType.TT_STR => {
                const value = self.current_token.value;
                try self.advance();
                return try Ast.initWithValue(self.allocator, AstType.AstStr, position, value);
            },
            TokenType.TT_KEY => {
                const value = self.current_token.value;
                if (std.mem.eql(u8, value, "true")) {
                    try self.advance();
                    return try Ast.init(self.allocator, AstType.AstTrue, position);
                } else if (std.mem.eql(u8, value, "false")) {
                    try self.advance();
                    return try Ast.init(self.allocator, AstType.AstFalse, position);
                } else if (std.mem.eql(u8, value, "null")) {
                    try self.advance();
                    return try Ast.init(self.allocator, AstType.AstNull, position);
                }
                self.reportError("Unexpected keyword in expression", position);
                return null;
            },
            TokenType.TT_SYM => {
                const value = self.current_token.value;
                if (std.mem.eql(u8, value, "[")) {
                    return try self.array();
                } else if (std.mem.eql(u8, value, "{")) {
                    return try self.dictionary();
                } else if (std.mem.eql(u8, value, "(")) {
                    try self.advance(); // consume '('
                    const expr = try self.expression();
                    if (!(try self.expectValue(")"))) {
                        return null;
                    }
                    return expr;
                }
                self.reportError("Unexpected symbol in expression", position);
                return null;
            },
            else => {
                self.reportError("Unexpected token in expression", position);
                return null;
            },
        }
    }

    /// Parse array literals
    fn array(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume '['

        const array_node = try Ast.init(self.allocator, AstType.AstArray, position);

        // Check for empty array
        if (self.peek() == TokenType.TT_SYM and
            std.mem.eql(u8, self.current_token.value, "]"))
        {
            try self.advance(); // consume ']'
            return array_node;
        }

        // Parse first element
        const first = try self.expression();
        if (first == null) {
            self.reportError("Expected expression in array", position);
            return null;
        }

        var current = first;
        array_node.A = first;

        // Parse remaining elements
        while (self.peek() == TokenType.TT_SYM and
            std.mem.eql(u8, self.current_token.value, ","))
        {
            try self.advance(); // consume ','

            // Allow trailing comma
            if (self.peek() == TokenType.TT_SYM and
                std.mem.eql(u8, self.current_token.value, "]"))
            {
                break;
            }

            const elem = try self.expression();
            if (elem == null) {
                self.reportError("Expected expression after comma", position);
                return null;
            }

            current.?.next = elem;
            current = elem;
        }

        if (!(try self.expectValue("]"))) {
            return null;
        }

        return array_node;
    }

    /// Parse dictionary literals
    fn dictionary(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;
        try self.advance(); // consume '{'

        const dict_node = try Ast.init(self.allocator, AstType.AstDict, position);

        // Check for empty dictionary
        if (self.peek() == TokenType.TT_SYM and
            std.mem.eql(u8, self.current_token.value, "}"))
        {
            try self.advance(); // consume '}'
            return dict_node;
        }

        // Parse first key-value pair
        const first = try self.keyValue();
        if (first == null) {
            self.reportError("Expected key-value pair in dictionary", position);
            return null;
        }

        var current = first;
        dict_node.A = first;

        // Parse remaining key-value pairs
        while (self.peek() == TokenType.TT_SYM and
            std.mem.eql(u8, self.current_token.value, ","))
        {
            try self.advance(); // consume ','

            // Allow trailing comma
            if (self.peek() == TokenType.TT_SYM and
                std.mem.eql(u8, self.current_token.value, "}"))
            {
                break;
            }

            const entry = try self.keyValue();
            if (entry == null) {
                self.reportError("Expected key-value pair after comma", position);
                return null;
            }

            current.?.next = entry;
            current = entry;
        }

        if (!(try self.expectValue("}"))) {
            return null;
        }

        return dict_node;
    }

    /// Parse dictionary key-value pair
    fn keyValue(self: *Parser) anyerror!?*Ast {
        const position = self.current_token.position;

        // Key can be string or identifier
        const key = if (self.peek() == TokenType.TT_STR or self.peek() == TokenType.TT_IDN)
            self.current_token.value
        else {
            self.reportError("Expected string or identifier as dictionary key", position);
            return null;
        };
        try self.advance();

        if (!(try self.expectValue(":"))) {
            return null;
        }

        const value_expr = try self.expression();
        if (value_expr == null) {
            self.reportError("Expected value in dictionary entry", position);
            return null;
        }

        const kv_node = try Ast.initWithValue(self.allocator, AstType.AstKeyValue, position, key);
        kv_node.A = value_expr;

        return kv_node;
    }
};
