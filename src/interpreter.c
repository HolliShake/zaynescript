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
#define PopN(n) (interpreter->Stacks[interpreter->StackC -= (n)])
#define Peek() (interpreter->Stacks[interpreter->StackC  - 1])
#define PeekAt(n) (interpreter->Stacks[interpreter->StackC - n])

#define PushEH(addr) (interpreter->ExceptionHandlerStacks[interpreter->ExceptionHandlerStackC++] = addr)
#define PoppEH() (interpreter->ExceptionHandlerStacks[--interpreter->ExceptionHandlerStackC])
#define PopNEH(n) (interpreter->ExceptionHandlerStacks[interpreter->ExceptionHandlerStackC -= (n)])
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
        printf("%s", ValueToString(interpreter->Stacks[i])); \
    } \
    printf(" ]\n"); \
} while (0)

#define SetVar(envObj, offset, value) EnvironmentSetLocal(CoerceToEnvironment(envObj), offset, value)
#define GetVar(envObj, offset) EnvironmentGetLocal(CoerceToEnvironment(envObj), offset)->Value

static int _ReadInt32(uint8_t* codes, int alignStart) {
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
    break; } \

static void _Run(Interpreter* interpreter, Value* fnValue, Value* rootEnvObj, Value* envObj) {
    UserFunction* uf = CoerceToUserFunction(fnValue);
    uint8_t opcode   = 0;
    Value* lhs       = NULL;
    Value* rhs       = NULL;
    Value* res       = NULL;
    Value* ext       = NULL;
    Value* arr       = NULL;
    Value* obj       = NULL;
    Value* cls       = NULL;
    Value* key       = NULL;
    Value* val       = NULL;
    Value* err       = NULL;
    HashMap* map     = NULL;
    Array* array     = NULL;
    int ip           = 0;
    int offset       = 0;
    int argc         = 0;
    int flg          = 0;
    int size         = 0;
    bool catched     = false;
    String str       = NULL;

    #define Forward(size) (ip += size)
    #define JmpFrwd(addr) (ip  = addr)

    while (ip != uf->CodeC) {

        if (interpreter->Allocated >= GC_THRESHOLD) {
            Mark(fnValue);
            Mark(rootEnvObj);
            Mark(envObj);
            GarbageCollect(interpreter);
        }

        opcode = uf->Codes[ip++];

        catched = interpreter->ExceptionHandlerStackC != 0;

        switch (opcode) {
            case OP_IMPORT_CORE: {
                str = _ReadString(uf->Codes, ip);
                flg = DoImportCore(interpreter, str, &res);
                if (flg == FLG_NOTFOUND)
                    HandleError(
                        "core module '%s' not found", 
                        str
                    );
                Push(res);
                Forward(strlen(str) + 1);
                free(str);
                break;
            }
            case OP_LOAD_CAPTURE: {
                offset = _ReadInt32(uf->Codes, ip);
                val    = (uf->Captures[offset]->Value);
                if (val == NULL)
                    HandleError(
                        "captured variable is referenced before initialization"
                    );
                Push(val);
                Forward(4);
                break;
            }
            case OP_LOAD_NAME: {
                offset = _ReadInt32(uf->Codes, ip);
                val    = GetVar(rootEnvObj, offset);
                if (val == NULL)
                    HandleError(
                        "variable is referenced before initialization"
                    );
                Push(val);
                Forward(4);
                break;
            }
            case OP_LOAD_LOCAL: {
                offset = _ReadInt32(uf->Codes, ip);
                val    = GetVar(envObj, offset);
                if (val == NULL)
                    HandleError(
                        "variable is referenced before initialization"
                    );
                Push(val);
                Forward(4);
                break;
            }
            case OP_LOAD_CONST: {
                offset = _ReadInt32(uf->Codes, ip);
                Push(interpreter->Constants[offset]);
                Forward(4);
                break;
            }
            case OP_LOAD_BOOL: {
                offset = _ReadInt32(uf->Codes, ip);
                Push(
                    offset 
                    ? interpreter->False 
                    : interpreter->True
                );
                Forward(4);
                break;
            }
            case OP_LOAD_NULL: {
                Push(interpreter->Null);
                break;
            }
            case OP_LOAD_STRING: {
                str = _ReadString(uf->Codes, ip);
                Push(NewStrValue(interpreter, str));
                Forward(strlen(str) + 1);
                free(str);
                break;
            }
            case OP_ARRAY_EXTEND: {
                ext = Popp();
                arr = Peek();
                if (!ValueIsArray(ext))
                    HandleError(
                        "expected array to extend to be an array, got %s", 
                        ValueTypeOf(ext)
                    );
                ArrayExtend(CoerceToArray(arr), CoerceToArray(ext));
                break;
            }
            case OP_ARRAY_PUSH: {
                val = Popp();
                arr = Peek();
                if (!ValueIsArray(arr))
                    HandleError(
                        "expected array to push to be an array, got %s", 
                        ValueTypeOf(arr)
                    );
                ArrayPush(CoerceToArray(arr), val);
                break;
            }
            case OP_ARRAY_MAKE: {
                size  = _ReadInt32(uf->Codes, ip);
                arr   = NewArrayValue(interpreter);
                array = CoerceToArray(arr);
                for (int i = 0; i < size; i++) {
                    val = PeekAt(size + i);
                    ArrayPush(array, val);
                }
                PopN(size);
                Push(arr);
                Forward(4);
                break;
            }
            case OP_OBJECT_EXTEND: {
                ext = Popp();
                obj = Peek();
                if (!ValueIsObject(ext))
                    HandleError(
                        "expected object to extend to be an object, got %s", 
                        ValueTypeOf(ext)
                    );
                HashMapExtend(CoerceToHashMap(obj), CoerceToHashMap(ext));
                break;
            }
            case OP_OBJECT_MAKE: {
                size = _ReadInt32(uf->Codes, ip);
                obj  = NewObjectValue(interpreter);
                map  = CoerceToHashMap(obj);
                for (int i = 0; i < size; i++) {
                    key = Popp();
                    val = Popp();
                    HashMapSet(map, ValueToString(key), val);
                }
                Push(obj);
                Forward(4);
                break;
            }
            case OP_OBJECT_PLUCK_ATTRIBUTE: {
                str = _ReadString(uf->Codes, ip);
                obj = Peek();
                map = CoerceToHashMap(obj);

                if (!HashMapContains(map, str)) {
                    Panic("object does not have attribute '%s'\n", str);
                }

                res = HashMapGet(map, str);

                Push(res);
                Forward(strlen(str) + 1);
                free(str);
                break;
            }
            case OP_CLASS_EXTEND: {
                ext = Popp(); // super class
                cls = Peek(); // class being extended
                if (!ValueIsClass(ext))
                    HandleError(
                        "expected superclass to be a class, got %s", 
                        ValueTypeOf(ext)
                    );
                ClassExtend(CoerceToUserClass(cls), ext);
                break;
            }
            case OP_CLASS_MAKE: {
                str = _ReadString(uf->Codes, ip);
                obj = NewClassValue(interpreter, CreateUserClass(AllocateString(str), NULL));
                Push(obj);
                Forward(strlen(str) + 1);
                free(str);
                break;
            }
            case OP_CLASS_DEFINE_STATIC_MEMBER: 
            case OP_CLASS_DEFINE_INSTANCE_MEMBER: {
                bool isStatic = (opcode == OP_CLASS_DEFINE_STATIC_MEMBER);
                key = Popp();
                val = Popp();
                obj = Peek();
                ClassDefineMember(CoerceToUserClass(obj), key, val, isStatic);
                break;
            }
            case OP_SET_INDEX: {
                val = Popp();
                key = Popp();
                obj = Peek();
                if (!ValueIsObject(obj)) {
                    HandleError(
                        "expected object to set attribute on to be an object, got %s", 
                        ValueTypeOf(obj)
                    );
                }
                HashMapSet(CoerceToHashMap(obj), ValueToString(key), val);
                break;
            }
            case OP_GET_INDEX: {
                key = Popp();
                obj = Popp();
                if (!ValueIsObject(obj))
                    HandleError(
                        "expected object to get attribute from to be an object, got %s", 
                        ValueTypeOf(obj)
                    );
    
                res = HashMapGet(CoerceToHashMap(obj), ValueToString(key));

                if (res == NULL)
                    HandleError(
                        "object (%s) has no attribute '%s'", 
                        ValueToString(obj),
                        ValueToString(key)
                    );
                Push(res);
                break;
            }
            case OP_LOAD_FUNCTION_CLOSURE:
            case OP_LOAD_FUNCTION: {
                offset = _ReadInt32(uf->Codes, ip);
                res    = NULL;
                DoLoadFunction(interpreter, rootEnvObj, envObj, offset, opcode == OP_LOAD_FUNCTION_CLOSURE, &res);
                Push(res);
                Forward(4);
                break;
            }
            case OP_CALL_CTOR: {
                argc = _ReadInt32(uf->Codes, ip);
                Forward(4);

                obj  = Popp();

                if (!ValueIsClass(obj))
                    HandleError(
                        "attempted to allocate non-class value of type %s", 
                        ValueTypeOf(obj)
                    );
                
                if (!ClassHasMember(CoerceToUserClass(obj), CONSTRUCTOR_NAME, false, true)) {
                    PopN(argc);
                    // Push default instance, no constructor call
                    ClassInstance* instance = CreateClassInstance(obj);
                    Push(NewClassInstanceValue(interpreter, instance));
                }
                break;
            }
            case OP_CALL: {
                argc = _ReadInt32(uf->Codes, ip);
                Forward(4);

                obj = Popp();

                if (obj == NULL) {
                    Panic("Attempted to call a null value");
                }

                if (ValueIsNativeFunction(obj)) {
                    NativeFunctionMeta* nFMeta = CoerceToNativeFunctionMeta(obj);
                    NativeFunction nf          = nFMeta->FuncPtr;

                    if (nFMeta->Argc != VARARG && argc != nFMeta->Argc) 
                        HandleError(
                            "expected %d arguments, got %d", nFMeta->Argc, argc
                        );

                    Value** args = Allocate(sizeof(Value*) * argc);
                    for (int i = argc - 1; i >= 0; i--) {
                        args[i] = Popp();
                    }
                    Value* nativeResult = nf(interpreter, argc, args);
                    Push(nativeResult);
                    free(args);
                    break;
                }

                // Call
                UserFunction* uf = CoerceToUserFunction(obj);
                Environment* env = CreateEnvironment(envObj, uf->LocalC);

                if (!ValueIsCallable(obj)) {
                    Panic("Attempted to call a non-callable value: %s\n", ValueToString(obj));
                }

                if (argc != uf->Argc) {
                    Panic("Expected %d arguments, got %d\n", uf->Argc, argc);
                }
                
                // printf("Running function %s with %d args\n", uf->Name, argc);
                _Run(
                    interpreter, 
                    obj, 
                    uf->ParentEnv != NULL ? uf->ParentEnv : rootEnvObj, 
                    NewEnvironmentValue(interpreter, env)
                );
                break;
            }
            case OP_MUL: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoMul(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (*) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_DIV: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoDiv(interpreter, lhs, rhs, &res);
                if (flg == FLG_ZERO_DIV) 
                    HandleError(
                        "zero division error"
                    )
                else if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (/) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_MOD: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoMod(interpreter, lhs, rhs, &res);
                if (flg == FLG_ZERO_DIV) 
                    HandleError(
                        "zero division error"
                    )
                else if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (%%) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_POSTINC: {
                // bot [obj, key, val] top
                rhs = Popp(); // old value
                res = NULL;
                flg = DoInc(interpreter, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (++) for type %s", 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                Push(rhs);
                break;
            }
            case OP_INC: {
                lhs = Popp();
                res = NULL;
                flg = DoInc(interpreter, lhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (++) for type %s", 
                        ValueTypeOf(lhs)
                    );
                Push(res);
                break;
            }
            case OP_ADD: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoAdd(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (+) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_POSTDEC: {
                // bot [obj, key, val] top
                rhs = Popp(); // old value
                res = NULL;
                flg = DoDec(interpreter, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (--) for type %s", 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                Push(rhs);
                break;
            }
            case OP_DEC: {
                lhs = Popp();
                res = NULL;
                flg = DoDec(interpreter, lhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (--) for type %s", 
                        ValueTypeOf(lhs)
                    );
                Push(res);
                break;
            }
            case OP_SUB: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoSub(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (-) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_LSHFT: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoLShift(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (<<) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_RSHFT: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoRShift(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (>>) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_LT: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoLT(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (<) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_LTE: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoLTE(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (<=) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_GT: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoGT(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (>) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_GTE: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoGTE(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (>=) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_EQ: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoEQ(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (==) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_NE: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoNE(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (!=) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_AND: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoAnd(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (and) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_OR: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoOr(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (or) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_XOR: {
                rhs = Popp();
                lhs = Popp();
                res = NULL;
                flg = DoXor(interpreter, lhs, rhs, &res);
                if (flg == FLG_INVALID_OPERATION) 
                    HandleError(
                        "invalid operation (xor) for type %s and %s", 
                        ValueTypeOf(lhs), 
                        ValueTypeOf(rhs)
                    );
                Push(res);
                break;
            }
            case OP_STORE_NAME: {
                offset = _ReadInt32(uf->Codes, ip);
                SetVar(rootEnvObj, offset, Popp());
                Forward(4);
                break;
            }
            case OP_STORE_LOCAL: {
                offset = _ReadInt32(uf->Codes, ip);
                SetVar(envObj, offset, Popp());
                Forward(4);
                break;
            }
            case OP_DUPTOP: {
                Push(Peek());
                break;
            }
            case OP_DUP2: {
                Value* a = PeekAt(1);
                Value* b = PeekAt(2);
                Push(b);
                Push(a);
                break;
            }
            case OP_POPTOP: {
                Popp();
                break;
            }
            case OP_ROT2: {
                // A B -> B A
                Value* a = PeekAt(1);
                Value* b = PeekAt(2);
                interpreter->Stacks[interpreter->StackC - 1] = b;
                interpreter->Stacks[interpreter->StackC - 2] = a;
                break;
            }
            case OP_ROT3: {
                // A B C -> C A B
                Value* a = PeekAt(1);
                Value* b = PeekAt(2);
                Value* c = PeekAt(3);
                interpreter->Stacks[interpreter->StackC - 1] = c;
                interpreter->Stacks[interpreter->StackC - 2] = a;
                interpreter->Stacks[interpreter->StackC - 3] = b;
                break;
            }
            case OP_ROT4: {
                // A B C D -> D A B C
                Value* d = PeekAt(1);
                Value* c = PeekAt(2);
                Value* b = PeekAt(3);
                Value* a = PeekAt(4);
                interpreter->Stacks[interpreter->StackC - 1] = c;
                interpreter->Stacks[interpreter->StackC - 2] = b;
                interpreter->Stacks[interpreter->StackC - 3] = a;
                interpreter->Stacks[interpreter->StackC - 4] = d;
                break;
            }
            case OP_SETUP_TRY: {
                offset = _ReadInt32(uf->Codes, ip);
                PushEH(offset);
                Forward(4);
                break;
            }
            case OP_POP_TRY: {
                PoppEH();
                break;
            }
            case OP_POPN_TRY: {
                size = _ReadInt32(uf->Codes, ip);
                PopNEH(size);
                Forward(4);
                break;
            }
            case OP_JUMP_IF_FALSE_OR_POP: {
                offset = _ReadInt32(uf->Codes, ip);
                val    = Peek();
                if (!ValueToBool(val)) {
                    JmpFrwd(offset);
                } else {
                    Popp();
                    Forward(4);
                }
                break;
            }
            case OP_JUMP_IF_TRUE_OR_POP: {
                offset = _ReadInt32(uf->Codes, ip);
                val    = Peek();
                if (ValueToBool(val)) {
                    JmpFrwd(offset);
                } else {
                    Popp();
                    Forward(4);
                }
                break;
            }
            case OP_POP_JUMP_IF_FALSE: {
                offset = _ReadInt32(uf->Codes, ip);
                val    = Popp();
                if (ValueToBool(val) == false) {
                    JmpFrwd(offset);
                } else {
                    Forward(4);
                }
                break;
            }
            case OP_JUMP: {
                offset = _ReadInt32(uf->Codes, ip);
                JmpFrwd(offset);
                break;
            }
            case OP_ABSOLUTE_JUMP: {
                offset = _ReadInt32(uf->Codes, ip);
                JmpFrwd(offset);
                break;
            }
            case OP_RETURN: {
                return;
            }
            default: {
                Panic("Unknown opcode: %s, %d %d\n", uf->Name != NULL ? uf->Name : "<anonymous>", opcode, OP_LOAD_NAME);
                return;
            }
        }
    }
}

void _RunProgram(Interpreter* interpreter, Value* fnValue) {
    UserFunction* uf = CoerceToUserFunction(fnValue);
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