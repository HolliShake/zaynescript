#include "./compiler.h"

#define PushArray(type, array, count, val, defaultValue) do { \
    (array)[(count)++] = val; \
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

static int _IsAstConstant(Compiler* compiler, Ast* node) {
    switch (node->Type) {
        case AST_INT:
        case AST_NUM:
        case AST_STR:
        case AST_BOOL:
        case AST_NULL:
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
    uf->Codes[uf->CodeC+0] = b1;
    uf->Codes[uf->CodeC+1] = b2;
    uf->Codes[uf->CodeC+2] = b3;
    uf->Codes[uf->CodeC+3] = b4;
}

static Value* _Expression(Compiler* compiler, UserFunction* uf, Ast* node) {
    Value* lhs = NULL, *rhs = NULL, *val = NULL;
    switch (node->Type) {
        case AST_INT: {
            int offset = _GetConstant(compiler, node->Value);
            if (offset == FLG_NOTFOUND) {
                offset = _SaveInt(compiler, atoi(node->Value));
            }

            val = _GetConstantValue(compiler, offset);

            _EmitConst(
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

            _EmitConst(
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
                offset = _SaveStr(compiler, node->Value);
            }

            val = _GetConstantValue(compiler, offset);

            _EmitConst(
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

            _EmitConst(
                compiler, 
                uf, 
                OP_LOAD_BOOL,
                val == compiler->Interpreter->True ? 1 : 0
            );
            break;
        }
        case AST_NULL: {
            val = compiler->Interpreter->Null;

            _Emit(
                compiler, 
                uf, 
                OP_LOAD_NULL
            );
            break;
        }
        case AST_MUL: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, node->A);
                rhs = _Expression(compiler, uf, node->B);
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
                break;
            }
            lhs = _Expression(compiler, uf, node->A);
            rhs = _Expression(compiler, uf, node->B);
            _Emit(compiler, uf, OP_MUL);
            break;
        }
        case AST_DIV: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, node->A);
                rhs = _Expression(compiler, uf, node->B);
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
                break;
            }

            lhs = _Expression(compiler, uf, node->A);
            rhs = _Expression(compiler, uf, node->B);
            _Emit(compiler, uf, OP_DIV);
            break;
        }
        case AST_MOD: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, node->A);
                rhs = _Expression(compiler, uf, node->B);
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
                break;
            }

            lhs = _Expression(compiler, uf, node->A);
            rhs = _Expression(compiler, uf, node->B);
            _Emit(compiler, uf, OP_MOD);
            break;
        }
        case AST_ADD: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, node->A);
                rhs = _Expression(compiler, uf, node->B);
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
                break;
            }

            lhs = _Expression(compiler, uf, node->A);
            rhs = _Expression(compiler, uf, node->B);
            _Emit(compiler, uf, OP_ADD);
            break;
        }
        case AST_SUB: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, node->A);
                rhs = _Expression(compiler, uf, node->B);
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
                break;
            }

            lhs = _Expression(compiler, uf, node->A);
            rhs = _Expression(compiler, uf, node->B);
            _Emit(compiler, uf, OP_SUB);
            break;
        }
        case AST_LSHFT: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, node->A);
                rhs = _Expression(compiler, uf, node->B);
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
                break;
            }

            lhs = _Expression(compiler, uf, node->A);
            rhs = _Expression(compiler, uf, node->B);
            _Emit(compiler, uf, OP_LSHFT);
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

static void _ExpressionStatement(Compiler* compiler, UserFunction* uf, Ast* node) {
    Value* val = _Expression(compiler, uf, node->A);
    _Emit(compiler, uf, OP_POPTOP);
    if (val != NULL) {
        printf("Value: %s\n", ValueToString(val));
    }
}

static void _Statement(Compiler* compiler, UserFunction* userFunction, Ast* node) {\
    switch (node->Type) {
        case AST_EXPRESSION_STATEMENT:
            _ExpressionStatement(compiler, userFunction, node);
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

static void _Program(Compiler* compiler, Ast* node) {
    UserFunction* uf = CreateUserFunction(AllocateString("main"), 0);

    Ast* current = node->A;
    while (current != NULL) {
        _Statement(compiler, uf, current);
        current = current->Next;
    }
}

void Compile(Compiler* compiler) {
    Ast* program = Parse(compiler->Parser);
    _Program(compiler, program);
}

#undef PushArray
#undef GetOffset