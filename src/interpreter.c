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

#define SetLocal(envObj, offset, value) (((Environment*)envObj->Value.Opaque)->Locals[offset] = value)
#define GetName(envObj, offset) (((Environment*)envObj->Value.Opaque)->Parent->Locals[offset])
#define GetLocal(envObj, offset) (((Environment*)envObj->Value.Opaque)->Locals[offset])

static int _ReadOffset(uint8_t* codes, int alignStart) {
    int offset = 0;
    offset |= codes[alignStart + 0] << 24;
    offset |= codes[alignStart + 1] << 16;
    offset |= codes[alignStart + 2] << 8;
    offset |= codes[alignStart + 3] << 0;
    return offset;
}

static void _Run(Interpreter* interpreter, UserFunction* uf, Value* envObj) {
    int ip = 0;
    uint8_t opcode;
    Value* lhs = NULL, *rhs = NULL, *res;

    #define Forward(size) (ip += size)
    #define JmpFrwd(addr) (ip  = addr)

    Mark(envObj);

    while (ip < uf->CodeC) {
        opcode = uf->Codes[ip++];
        switch (opcode) {
            case OP_LOAD_NAME: {
                int offset = _ReadOffset(uf->Codes, ip);
                Push(GetName(envObj, offset));
                Forward(4);
                break;
            }
            case OP_LOAD_LOCAL: {
                int offset = _ReadOffset(uf->Codes, ip);
                Push(GetLocal(envObj, offset));
                Forward(4);
                break;
            }
            case OP_LOAD_CONST: {
                int offset = _ReadOffset(uf->Codes, ip);
                Push(interpreter->Constants[offset]);
                Forward(4);
                break;
            }
            case OP_LOAD_BOOL: {
                int offset = _ReadOffset(uf->Codes, ip);
                Push(offset == 0 ? interpreter->False : interpreter->True);
                Forward(4);
                break;
            }
            case OP_LOAD_NULL: {
                Push(interpreter->Null);
                break;
            }
            case OP_LOAD_FUNCTION: {
                int offset = _ReadOffset(uf->Codes, ip);
                DoLoadFunction(interpreter, offset, &res);
                Push(res);
                Forward(4);
                break;
            }
            case OP_CALL: {
                int argc = _ReadOffset(uf->Codes, ip);
                Forward(4);

                // Call
                Value* function = Popp();
                UserFunction* fn = (UserFunction*) function->Value.Opaque;
                Environment* parentEnv = (Environment*) envObj->Value.Opaque;
                Environment* env = CreateEnvironment(parentEnv, fn->LocalC);

                if (argc != fn->Argc) {
                    printf("Expected %d arguments, got %d\n", fn->Argc, argc);
                    exit(EXIT_FAILURE);
                }

                _Run(interpreter, fn, NewEnvironmentValue(interpreter, env));
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
            case OP_STORE_NAME: {
                int offset = _ReadOffset(uf->Codes, ip);
                SetLocal(envObj, offset, Popp());
                Forward(4);
                break;
            }
            case OP_STORE_LOCAL: {
                int offset = _ReadOffset(uf->Codes, ip);
                SetLocal(envObj, offset, Popp());
                Forward(4);
                break;
            }
            case OP_POPTOP: {
                Value* val = Popp();
                printf("POPPED: %s\n", ValueToString(val));
                break;
            }
            case OP_JUMP_IF_FALSE_OR_POP: {
                int offset = _ReadOffset(uf->Codes, ip);
                if (!ValueToBool(Peek())) {
                    JmpFrwd(offset);
                } else {
                    Popp();
                    Forward(4);
                }
                break;
            }
            case OP_JUMP_IF_TRUE_OR_POP: {
                int offset = _ReadOffset(uf->Codes, ip);
                if (ValueToBool(Peek())) {
                    JmpFrwd(offset);
                } else {
                    Popp();
                    Forward(4);
                }
                break;
            }
            case OP_POP_JUMP_IF_FALSE: {
                int offset = _ReadOffset(uf->Codes, ip);
                Value* val = Peek();
                if (!ValueToBool(val)) {
                    JmpFrwd(offset);
                } else {
                    Forward(4);
                }
                break;
            }
            case OP_JUMP: {
                int offset = _ReadOffset(uf->Codes, ip);
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

void _RunProgram(Interpreter* interpreter, Value* ufValue) {
    UserFunction* uf = (UserFunction*) ufValue->Value.Opaque;
    Environment* env = CreateEnvironment(NULL, uf->LocalC);
    _Run(interpreter, uf, NewEnvironmentValue(interpreter, env));
    ForceGarbageCollect(interpreter);
}

void Interpret(Interpreter* interpreter, Value* ufValue /*UserFunction*/) {
    _RunProgram(interpreter, ufValue);
}