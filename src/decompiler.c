#include "./decompiler.h"

extern String ValueToString(Value* value);

static int _ReadOffset(uint8_t* codes, int alignStart) {
    int offset = 0;
    offset |= codes[alignStart + 0] << 24;
    offset |= codes[alignStart + 1] << 16;
    offset |= codes[alignStart + 2] <<  8;
    offset |= codes[alignStart + 3] <<  0;
    return offset;
}

static String _ReadString(uint8_t* codes, int alignStart) {
    String str = (String)(codes + alignStart);
    int length = strlen(str);
    String new = Allocate(length + 1);
    memcpy(new, str, length + 1);
    return new;
}

static void _Append(String* dest, String src) {
    if (*dest == NULL) {
        *dest = Allocate(strlen(src) + 1);
        strcpy(*dest, src);
    } else {
        int len = strlen(*dest);
        int srcLen = strlen(src);
        *dest = Reallocate(*dest, len + srcLen + 1);
        strcat(*dest, src);
    }
}

static void _AppendFmt(String* dest, const char* format, ...) {
    va_list args, args_copy;
    va_start(args, format);
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, format, args);
    va_end(args);

    String str = Allocate(size + 1);
    vsnprintf(str, size + 1, format, args_copy);
    va_end(args_copy);

    _Append(dest, str);
    free(str);
}

String DecompileFunction(Interpreter* interpreter, UserFunction* uf) {
    String result = NULL;
    int ip = 0;
    
    _AppendFmt(&result, "Function: %s (argc: %d, locals: %d)\n", uf->Name ? uf->Name : "<anonymous>", uf->Argc, uf->LocalC);

    while (ip < uf->CodeC) {
        int startIp = ip;
        uint8_t opcode = uf->Codes[ip++];
        
        _AppendFmt(&result, "%04d ", startIp);

        switch (opcode) {
            case OP_IMPORT_CORE: {
                String str = _ReadString(uf->Codes, ip);
                _AppendFmt(&result, "OP_IMPORT_CORE \"%s\"\n", str);
                ip += strlen(str) + 1;
                free(str);
                break;
            }
            case OP_LOAD_CAPTURE: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_LOAD_CAPTURE %d\n", offset);
                ip += 4;
                break;
            }
            case OP_LOAD_NAME: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_LOAD_NAME %d\n", offset);
                ip += 4;
                break;
            }
            case OP_LOAD_LOCAL: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_LOAD_LOCAL %d\n", offset);
                ip += 4;
                break;
            }
            case OP_LOAD_CONST: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_LOAD_CONST %d", offset);
                if (interpreter && interpreter->Constants && offset < interpreter->ConstantC) {
                    Value* val = interpreter->Constants[offset];
                    String valStr = ValueToString(val);
                    _AppendFmt(&result, " (%s)", valStr);
                    free(valStr);
                }
                _Append(&result, "\n");
                ip += 4;
                break;
            }
            case OP_LOAD_BOOL: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_LOAD_BOOL %d\n", offset);
                ip += 4;
                break;
            }
            case OP_LOAD_NULL: {
                _Append(&result, "OP_LOAD_NULL\n");
                break;
            }
            case OP_LOAD_STRING: {
                String str = _ReadString(uf->Codes, ip);
                _AppendFmt(&result, "OP_LOAD_STRING \"%s\"\n", str);
                ip += strlen(str) + 1;
                free(str);
                break;
            }
            case OP_ARRAY_EXTEND: _Append(&result, "OP_ARRAY_EXTEND\n"); break;
            case OP_ARRAY_PUSH: _Append(&result, "OP_ARRAY_PUSH\n"); break;
            case OP_ARRAY_MAKE: {
                int size = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_ARRAY_MAKE %d\n", size);
                ip += 4;
                break;
            }
            case OP_OBJECT_EXTEND: _Append(&result, "OP_OBJECT_EXTEND\n"); break;
            case OP_OBJECT_PLUCK_ATTRIBUTE: {
                String str = _ReadString(uf->Codes, ip);
                _AppendFmt(&result, "OP_PLUCK_ATTRIBUTE \"%s\"\n", str);
                ip += strlen(str) + 1;
                free(str);
                break;
            }
            case OP_OBJECT_MAKE: {
                int size = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_OBJECT_MAKE %d\n", size);
                ip += 4;
                break;
            }
            case OP_CLASS_EXTEND: _Append(&result, "OP_CLASS_EXTEND\n"); break;
            case OP_CLASS_MAKE: {
                String str = _ReadString(uf->Codes, ip);
                _AppendFmt(&result, "OP_CLASS_MAKE \"%s\"\n", str);
                ip += strlen(str) + 1;
                free(str);
                break;
            }
            case OP_CLASS_DEFINE_STATIC_MEMBER: _Append(&result, "OP_CLASS_DEFINE_STATIC_MEMBER\n"); break;
            case OP_CLASS_DEFINE_INSTANCE_MEMBER: _Append(&result, "OP_CLASS_DEFINE_INSTANCE_MEMBER\n"); break;
            case OP_SET_INDEX: _Append(&result, "OP_SET_INDEX\n"); break;
            case OP_GET_INDEX: _Append(&result, "OP_GET_INDEX\n"); break;
            case OP_LOAD_FUNCTION_CLOSURE: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_LOAD_FUNCTION_CLOSURE %d\n", offset);
                ip += 4;
                break;
            }
            case OP_LOAD_FUNCTION: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_LOAD_FUNCTION %d\n", offset);
                ip += 4;
                break;
            }
            case OP_CALL_CTOR: {
                int argc = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_CALL_CTOR %d\n", argc);
                ip += 4;
                break;
            }
            case OP_CALL: {
                int argc = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_CALL %d\n", argc);
                ip += 4;
                break;
            }
            case OP_CALL_METHOD: {
                int argc = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_CALL_METHOD %d\n", argc);
                ip += 4;
                break;
            }
            case OP_NOT: _Append(&result, "OP_NOT\n"); break;
            case OP_POS: _Append(&result, "OP_POS\n"); break;
            case OP_NEG: _Append(&result, "OP_NEG\n"); break;
            case OP_MUL: _Append(&result, "OP_MUL\n"); break;
            case OP_DIV: _Append(&result, "OP_DIV\n"); break;
            case OP_MOD: _Append(&result, "OP_MOD\n"); break;
            case OP_INC: _Append(&result, "OP_INC\n"); break;
            case OP_POSTINC: _Append(&result, "OP_POSTINC\n"); break;
            case OP_ADD: _Append(&result, "OP_ADD\n"); break;
            case OP_DEC: _Append(&result, "OP_DEC\n"); break;
            case OP_POSTDEC: _Append(&result, "OP_POSTDEC\n"); break;
            case OP_SUB: _Append(&result, "OP_SUB\n"); break;
            case OP_LSHFT: _Append(&result, "OP_LSHFT\n"); break;
            case OP_RSHFT: _Append(&result, "OP_RSHFT\n"); break;
            case OP_LT: _Append(&result, "OP_LT\n"); break;
            case OP_LTE: _Append(&result, "OP_LTE\n"); break;
            case OP_GT: _Append(&result, "OP_GT\n"); break;
            case OP_GTE: _Append(&result, "OP_GTE\n"); break;
            case OP_EQ: _Append(&result, "OP_EQ\n"); break;
            case OP_NE: _Append(&result, "OP_NE\n"); break;
            case OP_AND: _Append(&result, "OP_AND\n"); break;
            case OP_OR: _Append(&result, "OP_OR\n"); break;
            case OP_XOR: _Append(&result, "OP_XOR\n"); break;
            case OP_STORE_CAPTURE: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_STORE_CAPTURE %d\n", offset);
                ip += 4;
                break;
            }
            case OP_STORE_NAME: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_STORE_NAME %d\n", offset);
                ip += 4;
                break;
            }
            case OP_STORE_LOCAL: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_STORE_LOCAL %d\n", offset);
                ip += 4;
                break;
            }
            case OP_DUPTOP: _Append(&result, "OP_DUPTOP\n"); break;
            case OP_DUP2: _Append(&result, "OP_DUP2\n"); break;
            case OP_POPTOP: _Append(&result, "OP_POPTOP\n"); break;
            case OP_ROT2: _Append(&result, "OP_ROT2\n"); break;
            case OP_ROT3: _Append(&result, "OP_ROT3\n"); break;
            case OP_ROT4: _Append(&result, "OP_ROT4\n"); break;
            case OP_SETUP_TRY: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_SETUP_TRY %d\n", offset);
                ip += 4;
                break;
            }
            case OP_POP_TRY: _Append(&result, "OP_POP_TRY\n"); break;
            case OP_POPN_TRY: {
                int size = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_POPN_TRY %d\n", size);
                ip += 4;
                break;
            }
            case OP_JUMP_IF_FALSE_OR_POP: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_JUMP_IF_FALSE_OR_POP %d\n", offset);
                ip += 4;
                break;
            }
            case OP_JUMP_IF_TRUE_OR_POP: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_JUMP_IF_TRUE_OR_POP %d\n", offset);
                ip += 4;
                break;
            }
            case OP_POP_JUMP_IF_FALSE: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_POP_JUMP_IF_FALSE %d\n", offset);
                ip += 4;
                break;
            }
            case OP_JUMP: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_JUMP %d\n", offset);
                ip += 4;
                break;
            }
            case OP_ABSOLUTE_JUMP: {
                int offset = _ReadOffset(uf->Codes, ip);
                _AppendFmt(&result, "OP_ABSOLUTE_JUMP %d\n", offset);
                ip += 4;
                break;
            }
            case OP_RETURN: _Append(&result, "OP_RETURN\n"); break;
            default:
                _AppendFmt(&result, "UNKNOWN_OPCODE %d\n", opcode);
                break;
        }
    }
    return result;
}
