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

static void _EmitString(Compiler* compiler, UserFunction* uf, OpcodeEnum opcode, String str) {
    int length = strlen(str);
    _Emit(compiler, uf, opcode);
    for (int i = 0; i < length; i++) {
        _Emit(compiler, uf, (OpcodeEnum) str[i]);
    }
    _Emit(compiler, uf, (OpcodeEnum) '\0');
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

static void _JumpToAbsoluteLabel(Compiler* compiler, UserFunction* uf, int sourceOffset, int targetOffset) {
    uint8_t b1, b2, b3, b4;
    int offset = targetOffset;
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
            
            bool isOwnedLocally = ScopeIsLocalToFn(scope, node->Value);

            // if (!isOwnedLocally) {
            //     // Capture the variable
            //     printf("CAPTURE: %s\n", node->Value);
            // }

            Symbol* symbol = ScopeGetSymbol(scope, node->Value, true);

            _EmitArg(
                compiler, 
                uf, 
                isOwnedLocally ? OP_LOAD_LOCAL : OP_LOAD_NAME,
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
        case AST_ASSIGN: {
            rhs = _Expression(compiler, uf, scope, node->B);
            _Emit(compiler, uf, OP_DUPTOP);
            switch (node->A->Type) {
                case AST_NAME: {
                    if (!ScopeHasName(scope, node->A->Value)) {
                        ThrowError(
                            compiler->Parser->Lexer->Path, 
                            compiler->Parser->Lexer->Data, 
                            node->A->Position, 
                            "variable not found"
                        );
                    }
                    bool isOwnedLocally = ScopeIsLocalToFn(scope, node->Value);
                    Symbol* symbol = ScopeGetSymbol(scope, node->A->Value, true);
                    _EmitArg(
                        compiler, 
                        uf, 
                        isOwnedLocally ? OP_STORE_LOCAL : OP_STORE_NAME,
                        symbol->Offset
                    );
                    break;
                }
                default: {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        "invalid left operand"
                    );
                }
            }
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

    // Assume its already forwarded
    Symbol* symbol = ScopeGetSymbol(scope, fnName->Value, false);

    if (symbol == NULL) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            fnName->Position, 
            "function not found"
        );
    }

    int nameOffset = symbol->Offset;

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

        ScopeSetSymbol(fnScope, params->Value, false, true, false, offset);

        _EmitArg(compiler, fn, OP_STORE_LOCAL, offset);
        paramc++;
        params = params->Next;
    }

    fn->Argc = paramc;

    while (body != NULL) {
        _Statement(compiler, fn, fnScope, body);
        body = body->Next;
    }

    _Emit(compiler, fn, OP_LOAD_NULL);
    _Emit(compiler, fn, OP_RETURN);

    // Create the function
    Value* fnValue = NewUserFunctionValue(compiler->Interpreter, fn);
    int funcOffset = _SaveFunction(compiler, fnValue);

    _EmitArg(compiler, uf, OP_LOAD_FUNCTION, funcOffset);
    _EmitArg(compiler, uf, OP_STORE_NAME, nameOffset);
    FreeScope(fnScope);
}

static void _ImportStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    if (!ScopeIs(scope, SCOPE_GLOBAL)) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            node->Position, 
            "imports can only be declared at the global scope"
        );
    }

    Ast* imports    = node->A;
    Ast* moduleName = node->B;

    if (StringStartsWith(moduleName->Value, "core:")) {
        _EmitString(compiler, uf, OP_IMPORT_CORE, moduleName->Value + 5);
    } else {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            moduleName->Position, 
            "invalid module name"
        );
    }

    while (imports != NULL) {
        String attributeName = imports->Value;

        _EmitString(compiler, uf, OP_PLUCK_ATTRIBUTE, attributeName);

        if (ScopeHasLocal(scope, attributeName)) {
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                imports->Position, 
                "duplicate import name"
            );
        }

        int offset = UserFunctionEmitLocal(uf);
        _EmitArg(compiler, uf, OP_STORE_NAME, offset);
        ScopeSetSymbol(scope, attributeName, true, true, false, offset);

        imports = imports->Next;
    }
}

static void _VarDeclarationStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    if (!ScopeIs(scope, SCOPE_GLOBAL)) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            node->Position, 
            "variables can only be declared at the global scope"
        );
    }
    Ast* declarations = node->A;
    while (declarations != NULL) {
        if (declarations->B != NULL) {
            _Expression(compiler, uf, scope, declarations->B);
        } else {
            _Emit(compiler, uf, OP_LOAD_NULL);
        }

        int offset = UserFunctionEmitLocal(uf);

        _EmitArg(compiler, uf, OP_STORE_LOCAL, offset);

        if (ScopeHasLocal(scope, declarations->Value)) {
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                declarations->Position, 
                "duplicate variable name"
            );
        }

        ScopeSetSymbol(scope, declarations->Value, true, true, false, offset);

        declarations = declarations->Next;
    }
}

static void _LetDeclarationStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    if (!(ScopeIs(scope, SCOPE_FUNCTION) || ScopeIs(scope, SCOPE_BLOCK))) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            node->Position, 
            "let declarations can only be used inside a function or a block"
        );
    }
    Ast* declarations = node->A;
    while (declarations != NULL) {
        if (declarations->B != NULL) {
            _Expression(compiler, uf, scope, declarations->B);
        } else {
            _Emit(compiler, uf, OP_LOAD_NULL);
        }
        
        int offset = UserFunctionEmitLocal(uf);
        _EmitArg(compiler, uf, OP_STORE_LOCAL, offset);

        if (ScopeHasLocal(scope, declarations->Value)) {
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                declarations->Position, 
                "duplicate variable name"
            );
        }

        ScopeSetSymbol(scope, declarations->Value, false, true, false,offset);
        declarations = declarations->Next;
    }
}

static void _ConstDeclarationStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Ast* declarations = node->A;
    while (declarations != NULL) {
        if (declarations->B != NULL) {
            _Expression(compiler, uf, scope, declarations->B);
        } else {
            _Emit(compiler, uf, OP_LOAD_NULL);
        }

        int offset = UserFunctionEmitLocal(uf);
        _EmitArg(compiler, uf, OP_STORE_LOCAL, offset);

        if (ScopeHasLocal(scope, declarations->Value)) {
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                declarations->Position, 
                "duplicate variable name"
            );
        }

        ScopeSetSymbol(scope, declarations->Value, ScopeIs(scope, SCOPE_GLOBAL), ScopeGetFirst(scope, SCOPE_FUNCTION) != NULL, true, offset);
        declarations = declarations->Next;
    }
}

static void _InitializerConditionMutator(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Ast* lhs = node->A, *rhs = node->B;
    if (node->Type == AST_SHORT_ASSIGN) {
        _Expression(compiler, uf, scope, rhs);
        if (ScopeHasLocal(scope, lhs->Value)) {
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                lhs->Position, 
                "variable not found"
            );
        }
        int offset = UserFunctionEmitLocal(uf);
        ScopeSetSymbol(scope, lhs->Value, false, true, false, offset);
        _EmitArg(compiler, uf, OP_STORE_LOCAL, offset);
        return;
    }
    _Expression(compiler, uf, scope, node);
}

static void _IfStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Scope* useScope  = scope;
    Ast* initializer = node->A, *condition = NULL;
    if (initializer->Next != NULL) {
        condition = initializer->Next;
    }
    Ast* thenBranch  = node->B;
    Ast* elseBranch  = node->C;

    if (initializer != NULL && condition != NULL) {
        if (initializer->Type == AST_SHORT_ASSIGN) {
            useScope = CreateScope(SCOPE_BLOCK, scope);
        }
        _InitializerConditionMutator(compiler, uf, useScope, initializer);
        _Expression(compiler, uf, useScope, condition);
    } else if (initializer != NULL) {
        // use initializer as condition
        _InitializerConditionMutator(compiler, uf, useScope, initializer);
    } else {
        _Expression(compiler, uf, useScope, condition);
    }
    int jumpOffset = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
    _Statement(compiler, uf, useScope, thenBranch);
    int jumpEndIfOffset = _EmitJumpTo(compiler, uf, OP_JUMP);
    _JumpToLabel(compiler, uf, jumpOffset);
    if (elseBranch != NULL) {
        // else branch is always in the outside scope
        _Statement(compiler, uf, scope, elseBranch);
    }
    _JumpToLabel(compiler, uf, jumpEndIfOffset);
    if (useScope != scope) {
        FreeScope(useScope);
    }
}

static void _ForStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Scope* loopScope = CreateScope(SCOPE_LOOP, scope);
    Ast* initializer = node->A, *condition = NULL, *mutator = NULL;
    if (initializer != NULL && initializer->Next != NULL) {
        condition = initializer->Next;
        if (condition != NULL && condition->Next != NULL) {
            mutator = condition->Next;
        }
    }
    Ast* thenBranch = node->B;
    int forStart    = uf->CodeC;

    if (initializer != NULL) {
        // use initializer as condition
        _InitializerConditionMutator(compiler, uf, loopScope, initializer);
    }

    int jumpOffset = -1;
    if (condition != NULL) {
        forStart = uf->CodeC;
        _Expression(compiler, uf, loopScope, condition);
        jumpOffset = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
    }

    forStart = uf->CodeC;

    _Statement(compiler, uf, loopScope, thenBranch);
    if (mutator != NULL) {
        _Expression(compiler, uf, loopScope, mutator);
        _Emit(compiler, uf, OP_POPTOP);
    }
    _JumpToAbsoluteLabel(compiler, uf, _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP), forStart);
    if (jumpOffset != -1) _JumpToLabel(compiler, uf, jumpOffset);
    FreeScope(loopScope);
}

static void _WhileStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Scope* loopScope = CreateScope(SCOPE_LOOP, scope);
    Ast* initializer = node->A, *condition = NULL, *mutator = NULL;
    if (initializer->Next != NULL) {
        condition = initializer->Next;
        if (condition->Next != NULL) {
            mutator = condition->Next;
        }
    }
    Ast* thenBranch = node->B;
    int whileStart  = uf->CodeC;

    if (initializer != NULL) {
        // use initializer as condition
        _InitializerConditionMutator(compiler, uf, loopScope, initializer);
    }

    if (condition != NULL) {
        whileStart = uf->CodeC;
        _Expression(compiler, uf, loopScope, condition);
    }

    int jumpOffset = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
    _Statement(compiler, uf, loopScope, thenBranch);
    if (mutator != NULL) {
        _Expression(compiler, uf, loopScope, mutator);
        _Emit(compiler, uf, OP_POPTOP);
    }
    _JumpToAbsoluteLabel(compiler, uf, _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP), whileStart);
    _JumpToLabel(compiler, uf, jumpOffset);
    FreeScope(loopScope);
}

static void _DoWhileStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Ast* condition  = node->A;
    Ast* thenBranch = node->B;
    int doStart     = uf->CodeC;
    _Statement(compiler, uf, scope, thenBranch);
    _Expression(compiler, uf, scope, condition);
    int jumpOffset = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
    _JumpToAbsoluteLabel(compiler, uf, _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP), doStart);
    _JumpToLabel(compiler, uf, jumpOffset);
}

static void _BlockStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Scope* block = CreateScope(SCOPE_BLOCK, scope);
    Ast* current = node->A;
    while (current != NULL) {
        _Statement(compiler, uf, block, current);
        current = current->Next;
    }
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

static void _ForwardFunctions(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    while (node != NULL) {
        switch (node->Type) {
            case AST_FUNCTION: {
                Ast* fnName = node->A;
                ScopeSetSymbol(scope, fnName->Value, true, true, false, UserFunctionEmitLocal(uf));
                break;
            }
        }
        node = node->Next;
    }
}

static void _Statement(Compiler* compiler, UserFunction* userFunction, Scope* scope, Ast* node) {\
    switch (node->Type) {
        case AST_FUNCTION:
            _FunctionDeclaration(compiler, userFunction, scope, node);
            break;
        case AST_IMPORT:
            _ImportStatement(compiler, userFunction, scope, node);
            break;
        case AST_VAR_DECLARATION:
            _VarDeclarationStatement(compiler, userFunction, scope, node);
            break;
        case AST_LET_DECLARATION:
            _LetDeclarationStatement(compiler, userFunction, scope, node);
            break;
        case AST_CONST_DECLARATION:
            _ConstDeclarationStatement(compiler, userFunction, scope, node);
            break;
        case AST_IF:
            _IfStatement(compiler, userFunction, scope, node);
            break;
        case AST_FOR:
            _ForStatement(compiler, userFunction, scope, node);
            break;
        case AST_WHILE:
            _WhileStatement(compiler, userFunction, scope, node);
            break;
        case AST_DO_WHILE:
            _DoWhileStatement(compiler, userFunction, scope, node);
            break;
        case AST_BLOCK:
            _BlockStatement(compiler, userFunction, scope, node);
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
    _ForwardFunctions(compiler, uf, scope, current);
    while (current != NULL) {
        _Statement(compiler, uf, scope, current);
        current = current->Next;
    }

    _Emit(compiler, uf, OP_LOAD_NULL);
    _Emit(compiler, uf, OP_RETURN);

    FreeScope(scope);

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