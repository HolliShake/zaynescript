#include "./compiler.h"

#define PushArray(type, array, count, val, defaultValue) do { \
    (array)[count++] = val; \
    (array) = Reallocate((array), sizeof(type) * ((count) + 1)); \
    (array)[count] = (defaultValue); \
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

    for (int i = 0; i < offset; i++) {
        Value* constantRaw = compiler->Interpreter->Constants[i];
        if (ValueIsInt(constantRaw) && CoerceToI32(constantRaw) == val) {
            return i;
        }
    }

    Value* newValue = NewIntValue(compiler->Interpreter, val);

    PushArray(
        Value*,
        compiler->Interpreter->Constants, 
        compiler->Interpreter->ConstantC, 
        newValue, 
        NULL
    );

    return offset;
}

static int _SaveNum(Compiler* compiler, double val) {
    int offset = GetOffset();

    for (int i = 0; i < offset; i++) {
        Value* constantRaw = compiler->Interpreter->Constants[i];
        if (ValueIsNum(constantRaw) && CoerceToNum(constantRaw) == val) {
            return i;
        }
    }

    Value* newValue = NewNumValue(compiler->Interpreter, val);

    PushArray(
        Value*,
        compiler->Interpreter->Constants, 
        compiler->Interpreter->ConstantC, 
        newValue, 
        NULL
    );

    return offset;
}

static int _SaveStr(Compiler* compiler, String val) {
    int offset = GetOffset();
    for (int i = 0; i < offset; i++) {
        Value* constantRaw = compiler->Interpreter->Constants[i];
        if (!ValueIsStr(constantRaw)) {
            continue;
        }
        String constantStr = ValueToString(constantRaw);
        bool isEqual = strcmp(constantStr, val) == 0;
        free(constantStr);
        if (isEqual) {
            return i;
        }
    }

    Value* newValue = NewStrValue(compiler->Interpreter, val);

    PushArray(
        Value*,
        compiler->Interpreter->Constants, 
        compiler->Interpreter->ConstantC, 
        newValue, 
        NULL
    );

    return offset;
}

static int _SaveConstantValue(Compiler* compiler, Value* val) {
    int offset = GetOffset();
    
    // Search if the constant already exists
    for (int i = 0; i < offset; i++) {
        Value* constant = compiler->Interpreter->Constants[i];
        if (ValueIsEqual(constant, val)) {
            return i;
        }
    }

    PushArray(
        Value*,
        compiler->Interpreter->Constants, 
        compiler->Interpreter->ConstantC, 
        val, 
        NULL
    );
    
    return offset;
}

static Value* _GetConstantValue(Compiler* compiler, int offset) {
    if (offset < 0 || offset >= compiler->Interpreter->ConstantC) {
        Panic("Constant offset out of bounds %d (max %d)\n", offset, compiler->Interpreter->ConstantC);
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

static void _Statement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node);

static void _AssignOp(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* exp);
static void _AssignOpRhs(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* lhs, bool postfix);
static void _AssignOpLhs(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* lhs, bool postfix);

static void _Identifier(Compiler* compiler, UserFunction* uf, Scope* scope, String name, Position pos) {
    if (!ScopeHasName(scope, name)) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            pos, 
            "variable not found"
        );
    }

    Symbol* symbol = ScopeGetSymbol(scope, name, true);

    if (ScopeInside(scope, SCOPE_FUNCTION) && !ScopeIsLocalToFn(scope, name)) {
        int captureOffset = 0;
        if (!ScopeHasCapture(scope, name)) {
            captureOffset = UserFunctionAddCapture(
                uf, 
                ScopeGetDepthOfSymbol(scope, name),
                symbol->Offset
            );
            ScopeSetCapture(
                scope, 
                name, 
                false, 
                true, 
                false, 
                captureOffset
            );
        } else {
            captureOffset = ScopeGetCapture(scope, name, true)->Offset;
        }

        // Capture the variable
        _EmitArg(
            compiler, 
            uf, 
            OP_LOAD_CAPTURE,
            captureOffset
        );
        return;
    }
    
    _EmitArg(
        compiler, 
        uf, 
        OP_LOAD_LOCAL,
        symbol->Offset
    );
}

static Value* _ExpressionMain(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node, bool evalOnly) {
    Value* lhs = NULL, *rhs = NULL, *val = NULL;
    int offset = 0;
    switch (node->Type) {
        case AST_NAME: {
            _Identifier(compiler, uf, scope, node->Value, node->Position);
            break;
        }
        case AST_INT: {
            long long lld = strtoll(node->Value, NULL, 10);
            if (lld > INT_MAX || lld < INT_MIN) {
                offset = _SaveNum(compiler, (double)lld);
            } else {
                offset = _SaveInt(compiler, (int)lld);
            }
            val = _GetConstantValue(compiler, offset);
            if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
            break;
        }
        case AST_NUM: {
            offset = _SaveNum(compiler, strtod(node->Value, NULL));
            val = _GetConstantValue(compiler, offset);
            if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
            break;
        } 
        case AST_STR: {
            offset = _SaveStr(compiler, node->Value);
            val = _GetConstantValue(compiler, offset);
            if (!evalOnly) _EmitConst(compiler, uf, OP_LOAD_CONST, offset);
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
        case AST_THIS: {
            if (!(ScopeInside(scope, SCOPE_CLASS) || ScopeInside(scope, SCOPE_FUNCTION))) {
                ThrowError(
                    compiler->Parser->Lexer->Path, 
                    compiler->Parser->Lexer->Data, 
                    node->Position, 
                    "'this' can only be used inside class methods"
                );
            }
            _Identifier(compiler, uf, scope, "this", node->Position);
            break;
        }
        case AST_LIST_LITERAL: {
            Ast* elements  = node->A;
            bool hasSpread = false;
            int count = 0;
            while (elements != NULL) {
                switch (elements->Type) {
                    case AST_SPREAD: {
                        if (!hasSpread) {
                            // Emit array with number if elements
                            _EmitArg(compiler, uf, OP_ARRAY_MAKE, count);
                        }
                        count = 0;
                        hasSpread = true;
                        _Expression(compiler, uf, scope, elements->A);
                        _Emit(compiler, uf, OP_ARRAY_EXTEND);
                        break;
                    }
                    default: {
                        if (!hasSpread) ++count;
                        _Expression(compiler, uf, scope, elements);
                        if (hasSpread) _Emit(compiler, uf, OP_ARRAY_PUSH);
                        break;
                    }
                }
                elements = elements->Next;
            }
            if (!hasSpread) _EmitArg(compiler, uf, OP_ARRAY_MAKE, count);
            break;
        }
        case AST_OBJECT_LITERAL: {
            Ast* properties = node->A, *k = NULL, *v = NULL;
            bool hasSpread = false;
            int count = 0;
            while (properties != NULL) {
                switch (properties->Type) {
                    case AST_SPREAD: {
                        if (!hasSpread) {
                            // Emit object with number if pairs
                            _EmitArg(compiler, uf, OP_OBJECT_MAKE, count);
                        }
                        count = 0;
                        hasSpread = true;
                        _Expression(compiler, uf, scope, properties->A);
                        _Emit(compiler, uf, OP_OBJECT_EXTEND);
                        break;
                    }
                    case AST_NAME: {
                        if(!hasSpread) ++count;
                        k = properties;
                        v = properties;
                        _Expression(compiler, uf, scope, v);
                        _EmitString(compiler, uf, OP_LOAD_STRING, k->Value);
                        if (hasSpread) _Emit(compiler, uf, OP_SET_INDEX);
                        break;
                    }
                    case AST_OBJECT_KEY_VAL: {
                        if (!hasSpread) ++count;
                        k = properties->A;
                        v = k->B;
                        _Expression(compiler, uf, scope, v);
                        _EmitString(compiler, uf, OP_LOAD_STRING, k->Value);
                        if (hasSpread) {
                            _Emit(compiler, uf, OP_ROT2);
                            _Emit(compiler, uf, OP_SET_INDEX);
                        }
                        break;
                    }
                    default: {
                        ThrowError(
                            compiler->Parser->Lexer->Path, 
                            compiler->Parser->Lexer->Data, 
                            properties->Position, 
                            "invalid object property"
                        );
                    }
                }
                properties = properties->Next;
            }
            if (!hasSpread) {
                // Emit object with number if pairs
                _EmitArg(compiler, uf, OP_OBJECT_MAKE, count);
            }
            break;
        }
        case AST_FUNCTION: {
            Scope* fnScope = CreateScope(SCOPE_FUNCTION, scope);
            Ast* params = node->B;
            Ast* body   = node->C;

            UserFunction* fn = CreateUserFunction(NULL, 0);

            // First, count parameters and collect them
            int paramc = 0;
            Ast* paramCount = params;
            while (paramCount != NULL) {
                paramc++;
                paramCount = paramCount->Next;
            }

            // Create array to store parameters in reverse
            Ast** paramArray = Allocate(sizeof(Ast*) * paramc);
            int i = 0;
            Ast* param = params;
            while (param != NULL) {
                paramArray[i++] = param;
                param = param->Next;
            }

            // Process parameters in reverse order
            for (int j = paramc - 1; j >= 0; j--) {
                Ast* currentParam = paramArray[j];
                if (ScopeHasLocal(fnScope, currentParam->Value)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        currentParam->Position, 
                        "duplicate parameter name"
                    );
                }

                int offset = UserFunctionEmitLocal(fn);

                ScopeSetSymbol(fnScope, currentParam->Value, false, true, false, offset);

                _EmitArg(compiler, fn, OP_STORE_LOCAL, offset);
            }

            free(paramArray);

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

            _EmitArg(compiler, uf, OP_LOAD_FUNCTION_CLOSURE, funcOffset);
            FreeScope(fnScope);
            break;
        }
        case AST_ALLOCATION: {
            Ast* cls      = node->A;
            Ast* arguments = node->B;

            int argc = 0;
            // Count arguments first
            Ast* argCount = arguments;
            while (argCount != NULL) {
                argc++;
                argCount = argCount->Next;
            }

            // Emit arguments in reverse order
            Ast** argArray = Allocate(sizeof(Ast*) * argc);
            int i = 0;
            Ast* arg = arguments;
            while (arg != NULL) {
                argArray[i++] = arg;
                arg = arg->Next;
            }
            for (int j = argc - 1; j >= 0; j--) {
                _Expression(compiler, uf, scope, argArray[j]);
            }
            free(argArray);

            _Expression(compiler, uf, scope, cls);
            _EmitArg(compiler, uf, OP_CALL_CTOR, argc);
            break;
        }
        case AST_MEMBER: {
            Ast* objc = node->A;
            Ast* attr = node->B;
            _Expression(compiler, uf, scope, objc);
            _EmitString(compiler, uf, OP_LOAD_STRING, attr->Value);
            _Emit(compiler, uf, OP_GET_INDEX);
            break;
        }
        case AST_INDEX: {
            Ast* objc = node->A;
            Ast* indx = node->B;
            _Expression(compiler, uf, scope, objc);
            _Expression(compiler, uf, scope, indx);
            _Emit(compiler, uf, OP_GET_INDEX);
            break;
        }
        case AST_CALL: {
            Ast* objc = node->A;
            Ast* args = node->B;

            int argc = 0;

            switch (objc->Type) {
                case AST_MEMBER:
                case AST_INDEX: {
                    Ast* obj = objc->A;
                    Ast* att = objc->B;

                    // Count arguments first
                    Ast* arg = args;
                    while (arg != NULL) {
                        argc++;
                        _Expression(compiler, uf, scope, arg);
                        arg = arg->Next;
                    }

                    _Expression(compiler, uf, scope, obj); // must be in Stack
                    _Emit(compiler, uf, OP_DUPTOP); // duplicate for 'this'
                    if (objc->Type == AST_MEMBER) {
                        _EmitString(compiler, uf, OP_LOAD_STRING, att->Value);
                    } else {
                        _Expression(compiler, uf, scope, att);
                    }
                    
                    _EmitArg(compiler, uf, OP_CALL_METHOD, argc);
                    break;
                }
                default: {
                    // Count arguments first
                    Ast* arg = args;
                    while (arg != NULL) {
                        argc++;
                        _Expression(compiler, uf, scope, arg);
                        arg = arg->Next;
                    }
                    _Expression(compiler, uf, scope, objc);
                    _EmitArg(compiler, uf, OP_CALL, argc);
                    break;
                }
            }
            break;
        }
        case AST_LOGICAL_NOT: {
            _Expression(compiler, uf, scope, node->A);
            _Emit(compiler, uf, OP_NOT);
            break;
        }
        case AST_POSITIVE: {
            _Expression(compiler, uf, scope, node->A);
            _Emit(compiler, uf, OP_POS);
            break;
        }
        case AST_NEGATIVE: {
            _Expression(compiler, uf, scope, node->A);
            _Emit(compiler, uf, OP_NEG);
            break;
        }
        case AST_POST_INC: {
            _AssignOpRhs(compiler, uf, scope, node->A, true);
            _Emit(compiler, uf, OP_POSTINC);
            _AssignOpLhs(compiler, uf, scope, node->A, true);
            break;
        }
        case AST_POST_DEC: {
            _AssignOpRhs(compiler, uf, scope, node->A, true);
            _Emit(compiler, uf, OP_POSTDEC);
            _AssignOpLhs(compiler, uf, scope, node->A, true);
            break;
        }
        case AST_PRE_INC: {
            _AssignOpRhs(compiler, uf, scope, node->A, false);
            _Emit(compiler, uf, OP_INC);
            _Emit(compiler, uf, OP_DUPTOP);
            _AssignOpLhs(compiler, uf, scope, node->A, false);
            break;
        }
        case AST_PRE_DEC: {
            _AssignOpRhs(compiler, uf, scope, node->A, false);
            _Emit(compiler, uf, OP_DEC);
            _Emit(compiler, uf, OP_DUPTOP);
            _AssignOpLhs(compiler, uf, scope, node->A, false);
            break;
        }
        case AST_MUL: {
            if (_IsAstConstant(compiler, node)) {
                lhs = _Expression(compiler, uf, scope, node->A);
                rhs = _Expression(compiler, uf, scope, node->B);
                val = DoMul(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoDiv(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoMod(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoAdd(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoSub(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoLShift(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoRShift(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoLT(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoLTE(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoGT(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoGTE(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoEQ(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoNE(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoAnd(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoOr(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
                val = DoXor(compiler->Interpreter, lhs, rhs);
                if (ValueIsError(val)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        node->Position, 
                        ValueToString(val)
                    );
                }
                offset = _SaveConstantValue(compiler, val);
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
            _AssignOp(compiler, uf, scope, node); 
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

static void _AssignOp(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* exp) {
    Ast* lhs = exp->A;
    Ast* rhs = exp->B;
    switch (lhs->Type) {
        case AST_NAME: {
            _Expression(compiler, uf, scope, rhs);
            _Emit(compiler, uf, OP_DUPTOP);
            if (!ScopeHasName(scope, lhs->Value)) {
                ThrowError(
                    compiler->Parser->Lexer->Path, 
                    compiler->Parser->Lexer->Data, 
                    lhs->Position, 
                    "variable not found"
                );
            }
            Symbol* symbol = ScopeGetSymbol(scope, lhs->Value, true);
            if (symbol->IsConstant) {
                ThrowError(
                    compiler->Parser->Lexer->Path, 
                    compiler->Parser->Lexer->Data, 
                    lhs->Position, 
                    "cannot reassign constant variable"
                );
            }
            if (ScopeInside(scope, SCOPE_FUNCTION) && !ScopeIsLocalToFn(scope, lhs->Value)) {
                int captureOffset = 0;
                if (!ScopeHasCapture(scope, lhs->Value)) {
                    captureOffset = UserFunctionAddCapture(
                        uf, 
                        ScopeGetDepthOfSymbol(scope, lhs->Value),
                        symbol->Offset
                    );
                    ScopeSetCapture(
                        scope, 
                        lhs->Value, 
                        false, 
                        true, 
                        false, 
                        captureOffset
                    );
                } else {
                    captureOffset = ScopeGetCapture(scope, lhs->Value, true)->Offset;
                }

                // Capture the variable
                _EmitArg(
                    compiler, 
                    uf, 
                    OP_STORE_CAPTURE,
                    captureOffset
                );
            } else {
                _EmitArg(
                    compiler, 
                    uf, 
                    OP_STORE_LOCAL,
                    symbol->Offset
                );
            }
            break;
        }
        case AST_MEMBER: {
            // bot [obj, key, val] top
            Ast* obj = lhs->A;
            Ast* att = lhs->B;
            _Expression(compiler, uf, scope, obj);
            _EmitString(compiler, uf, OP_LOAD_STRING, att->Value);
            _Expression(compiler, uf, scope, rhs);
            _Emit(compiler, uf, OP_DUPTOP);
            _Emit(compiler, uf, OP_ROT4);
            _Emit(compiler, uf, OP_SET_INDEX);
            _Emit(compiler, uf, OP_POPTOP); // Pops object
            break;
        }
        case AST_INDEX: {
            // bot [obj, key, val] top
            Ast* obj = lhs->A;
            Ast* idx = lhs->B;
            _Expression(compiler, uf, scope, obj);
            _Expression(compiler, uf, scope, idx);
            _Expression(compiler, uf, scope, rhs);
            _Emit(compiler, uf, OP_DUPTOP);
            _Emit(compiler, uf, OP_ROT4);
            _Emit(compiler, uf, OP_SET_INDEX);
            _Emit(compiler, uf, OP_POPTOP); // Pops object
            break;
        }
        default: {
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                exp->Position, 
                "invalid assignment operation"
            );
        }
    }
}

static void _AssignOpRhs(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* rhs, bool postfix) {
    switch (rhs->Type) {
        case AST_NAME: {
            _Expression(compiler, uf, scope, rhs);
            break;
        }
        case AST_INDEX: {
            // bot [obj, key, obj, key, val] top
            Ast* obj = rhs->A;
            Ast* att = rhs->B;
            _Expression(compiler, uf, scope, obj);
            _Expression(compiler, uf, scope, att);
            _Emit(compiler, uf, OP_DUP2);
            _Emit(compiler, uf, OP_GET_INDEX);
            break;
        }
        case AST_MEMBER: {
            // bot [obj, key, obj, key,val] top
            Ast* obj = rhs->A;
            Ast* att = rhs->B;
            _Expression(compiler, uf, scope, obj);
            _EmitString(compiler, uf, OP_LOAD_STRING, att->Value);
            _Emit(compiler, uf, OP_DUP2);
            _Emit(compiler, uf, OP_GET_INDEX);
            break;
        }
        default: {
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                rhs->Position, 
                "invalid left operand"
            );
        }
    }
}

static void _AssignOpLhs(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* lhs, bool postfix) {
    switch (lhs->Type) {
        case AST_NAME: {
            if (postfix) {
                _Emit(compiler, uf, OP_ROT2);
            }

            if (!ScopeHasName(scope, lhs->Value)) {
                ThrowError(
                    compiler->Parser->Lexer->Path, 
                    compiler->Parser->Lexer->Data, 
                    lhs->Position, 
                    "variable not found"
                );
            }

            Symbol* symbol = ScopeGetSymbol(scope, lhs->Value, true);

            if (symbol->IsConstant) {
                ThrowError(
                    compiler->Parser->Lexer->Path, 
                    compiler->Parser->Lexer->Data, 
                    lhs->Position, 
                    "cannot reassign constant variable"
                );
            }

            if (ScopeInside(scope, SCOPE_FUNCTION) && !ScopeIsLocalToFn(scope, lhs->Value)) {
                int captureOffset = 0;
                if (!ScopeHasCapture(scope, lhs->Value)) {
                    captureOffset = UserFunctionAddCapture(
                        uf, 
                        ScopeGetDepthOfSymbol(scope, lhs->Value),
                        symbol->Offset
                    );
                    ScopeSetCapture(
                        scope, 
                        lhs->Value, 
                        false, 
                        true, 
                        false, 
                        captureOffset
                    );
                } else {
                    captureOffset = ScopeGetCapture(scope, lhs->Value, true)->Offset;
                }

                // Capture the variable
                _EmitArg(
                    compiler, 
                    uf, 
                    OP_STORE_CAPTURE,
                    captureOffset
                );
            } else {
                _EmitArg(
                    compiler, 
                    uf, 
                    OP_STORE_LOCAL,
                    symbol->Offset
                );
            }
            break;
        }
        case AST_INDEX:
        case AST_MEMBER: {
            // bot [obj, key, val, val] top
            // After rotate4:
            // bot [val, obj, key, val] top
            // postfix:
            // bot [obj, key, val, old] top
            // after rotate4:
            // bot [old, obj, key, val] top
            _Emit(compiler, uf, OP_ROT4);
            _Emit(compiler, uf, OP_SET_INDEX);
            _Emit(compiler, uf, OP_POPTOP); // Pops object
            break;
        }
        default: {
            ThrowError(
                compiler->Parser->Lexer->Path, 
                compiler->Parser->Lexer->Data, 
                lhs->Position, 
                "invalid left operand"
            );
        }
    }
}

static void _ClassDeclaration(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    if (!ScopeIs(scope, SCOPE_GLOBAL)) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            node->Position, 
            "classes can only be declared at the global scope"
        );
    }

    Scope* classScope = CreateScope(SCOPE_CLASS, scope);

    Ast* className = node->A;
    Ast* super     = node->B;
    Ast* body      = node->C;

    // Assume its already forwarded
    Symbol* symbol = ScopeGetSymbol(scope, className->Value, false);

    if (symbol == NULL) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            className->Position, 
            "class not found"
        );
    }

    int nameOffset = symbol->Offset;

    _EmitString(compiler, uf, OP_CLASS_MAKE, className->Value);

    if (super != NULL) {
        _Expression(compiler, uf, scope, super);
        _Emit(compiler, uf, OP_CLASS_EXTEND);
    }

    while (body != NULL) {
        bool isStatic   = strcmp(body->Value, "static") == 0; // instance or static
        Ast* actualBody = body->A;
        switch (actualBody->Type) {
            case AST_ASSIGN: {
                Ast* propName = actualBody->A;
                Ast* propVal  = actualBody->B;
                _Expression(compiler, uf, scope, propVal);
                _EmitString(compiler, uf, OP_LOAD_STRING, propName->Value);

                if (ScopeHasLocal(classScope, propName->Value)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        propName->Position, 
                        "duplicate class member name"
                    );
                }
                ScopeSetSymbol(classScope, propName->Value, false, true, false, -1);
                break;
            }
            case AST_FUNCTION: {
                Scope* fnScope = CreateScope(SCOPE_FUNCTION, scope);
                Ast* fnName = actualBody->A;
                Ast* params = actualBody->B;
                Ast* body   = actualBody->C;

                UserFunction* fn = CreateUserFunction(AllocateString(fnName->Value), 0);

                // First, count parameters
                int paramc = 0;
                Ast* paramCount = params;
                while (paramCount != NULL) {
                    paramc++;
                    paramCount = paramCount->Next;
                }

                // Create array to store parameters in reverse
                Ast** paramArray = Allocate(sizeof(Ast*) * paramc);
                int i = 0;
                Ast* param = params;
                while (param != NULL) {
                    paramArray[i++] = param;
                    param = param->Next;
                }

                int add = 0;
                if (!isStatic) {
                    // Emit 'this' as the first parameter
                    int offset = UserFunctionEmitLocal(fn);
                    ScopeSetSymbol(fnScope, KEY_THIS, false, true, false, offset);
                    _EmitArg(compiler, fn, OP_STORE_LOCAL, offset);
                    add++;
                }

                // Process parameters in reverse order
                for (int j = paramc - 1; j >= 0; j--) {
                    Ast* currentParam = paramArray[j];
                    if (ScopeHasLocal(fnScope, currentParam->Value)) {
                        ThrowError(
                            compiler->Parser->Lexer->Path, 
                            compiler->Parser->Lexer->Data, 
                            currentParam->Position, 
                            "duplicate parameter name"
                        );
                    }

                    int offset = UserFunctionEmitLocal(fn);

                    ScopeSetSymbol(fnScope, currentParam->Value, false, true, false, offset);

                    _EmitArg(compiler, fn, OP_STORE_LOCAL, offset);
                }

                free(paramArray);

                fn->Argc = paramc + add;

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
                _EmitString(compiler, uf, OP_LOAD_STRING, fnName->Value);

                if (ScopeHasLocal(classScope, fnName->Value)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        fnName->Position, 
                        "duplicate class member name"
                    );
                }

                ScopeSetSymbol(classScope, fnName->Value, false, true, false, -1);
                break;
            }
            default: {
                ThrowError(
                    compiler->Parser->Lexer->Path, 
                    compiler->Parser->Lexer->Data, 
                    body->Position, 
                    "invalid class body element"
                );
            }
        }
        _Emit(compiler, uf, isStatic ? OP_CLASS_DEFINE_STATIC_MEMBER : OP_CLASS_DEFINE_INSTANCE_MEMBER);
        body = body->Next;
    }
    _EmitArg(compiler, uf, OP_STORE_NAME, nameOffset);
    FreeScope(classScope);
}

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

    UserFunction* fn = CreateUserFunction(AllocateString(fnName->Value), 0);

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

    // Validate, should not contain spaces, dots, or special characters
    if (strpbrk(moduleName->Value, " ./><=!@#$%^&*()_+-=[]{}|\\;'\"`~") != NULL) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            moduleName->Position, 
            "invalid module name, should not contain spaces, dots, or special characters"
        );
    }

    if (StringStartsWith(moduleName->Value, "core:")) {
        _EmitString(compiler, uf, OP_IMPORT_CORE, moduleName->Value + 5);
        if (imports == NULL) {
            // store as object
            _Emit(compiler, uf, OP_DUPTOP);

            if (ScopeHasLocal(scope, moduleName->Value + 5)) {
                ThrowError(
                    compiler->Parser->Lexer->Path, 
                    compiler->Parser->Lexer->Data, 
                    moduleName->Position, 
                    "duplicate import name"
                );
            }

            int offset = UserFunctionEmitLocal(uf);
            ScopeSetSymbol(scope, moduleName->Value + 5, true, true, true, offset);
            _EmitArg(compiler, uf, OP_STORE_NAME, offset);
        }
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

        _EmitString(compiler, uf, OP_OBJECT_PLUCK_ATTRIBUTE, attributeName);

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

    // Pop the module object
    _Emit(compiler, uf, OP_POPTOP);
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
        printf("%s := %d\n", declarations->Value, offset);

        _EmitArg(compiler, uf, OP_STORE_NAME, offset);

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
    if (!(ScopeIs(scope, SCOPE_FUNCTION) || ScopeIs(scope, SCOPE_BLOCK) || ScopeIs(scope, SCOPE_TRY_BLOCK))) {
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
    if (!(ScopeIs(scope, SCOPE_GLOBAL) || ScopeIs(scope, SCOPE_FUNCTION) || ScopeIs(scope, SCOPE_BLOCK) || ScopeIs(scope, SCOPE_TRY_BLOCK))) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            node->Position, 
            "const declarations can only be used inside a function or a block"
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
        _EmitArg(compiler, uf, ScopeIs(scope, SCOPE_GLOBAL) ? OP_STORE_NAME : OP_STORE_LOCAL, offset);

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

    if (initializer != NULL) {
        _InitializerConditionMutator(compiler, uf, loopScope, initializer);
    }

    int forStart   = uf->CodeC;
    int jumpOffset = -1;
    if (condition != NULL) {
        _Expression(compiler, uf, loopScope, condition);
        jumpOffset = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
    }

    _Statement(compiler, uf, loopScope, thenBranch);

    if (mutator != NULL) {
        // continues, but execute mutator first
        for (int i = 0; i < loopScope->ContinueJumpC; i++) {
            _JumpToLabel(compiler, uf, loopScope->ContinueJumps[i]);
        }
        _Expression(compiler, uf, loopScope, mutator);
        _Emit(compiler, uf, OP_POPTOP);
    } else {
        // continues to jump to forStart if has no mutator
        for (int i = 0; i < loopScope->ContinueJumpC; i++) {
            _JumpToAbsoluteLabel(compiler, uf, loopScope->ContinueJumps[i], forStart);
        }
    }

    _JumpToAbsoluteLabel(compiler, uf, _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP), forStart);

    // breaks
    for (int i = 0; i < loopScope->BreakJumpC; i++) {
        _JumpToLabel(compiler, uf, loopScope->BreakJumps[i]);
    }

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
        // use initializer as condition | condition only while statement
        _InitializerConditionMutator(compiler, uf, loopScope, initializer);
    }

    if (condition != NULL) {
        whileStart = uf->CodeC;
        _Expression(compiler, uf, loopScope, condition);
    }

    int jumpOffset = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
    _Statement(compiler, uf, loopScope, thenBranch);

    if (mutator != NULL) {
        // continues, but execute mutator first
        for (int i = 0; i < loopScope->ContinueJumpC; i++) {
            _JumpToLabel(compiler, uf, loopScope->ContinueJumps[i]);
        }
        _Expression(compiler, uf, loopScope, mutator);
        _Emit(compiler, uf, OP_POPTOP);
    } else {
        // continues to jump to whileStart if has no mutator
        for (int i = 0; i < loopScope->ContinueJumpC; i++) {
            _JumpToAbsoluteLabel(compiler, uf, loopScope->ContinueJumps[i], whileStart);
        }
    }

    _JumpToAbsoluteLabel(compiler, uf, _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP), whileStart);
    _JumpToLabel(compiler, uf, jumpOffset);
    FreeScope(loopScope);
}

static void _DoWhileStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Scope* loopScope = CreateScope(SCOPE_LOOP, scope);
    Ast* condition  = node->A;
    Ast* thenBranch = node->B;
    int doStart     = uf->CodeC;
    _Statement(compiler, uf, loopScope, thenBranch);
    _Expression(compiler, uf, scope, condition);
    int jumpOffset = _EmitJumpTo(compiler, uf, OP_POP_JUMP_IF_FALSE);
    _JumpToAbsoluteLabel(compiler, uf, _EmitJumpTo(compiler, uf, OP_ABSOLUTE_JUMP), doStart);
    // breaks
    for (int i = 0; i < loopScope->BreakJumpC; i++) {
        _JumpToLabel(compiler, uf, loopScope->BreakJumps[i]);
    }
    // continues
    for (int i = 0; i < loopScope->ContinueJumpC; i++) {
        _JumpToAbsoluteLabel(compiler, uf, loopScope->ContinueJumps[i], doStart);
    }
    _JumpToLabel(compiler, uf, jumpOffset);
}

static void _TryCatch(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Ast* tryBlock   = node->A;
    Ast* catchParam = node->B;
    Ast* catchBlock = node->C;
    int targetOffset = -1, skipCatchOffset = -1;
    // Try begin
    targetOffset = _EmitJumpTo(compiler, uf, OP_SETUP_TRY);
    Scope* tryScope = CreateScope(SCOPE_TRY_BLOCK, scope);
    while (tryBlock != NULL) {
        _Statement(compiler, uf, tryScope, tryBlock);
        tryBlock = tryBlock->Next;
    }
    _Emit(compiler, uf, OP_POP_TRY);
    FreeScope(tryScope);
    // Try end
    skipCatchOffset = _EmitJumpTo(compiler, uf, OP_JUMP);
    // Catch begin, Jump here if encounters an error
    _JumpToLabel(compiler, uf, targetOffset);
    Scope* catchScope = CreateScope(SCOPE_BLOCK, scope);
    // Store error object
    int offset = UserFunctionEmitLocal(uf);
    ScopeSetSymbol(catchScope, catchParam->Value, false, true, false, offset);
    _EmitArg(compiler, uf, OP_STORE_LOCAL, offset);
    while (catchBlock != NULL) {
        _Statement(compiler, uf, catchScope, catchBlock);
        catchBlock = catchBlock->Next;
    }
    FreeScope(catchScope);
    // Catch end
    _JumpToLabel(compiler, uf, skipCatchOffset);
}

static void _BlockStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Scope* block = CreateScope(SCOPE_BLOCK, scope);
    Ast* current = node->A;
    while (current != NULL) {
        _Statement(compiler, uf, block, current);
        current = current->Next;
    }
    FreeScope(block);
}

static void _ContinueStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    if (!ScopeInside(scope, SCOPE_LOOP)) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            node->Position, 
            "continue statement can only be used inside a loop"
        );
    }
    if (ScopeInside(scope, SCOPE_TRY_BLOCK)) {
        int n = ScopeCountNested(scope, SCOPE_TRY_BLOCK);
        // Pop try blocks until we exit the try block
        if (n == 1) _Emit(compiler, uf, OP_POP_TRY);
        else _EmitArg(compiler, uf, OP_POPN_TRY, n);
    }
    int offset = _EmitJumpTo(compiler, uf, OP_JUMP);
    ScopeAddContinueJump(scope, offset);
}

static void _BreakStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    if (!ScopeInside(scope, SCOPE_LOOP)) {
        ThrowError(
            compiler->Parser->Lexer->Path, 
            compiler->Parser->Lexer->Data, 
            node->Position, 
            "break statement can only be used inside a loop"
        );
    }
    if (ScopeInside(scope, SCOPE_TRY_BLOCK)) {
        int n = ScopeCountNested(scope, SCOPE_TRY_BLOCK);
        // Pop try blocks until we exit the try block
        if (n == 1) _Emit(compiler, uf, OP_POP_TRY);
        else _EmitArg(compiler, uf, OP_POPN_TRY, n);
    }
    int offset = _EmitJumpTo(compiler, uf, OP_JUMP);
    ScopeAddBreakJump(scope, offset);
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

    if (ScopeInside(scope, SCOPE_TRY_BLOCK)) {
        int n = ScopeCountNested(scope, SCOPE_TRY_BLOCK);
        // Pop try blocks until we exit the try block
        if (n == 1) _Emit(compiler, uf, OP_POP_TRY);
        else _EmitArg(compiler, uf, OP_POPN_TRY, n);
    }

    if (node->A != NULL) _Expression(compiler, uf, scope, node->A);
    _Emit(compiler, uf, OP_RETURN);
}

static void _ExpressionStatement(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    Value* val = _Expression(compiler, uf, scope, node->A);
    _Emit(compiler, uf, OP_POPTOP);
}

static void _ForwardDeclairations(Compiler* compiler, UserFunction* uf, Scope* scope, Ast* node) {
    while (node != NULL) {
        switch (node->Type) {
            case AST_CLASS: {
                Ast* className = node->A;
                if (ScopeHasLocal(scope, className->Value)) {
                    ThrowError(
                        compiler->Parser->Lexer->Path, 
                        compiler->Parser->Lexer->Data, 
                        className->Position, 
                        "duplicate class name"
                    );
                }
                int offset = UserFunctionEmitLocal(uf);
                ScopeSetSymbol(scope, className->Value, true, true, false, offset);
                break;
            }
            case AST_FUNCTION: {
                Ast* fnName = node->A;
                ScopeSetSymbol(scope, fnName->Value, true, true, false, UserFunctionEmitLocal(uf));
                break;
            }
            default:
            break;
        }
        node = node->Next;
    }
}

static void _Statement(Compiler* compiler, UserFunction* userFunction, Scope* scope, Ast* node) {\
    switch (node->Type) {
        case AST_CLASS: 
            _ClassDeclaration(compiler, userFunction, scope, node);
            break;
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
        case AST_TRY_CATCH:
            _TryCatch(compiler, userFunction, scope, node);
            break;
        case AST_BLOCK:
            _BlockStatement(compiler, userFunction, scope, node);
            break;
        case AST_CONTINUE:
            _ContinueStatement(compiler, userFunction, scope, node);
            break;
        case AST_BREAK:
            _BreakStatement(compiler, userFunction, scope, node);
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

    Value* value = NewUserFunctionValue(compiler->Interpreter, uf);
    _SaveFunction(compiler, value);

    Ast* current = node->A;
    _ForwardDeclairations(compiler, uf, scope, current);
    while (current != NULL) {
        _Statement(compiler, uf, scope, current);
        current = current->Next;
    }

    _Emit(compiler, uf, OP_LOAD_NULL);
    _Emit(compiler, uf, OP_RETURN);

    FreeScope(scope);

    return value;
}

Value* Compile(Compiler* compiler) {
    Ast* program = Parse(compiler->Parser);
    Value* value = _Program(compiler, program);
    FreeAst(program);
    return value;
}

void FreeCompiler(Compiler* compiler) {
    free(compiler);
}

#undef PushArray
#undef GetOffset