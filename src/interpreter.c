#include "./interpreter.h"

Interpreter* CreateInterpreter() {
    Interpreter* interpreter                = Allocate(sizeof(Interpreter));
    interpreter->Allocated                  = 0;
    interpreter->GcRoot                     = NULL;
    interpreter->True                       = NewBoolValue(interpreter, 1);
    interpreter->False                      = NewBoolValue(interpreter, 0);
    interpreter->Null                       = NewNullValue(interpreter);
    interpreter->Constants                  = Allocate(sizeof(Value*));
    interpreter->ConstantC                  = 0;
    interpreter->Constants[0]               = NULL;
    interpreter->Functions                  = Allocate(sizeof(Value*));
    interpreter->FunctionC                  = 0;
    interpreter->Functions[0]               = NULL;
    // interpreter->Stacks[STACK_SIZE];
    interpreter->StackC                     = 0;
    // interpreter->ExceptionHandlerStacks[STACK_SIZE];
    interpreter->ExceptionHandlerStackC     = 0;
    return interpreter;
}

#define Push(value) (interpreter->Stacks[interpreter->StackC++] = value)
#define Popp() (interpreter->Stacks[--interpreter->StackC])
#define Peek() (interpreter->Stacks[interpreter->StackC  - 1])

#define PushEH(addr) (interpreter->ExceptionHandlerStacks[interpreter->ExceptionHandlerStackC++] = addr)
#define PoppEH() (interpreter->ExceptionHandlerStacks[--interpreter->ExceptionHandlerStackC])
#define PeekEH() (interpreter->ExceptionHandlerStacks[interpreter->ExceptionHandlerStackC - 1])

#define DumpFrame() do { \
    printf("Stack: "); \
    for (int i = 0; i < uf->CodeC; i++) { \
        printf("%d ", uf->Codes[i]); \
    } \
    printf("\n"); \
} while (0)




#define DumpStack() do { \
    printf("Stack [%d items]: [ ", interpreter->StackC); \
    for (int i = 0; i < interpreter->StackC; i++) { \
        if (i > 0) printf(", "); \
        printf("%s", ValueToString(interpreter->Stack[i])); \
    } \
    printf(" ]\n"); \
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

#define InterpreterPanic(message, ...) do { \
    fprintf(stderr, "[%s:%d]::Panic: ", __FILE__, __LINE__); \
    fprintf(stderr, "%s", (char*) message); \
    fprintf(stderr, "\n"); \
    ForceGarbageCollect(interpreter); \
    FreeInterpreter(interpreter); \
    exit(EXIT_FAILURE); \
} while(0)

#define HandleError(messageFormat, ...) { \
    int size = snprintf(NULL, 0, (char*) messageFormat, ##__VA_ARGS__) + 1; \
    String message = (String) Allocate(size); \
    snprintf(message, size, (char*) messageFormat, ##__VA_ARGS__); \
    if (catched) { \
        JmpFrwd(PeekEH()); \
        PoppEH(); \
        Push(NewErrorValue(interpreter, message)); \
        free(message); \
        break; \
    } \
    InterpreterPanic(message); \
    free(message); \
    continue; } \

static void _Run(Interpreter* interpreter, Value* fnValue, Value* rootEnvObj, Value* envObj) {
    UserFunction* uf = ValueToUFn(fnValue);
    uint8_t opcode   = 0;
    Value* lhs       = NULL;
    Value* rhs       = NULL;
    Value* res       = NULL;
    Value* err       = NULL;
    int ip           = 0;
    int offset       = 0;
    int argc         = 0;
    bool catched     = false;
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

        catched = interpreter->ExceptionHandlerStackC != 0;

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
                    Panic("Object does not have attribute '%s'\n", str);
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
                    Panic("Attempted to call a null value");
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
                UserFunction* uf = ValueToUFn(function);
                Environment* env = CreateEnvironment(envObj, uf->LocalC);

                if (!ValueIsCallable(function)) {
                    Panic("Attempted to call a non-callable value: %s\n", ValueToString(function));
                }

                if (argc != uf->Argc) {
                    Panic("Expected %d arguments, got %d\n", uf->Argc, argc);
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
                    Panic("invalid operation");
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
                    Panic("invalid operation");
                } else if (result == FLG_INVALID_OPERATION) {
                    Panic("invalid operation");
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
                    Panic("invalid operation");
                } else if (result == FLG_INVALID_OPERATION) {
                    Panic("invalid operation");
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
                    HandleError(
                        "invalid operation (+) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
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
                    Panic("invalid operation");
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
                    Panic("invalid operation");
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
                    Panic("invalid operation");
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
                   Panic("invalid operation %s and %s", ValueToString(lhs), ValueToString(rhs));
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
                    Panic("invalid operation");
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
                    Panic("invalid operation");
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
                    Panic("invalid operation");
                }
                Push(res);
                break;
            }
            case OP_EQ: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoEQ(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    Panic("invalid operation");
                }
                Push(res);
                break;
            }
            case OP_NE: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                int result = DoNE(interpreter, lhs, rhs, &res);
                if (result == FLG_INVALID_OPERATION) {
                    Panic("invalid operation");
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
            case OP_SETUP_TRY: {
                offset = _ReadOffset(uf->Codes, ip);
                PushEH(offset);
                Forward(4);
                break;
            }
            case OP_POP_TRY: {
                PoppEH();
                break;
            }
            case OP_JUMP_IF_FALSE_OR_POP: {
                offset = _ReadOffset(uf->Codes, ip);
                res    = Peek();
                if (!ValueToBool(res)) {
                    JmpFrwd(offset);
                } else {
                    Popp();
                    Forward(4);
                }
                break;
            }
            case OP_JUMP_IF_TRUE_OR_POP: {
                offset = _ReadOffset(uf->Codes, ip);
                res    = Peek();
                if (ValueToBool(res)) {
                    JmpFrwd(offset);
                } else {
                    Popp();
                    Forward(4);
                }
                break;
            }
            case OP_POP_JUMP_IF_FALSE: {
                offset = _ReadOffset(uf->Codes, ip);
                res    = Popp();
                if (ValueToBool(res) == false) {
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
                Panic("Unknown opcode: %d %d\n", opcode, OP_LOAD_NAME);
                return;
            }
        }
    }
}

void _RunProgram(Interpreter* interpreter, Value* fnValue) {
    UserFunction* uf = ValueToUFn(fnValue);
    Environment* env = CreateEnvironment(NULL, uf->LocalC);
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