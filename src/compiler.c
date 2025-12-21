#include "./compiler.h"

#define PushArray(type, array, count, val, defaultValue) do { \
    (array)[(count)] = val; \
    count++; \
    (array) = Reallocate((array), sizeof(type) * ((count) + 1)); \
    (array)[(count)] = (defaultValue); \
} while(0)

#define GetOffset() (compiler->Interpreter->ConstantC)

Compiler* CreateCompiler(Interpreter* interpreter, Parser* parser) {
    Compiler* compiler    = Allocate(sizeof(Compiler));
    compiler->Interpreter = interpreter;
    compiler->Parser      = parser;
    return compiler;
}

static bool _IsAstConstant(Compiler* compiler, Ast* node) {
    switch (node->Type) {
        case AST_INT:
        case AST_NUM:
        case AST_STR:
            return 1;
        case AST_MUL:
        case AST_DIV:
        case AST_MOD:
        case AST_ADD:
        case AST_SUB:
            return _IsAstConstant(compiler, node->A) && _IsAstConstant(compiler, node->B);
        default:
            return 0;
    }
}

static int _SaveFunction(Compiler* compiler, Value* fn) {
    int offset = compiler->Interpreter->FunctionC;
    PushArray(
        Value*,
        compiler->Interpreter->Functions,
        compiler->Interpreter->FunctionC,
        fn,
        NULL
    );
    return offset;
}

static int _SaveInt(Compiler* compiler, int val) {
    int offset = GetOffset();
    PushArray(
        Value*,
        compiler->Interpreter->Constants, 
        compiler->Interpreter->ConstantC, 
        NewIntValue(compiler->Interpreter, val), 
        NULL
    );
    return offset;
}

static int _SaveNum(Compiler* compiler, double val) {
    int offset = GetOffset();
    PushArray(
        Value*,
        compiler->Interpreter->Constants, 
        compiler->Interpreter->ConstantC, 
        NewNumValue(compiler->Interpreter, val), 
        NULL
    );
    return offset;
}

static int _SaveStr(Compiler* compiler, String val) {
    int offset = GetOffset();
    PushArray(
        Value*,
        compiler->Interpreter->Constants, 
        compiler->Interpreter->ConstantC, 
        NewStrValue(compiler->Interpreter, val), 
        NULL
    );
    return offset;
}

static int _GetConstant(Compiler* compiler, String str) {
    // FLG_NOTFOUND if not found
    for (int i = 0; i < compiler->Interpreter->ConstantC; i++) {
        Value* constant = compiler->Interpreter->Constants[i];
        if (constant == NULL) continue;
        
        String constantStr = ValueToString(constant);
        if (constantStr != NULL && strcmp(constantStr, str) == 0) {
            free(constantStr);
            return i;
        }
        if (constantStr != NULL) {
            free(constantStr);
        }
    }
    return FLG_NOTFOUND;
}

static Value* _GetConstantValue(Compiler* compiler, int offset) {
    if (offset == FLG_NOTFOUND) {
        return NULL;
    }
    return compiler->Interpreter->Constants[offset];
}

static void _Emit(Compiler* compiler, UserFunction* uf, OpcodeEnum opcode) {
    PushArray(
        uint8_t,
        uf->Codes, 
        uf->CodeC, 
        opcode, 
        0
    );
}

static void _EmitConst(Compiler* compiler, UserFunction* uf, OpcodeEnum opcode, int offset) {
    uint8_t b1, b2, b3, b4;
    uf->Codes[uf->CodeC++] = opcode; 
    uf->Codes = Reallocate(uf->Codes, sizeof(uint8_t) * (uf->CodeC + 5));
    b1 = (offset >> 24) & 0xFF;
    b2 = (offset >> 16) & 0xFF;
    b3 = (offset >>  8) & 0xFF;
    b4 = (offset >>  0) & 0xFF;
    uf->Codes[uf->CodeC++] = b1;
    uf->Codes[uf->CodeC++] = b2;
    uf->Codes[uf->CodeC++] = b3;
    uf->Codes[uf->CodeC++] = b4;
}

static void _EmitArg(Compiler* compiler, UserFunction* uf, OpcodeEnum opcode, int index) {
    _EmitConst(compiler, uf, opcode, index);
}

static int _EmitJumpTo(Compiler* compiler, UserFunction* uf, OpcodeEnum opcode) {
    int offset = uf->CodeC;
    _EmitConst(compiler, uf, opcode, 0);
    return offset;
}

static void _JumpToLabel(Compiler* compiler, UserFunction* uf, int sourceOffset) {
    uint8_t b1, b2, b3, b4;
    int offset = uf->CodeC;
    b1 = (offset >> 24) & 0xFF;
    b2 = (offset >> 16) & 0xFF;
    b3 = (offset >>  8) & 0xFF;
    b4 = (offset >>  0) & 0xFF;
    uf->Codes[sourceOffset + 1] = b1;
    uf->Codes[sourceOffset + 2] = b2;
    uf->Codes[sourceOffset + 3] = b3;
    uf->Codes[sourceOffset + 4] = b4;
}

#define _Expression(compiler, uf, scope, node) _ExpressionMain(compiler, uf, scope, node, false)

static Value* _ExpressionMain(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node, bool evalOnly) {
    Value* lhs = NULL, *rhs = NULL, *val = NULL;
    switch (node->Type) {
        case AST_NAME: {
            if (!ScopeHasName(scope, node->Value)) {
                ThrowError(
                    compiler->Parser->Lexer->Path, 
                    compiler->Parser->Lexer->Data, 
                    node->Position, 
                    "variable not found"
                );
            }
            
            int offset = 0;

            if (!ScopeIsLocalToFn(scope, node->Value)) {
                // Capture the variable
            }

            Symbol* symbol = ScopeGetSymbol(scope, node->Value, true);

            _EmitArg(
                compiler, 
                uf, 
                ScopeIsLocalToFn(scope, node->Value) || !symbol->IsGlobal ? OP_LOAD_LOCAL : OP_LOAD_NAME,
                symbol->Offset
            );

            break;
        }
        case AST_INT: {
            int offset = _GetConstant(compiler, node->Value);
            if (offset == FLG_NOTFOUND) {
                offset = _SaveInt(compiler, atoi(node->Value));
            }

            val = _GetConstantValue(compiler, offset);

            if (!evalOnly) _EmitConst(
                compiler, 
                uf, 
                OP_LOAD_CONST, 
                offset
            );
            break;
        }
        case AST_NUM: {
            int offset = _GetConstant(compiler, node->Value);
            if (offset == FLG_NOTFOUND) {
                offset = _SaveNum(compiler, strtod(node->Value, NULL));
            }

            val = _GetConstantValue(compiler, offset);

            if (!evalOnly) _EmitConst(
                compiler, 
                uf, 
                OP_LOAD_CONST, 
                offset
            );
            break;
        } 
        case AST_STR: {
            int offset = _GetConstant(compiler, node->Value);
            if (offset == FLG_NOTFOUND) {
                offset = _SaveStr(compiler, AllocateString(node->Value));
            }

            val = _GetConstantValue(compiler, offset);

            if (!evalOnly) _EmitConst(
                compiler, 
                uf, 
                OP_LOAD_CONST, 
                offset
            );
            break;
        }
        case AST_BOOL: {
            // Do not save boolean constants
            if (strcmp(node->Value, "true") == 0) {
                val = compiler->Interpreter->True;
            } else {
                val = compiler->Interpreter->False;
            }

            if (!evalOnly) _EmitArg(
                compiler, 
                uf, 
                OP_LOAD_BOOL,
                val == compiler->Interpreter->True ? 1 : 0
            );
            break;
        }
        case AST_NULL: {
            val = compiler->Interpreter->Null;

            if (!evalOnly) _Emit(
                compiler, 
                uf, 
                OP_LOAD_NULL
            );
            break;
        }
        case AST_CALL: {
            Ast* objc = node->A;
            Ast* args = node->B;

            int argc = 0;
            while (args != NULL) {
                _Expression(compiler, uf, scope, args);
                argc++;
                args = args->Next;
            }

            _Expression(compiler, uf, scope, objc);
            _EmitArg(compiler, uf, OP_CALL, argc);
            break;
        }
        case AST_MUL: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoMul(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }
            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_MUL);
            break;
        }
        case AST_DIV: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoDiv(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_ZERO_DIV) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "division by zero"
                    );
                } else if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_DIV);
            break;
        }
        case AST_MOD: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoMod(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_ZERO_DIV) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "modulo by zero"
                    );
                } else if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_MOD);
            break;
        }
        case AST_ADD: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _ExpressionMain(compiler, uf, scope, node->A, true);
                rhs = _ExpressionMain(compiler, uf, scope, node->B, true);
                int offset = DoAdd(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_ADD);
            break;
        }
        case AST_SUB: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoSub(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_SUB);
            break;
        }
        case AST_LSHFT: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoLShift(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_LSHFT);
            break;
        }
        case AST_RSHFT: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoRShift(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_RSHFT);
            break;
        }
        case AST_LT: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoLT(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_LT);
            break;
        }
        case AST_LTE: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoLTE(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_LTE);
            break;
        }
        case AST_GT: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoGT(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_GT);
            break;
        }
        case AST_GTE: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoGTE(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_GTE);
            break;
        }
        case AST_EQ: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoEQ(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_EQ);
            break;
        }
        case AST_NE: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoNE(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }
                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_NE);
            break;
        }
        case AST_AND: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoAnd(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }

                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_AND);
            break;
        }
        case AST_OR: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoOr(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }

                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_OR);
            break;
        }
        case AST_XOR: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                int offset = DoXor(compiler->Interpreter, lhs, rhs, NULL);
                if (offset == FLG_INVALID_OPERATION) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid operation"
                    );
                }

                val = _GetConstantValue(compiler, offset);
                if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
                break;
            }

            lhs = _Expression(compiler, uf, scope, node->A);
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_XOR);
            break;
        }
        case AST_LAND: {
            _Expression(compiler, uf, scope, node->A);
            int jumpOffset = _EmitJumpTo(compiler, uf, OP_JUMP_IF_FALSE_OR_POP);
            _Expression(compiler, uf, scope, node->B);
            _JumpToLabel(compiler, uf, jumpOffset);
            break;
        }
        case AST_LOR: {
            _Expression(compiler, uf, scope, node->A);
            int jumpOffset = _EmitJumpTo(compiler, uf, OP_JUMP_IF_TRUE_OR_POP);
            _Expression(compiler, uf, scope, node->B);
            _JumpToLabel(compiler, uf, jumpOffset);
            break;
        }
        default: {
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                node->Position, 
                "expected an expression"
            );
            break;
        }
    }
    return val;
}

static void _Statement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node);

static void _FunctionDeclaration(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    if (!ScopeIs(scope, SCOPE_GLOBAL)) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            node->Position, 
            "functions can only be declared at the global scope"
        );
    }

    Scope* fnScope = CreateScope(SCOPE_FUNCTION, scope);
    Ast* fnName = node->A;
    Ast* params = node->B;
    Ast* body   = node->C;

    int nameOffset = UserFunctionEmitLocal(uf);

    if (ScopeHasLocal(scope, fnName->Value)) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            fnName->Position, 
            "duplicate function name"
        );
    }

    ScopeSetSymbol(scope, fnName->Value, true, true, nameOffset);

    UserFunction* fn = CreateUserFunction(fnName->Value, 0);

    int paramc = 0;
    while (params != NULL) {
        if (ScopeHasLocal(fnScope, params->Value)) {
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                params->Position, 
                "duplicate parameter name"
            );
        }

        int offset = UserFunctionEmitLocal(fn);

        ScopeSetSymbol(fnScope, params->Value, false, true, offset);

        _EmitArg(compiler, fn, OP_STORE_LOCAL, offset);
        paramc++;
        params = params->Next;
    }

    fn->Argc = paramc;

    while (body != NULL) {
        _Statement(compiler, fn, fnScope, body);
        body = body->Next;
    }

    if (!fnScope->Returned) {
        _Emit(compiler, fn, OP_LOAD_NULL);
        _Emit(compiler, fn, OP_RETURN);
    }

    // Create the function
    Value* fnValue = NewUserFunctionValue(compiler->Interpreter, fn);
    int funcOffset = _SaveFunction(compiler, fnValue);

    _EmitArg(compiler, uf, OP_LOAD_FUNCTION, funcOffset);
    _EmitArg(compiler, uf, OP_STORE_NAME, nameOffset);
}

static void _IfStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Ast* condition  = node->A;
    Ast* thenBranch = node->B;
    Ast* elseBranch = node->C;
    _Expression(compiler, uf, scope, condition);
    int jumpOffset = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
    _Statement(compiler, uf, scope, thenBranch);
    int jumpEndIfOffset = _EmitJumpTo(compiler, uf, OP_JUMP);
    _JumpToLabel(compiler, uf, jumpOffset);
    if (elseBranch != NULL) {
        _Statement(compiler, uf, scope, elseBranch);
    }
    _JumpToLabel(compiler, uf, jumpEndIfOffset);
}

static void _ReturnStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    if (!ScopeInside(scope, SCOPE_FUNCTION)) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            node->Position, 
            "return statement can only be used inside a function"
        );
    }

    Scope* fnScope = ScopeGetFirst(scope, SCOPE_FUNCTION);
    if (fnScope != NULL) {
        fnScope->Returned = true;
    }

    if (node->A != NULL) _Expression(compiler, uf, scope, node->A);
    _Emit(compiler, uf, OP_RETURN);
}

static void _ExpressionStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Value* val = _Expression(compiler, uf, scope, node->A);
    _Emit(compiler, uf, OP_POPTOP);
}

static void _Statement(Compiler* compiler, UserFunction* userFunction, Scope* scope, Ast* node) {\
    switch (node->Type) {
        case AST_FUNCTION:
            _FunctionDeclaration(compiler, userFunction, scope, node);
            break;
        case AST_IF:
            _IfStatement(compiler, userFunction, scope, node);
            break;
        case AST_RETURN:
            _ReturnStatement(compiler, userFunction, scope, node);
            break;
        case AST_EXPRESSION_STATEMENT:
            _ExpressionStatement(compiler, userFunction, scope, node);
            break;
        default:
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                node->Position, 
                "expected a statement"
            );
            break;
    }
}

static Value* _Program(Compiler* compiler, Ast* node) {
    Scope* scope = CreateScope(SCOPE_GLOBAL, NULL);
    UserFunction* uf = CreateUserFunction(AllocateString("main"), 0);

    Ast* current = node->A;
    while (current != NULL) {
        _Statement(compiler, uf, scope, current);
        current = current->Next;
    }

    _Emit(compiler, uf, OP_LOAD_NULL);
    _Emit(compiler, uf, OP_RETURN);

    return NewUserFunctionValue(compiler->Interpreter, uf);
}

Value* Compile(Compiler* compiler) {
    Ast* program = Parse(compiler->Parser);
    Value* value = _Program(compiler, program);
    FreeAst(program);
    return value;
}

#undef PushArray
#undef GetOffset