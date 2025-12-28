#include "./interpreter.h"

Interpreter* CreateInterpreter() {
    Interpreter* interpreter    = Allocate(sizeof(Interpreter));
    interpreter->Allocated      = 0;
    interpreter->GcRoot         = NULL;
    interpreter->True           = NewBoolValue(interpreter, 1);
    interpreter->False          = NewBoolValue(interpreter, 0);
    interpreter->Null           = NewNullValue(interpreter);
    interpreter->Constants      = Allocate(sizeof(Value*));
    interpreter->ConstantC      = 0;
    interpreter->Constants[0]   = NULL;
    interpreter->Functions      = Allocate(sizeof(Value*));
    interpreter->FunctionC      = 0;
    interpreter->Functions[0]   = NULL;
    interpreter->StackC         = 0;
    return interpreter;
}

#define Push(value) (interpreter->Stack[interpreter->StackC++] = value)
#define Popp() (interpreter->Stack[--interpreter->StackC])
#define Peek() (interpreter->Stack[interpreter->StackC  - 1])
#define DumpFrame() do { \
    printf("Stack: "); \
    for (int i = 0; i < uf->CodeC; i++) { \
        printf("%d ", uf->Codes[i]); \
    } \
    printf("\n"); \
} while (0)

#define SetVar(envObj, offset, value) EnvironmentSetLocal((Environment*)envObj->Value.Opaque, offset, value)
#define GetVar(envObj, offset) EnvironmentGetLocal((Environment*)envObj->Value.Opaque, offset)->Value
#define ValueToUFn(value) ((UserFunction*) value->Value.Opaque)

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

static void _Run(Interpreter* interpreter, Value* fnValue, Value* rootEnvObj, Value* envObj) {
    UserFunction* uf = ValueToUFn(fnValue);
    uint8_t opcode   = 0;
    Value* lhs       = NULL;
    Value* rhs       = NULL;
    Value* res       = NULL;
    int ip           = 0;
    int offset       = 0;
    int argc         = 0;
    String str       = NULL;

    #define Forward(size) (ip += size)
    #define JmpFrwd(addr) (ip  = addr)

    while (ip < uf->CodeC) {
        opcode = uf->Codes[ip++];

        if (interpreter->Allocated >= GC_THRESHOLD) {
            Mark(rootEnvObj);
            Mark(envObj);
            GarbageCollect(interpreter);
        }

        switch (opcode) {
            case OP_IMPORT_CORE: {
                str = _ReadString(uf->Codes, ip);
                DoImportCore(interpreter, str, &res);
                Push(res);
                Forward(strlen(str) + 1);
                free(str);
                str = NULL;
                break;
            }
            case OP_LOAD_NAME: {
                offset = _ReadOffset(uf->Codes, ip);
                Push(GetVar(rootEnvObj, offset));
                Forward(4);
                break;
            }
            case OP_LOAD_LOCAL: {
                offset = _ReadOffset(uf->Codes, ip);
                Push(GetVar(envObj, offset));
                Forward(4);
                break;
            }
            case OP_LOAD_CONST: {
                offset = _ReadOffset(uf->Codes, ip);
                Push(interpreter->Constants[offset]);
                Forward(4);
                break;
            }
            case OP_LOAD_BOOL: {
                offset = _ReadOffset(uf->Codes, ip);
                Push(offset == 0 ? interpreter->False : interpreter->True);
                Forward(4);
                break;
            }
            case OP_LOAD_NULL: {
                Push(interpreter->Null);
                break;
            }
            case OP_LOAD_FUNCTION: {
                offset = _ReadOffset(uf->Codes, ip);
                DoLoadFunction(interpreter, offset, &res);
                Push(res);
                Forward(4);
                break;
            }
            case OP_PLUCK_ATTRIBUTE: {
                str = _ReadString(uf->Codes, ip);
                Value* object = Peek();
                
                HashMap* map = (HashMap*) object->Value.Opaque;

                if (!HashMapContains(map, str)) {
                    printf("Object does not have attribute '%s'\n", str);
                    exit(EXIT_FAILURE);
                }
                res = HashMapGet(map, str);

                Push(res);
                Forward(strlen(str) + 1);
                free(str);
                str = NULL;
                break;
            }
            case OP_CALL: {
                argc = _ReadOffset(uf->Codes, ip);
                Forward(4);

                Value* function = Popp();

                if (function == NULL) {
                    printf("Attempted to call a null value\n");
                    exit(EXIT_FAILURE);
                }

                if (ValueIsNativeFunction(function)) {
                    NativeFunction nf = (NativeFunction) function->Value.Opaque;
                    Value** args = Allocate(sizeof(Value*) * argc);
                    for (int i = argc - 1; i >= 0; i--) {
                        args[i] = Popp();
                    }
                    Value* nativeResult = nf(interpreter, argc, args);

                    Push(nativeResult);
                    break;
                }

                // Call
                UserFunction* uf       = ValueToUFn(function);
                Environment* parentEnv = (Environment*) rootEnvObj->Value.Opaque;
                Environment* env       = CreateEnvironment(uf->LocalC);

                if (!ValueIsCallable(function)) {
                    printf("Attempted to call a non-callable value: %s\n", ValueToString(function));
                    exit(EXIT_FAILURE);
                }

                if (argc != uf->Argc) {
                    printf("Expected %d arguments, got %d\n", uf->Argc, argc);
                    exit(EXIT_FAILURE);
                }

                _Run(interpreter, function, rootEnvObj, NewEnvironmentValue(interpreter, env));
                break;
            }
            case OP_MUL: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoMul(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation for %s * %s\n", ValueToString(lhs), ValueToString(rhs));
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_DIV: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoDiv(interpreter, lhs, rhs, &res);
                if (result == FLG_ZERO_DIV) {
                    printf("Division by zero\n");
                    exit(EXIT_FAILURE);
                } else if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation\n");
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_MOD: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoMod(interpreter, lhs, rhs, &res);
                if (result == FLG_ZERO_DIV) {
                    printf("Modulo by zero\n");
                    exit(EXIT_FAILURE);
                } else if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation\n");
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_ADD: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoAdd(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation\n");
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_SUB: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoSub(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation\n");
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_LSHFT: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoLShift(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation\n");
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_RSHFT: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoRShift(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation\n");
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_LT: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoLT(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation\n");
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_LTE: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoLTE(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation\n");
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_GT: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoGT(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation\n");
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_GTE: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoGTE(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    printf("Invalid operation\n");
                    exit(EXIT_FAILURE);
                }
                Push(res);
                break;
            }
            case OP_STORE_NAME: {
                offset = _ReadOffset(uf->Codes, ip);
                SetVar(rootEnvObj, offset, Popp());
                Forward(4);
                break;
            }
            case OP_STORE_LOCAL: {
                offset = _ReadOffset(uf->Codes, ip);
                SetVar(envObj, offset, Popp());
                Forward(4);
                break;
            }
            case OP_DUPTOP: {
                Push(Peek());
                break;
            }
            case OP_POPTOP: {
                Value* val = Popp();
                break;
            }
            case OP_JUMP_IF_FALSE_OR_POP: {
                offset = _ReadOffset(uf->Codes, ip);
                if (!ValueToBool(Peek())) {
                    JmpFrwd(offset);
                } else {
                    Popp();
                    Forward(4);
                }
                break;
            }
            case OP_JUMP_IF_TRUE_OR_POP: {
                offset = _ReadOffset(uf->Codes, ip);
                if (ValueToBool(Peek())) {
                    JmpFrwd(offset);
                } else {
                    Popp();
                    Forward(4);
                }
                break;
            }
            case OP_POP_JUMP_IF_FALSE: {
                offset = _ReadOffset(uf->Codes, ip);
                Value* val = Popp();
                if (!ValueToBool(val)) {
                    JmpFrwd(offset);
                } else {
                    Forward(4);
                }
                break;
            }
            case OP_JUMP: {
                offset = _ReadOffset(uf->Codes, ip);
                JmpFrwd(offset);
                break;
            }
            case OP_ABSOLUTE_JUMP: {
                offset = _ReadOffset(uf->Codes, ip);
                JmpFrwd(offset);
                break;
            }
            case OP_RETURN: {
                return;
            }
            default: {
                printf("Unknown opcode: %d %d\n", opcode, OP_LOAD_NAME);
                exit(EXIT_FAILURE);
                break;
            }
        }
    }
}

void _RunProgram(Interpreter* interpreter, Value* fnValue) {
    UserFunction* uf = ValueToUFn(fnValue);
    Environment* env = CreateEnvironment(uf->LocalC);
    Value* envObj    = NewEnvironmentValue(interpreter, env);
    _Run(interpreter, fnValue, envObj, envObj);
    ForceGarbageCollect(interpreter);
}

void Interpret(Interpreter* interpreter, Value* fnValue /*UserFunction*/) {
    _RunProgram(interpreter, fnValue);
}

void FreeInterpreter(Interpreter* interpreter) {
    free(interpreter->Constants);
    free(interpreter->Functions);
    free(interpreter);
}