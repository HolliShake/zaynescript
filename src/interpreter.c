#include "./interpreter.h" 

Interpreter* CreateInterpreter() {
    Interpreter* interpreter                = Allocate(sizeof(Interpreter));
    interpreter->Allocated                  = 0;
    interpreter->GcRoot                     = NULL;
    interpreter->Array                      = CreateArrayClass(interpreter);
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
#define Popp()      (interpreter->Stacks[--interpreter->StackC])
#define PopN(n)     (interpreter->Stacks[interpreter->StackC -= (n)])
#define Peek()      (interpreter->Stacks[interpreter->StackC - 1])
#define PeekAt(n)   (interpreter->Stacks[interpreter->StackC - n])

#define PushEH(addr) (interpreter->ExceptionHandlerStacks[interpreter->ExceptionHandlerStackC++] = addr)
#define PoppEH()     (interpreter->ExceptionHandlerStacks[--interpreter->ExceptionHandlerStackC])
#define PopNEH(n)    (interpreter->ExceptionHandlerStacks[interpreter->ExceptionHandlerStackC -= (n)])
#define PeekEH()     (interpreter->ExceptionHandlerStacks[interpreter->ExceptionHandlerStackC - 1])

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
#define GetCap(uFunct, offset) (uFunct->Captures[offset]->Value)

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

#define RaiseError(errorValue) { \
    if (catched) { \
        JmpFrwd(PeekEH()); \
        PoppEH(); \
        Push(errorValue); \
        break; \
    } \
    InterpreterPanic(ValueToString(errorValue)); \
    break; } \

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


static int _GetArgc(Value* fn) {
    if (ValueIsClass(fn)) {
        UserClass* cls = CoerceToUserClass(fn);
        if (ClassHasMember(cls, CONSTRUCTOR_NAME, false, true)) {
            Value* constructor = ClassGetMember(cls, CONSTRUCTOR_NAME, false);
            return _GetArgc(constructor);
        } else {
            return 0;
        }
    } else if (ValueIsNativeFunction(fn)) {
        NativeFunctionMeta* nFMeta = CoerceToNativeFunctionMeta(fn);
        return nFMeta->Argc;
    } else if (ValueIsUserFunction(fn)) {
        UserFunction* uf = CoerceToUserFunction(fn);
        return uf->Argc;
    }
    return 0;
}

static int _GetArg2(Interpreter* interp, Value* obj, Value* methodName)  {
    Value* method = GenericGetAttribute(interp, obj, methodName, true);
    if (ValueIsNull(method)) {
        return 0;
    }
    return _GetArgc(method);
}

void Run(Interpreter* interpreter, Value* fnValue, Value* rootEnvObj, Value* envObj) {
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
                res = DoImportCore(interpreter, str);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                Forward(strlen(str) + 1);
                free(str);
                break;
            }
            case OP_LOAD_CAPTURE: {
                offset = _ReadInt32(uf->Codes, ip);
                val    = GetCap(uf, offset);
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
                val = HashMapGet(map, str);
                Push((val == NULL) ? interpreter->Null : val);
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
                //NOTE: memory leak (AllocateString creates a char* string passed to CreateUserClass. CreateUserClass duplicates this string for its own storage, so the first allocation is leaked)
                obj = NewClassValue(interpreter, CreateUserClass(AllocateString(str), NULL));
                Push(obj);
                Forward(strlen(str) + 1);
                free(str);
                break;
            }
            case OP_CLASS_DEFINE_STATIC_MEMBER: 
            case OP_CLASS_DEFINE_INSTANCE_MEMBER: {
                key = Popp();
                val = Popp();
                obj = Peek();
                ClassDefineMember(CoerceToUserClass(obj), key, val, (opcode == OP_CLASS_DEFINE_STATIC_MEMBER));
                break;
            }
            case OP_SET_INDEX: {
                val = Popp();
                key = Popp();
                obj = Peek();
                res = DoSetIndex(interpreter, obj, key, val);
                if (ValueIsError(res)) RaiseError(res);
                break;
            }
            case OP_GET_INDEX: {
                key = Popp();
                obj = Popp();
                res = DoGetIndex(interpreter, obj, key);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_LOAD_FUNCTION_CLOSURE:
            case OP_LOAD_FUNCTION: {
                offset = _ReadInt32(uf->Codes, ip);
                res    = DoLoadFunction(
                    interpreter, 
                    rootEnvObj, 
                    envObj, 
                    offset, 
                    (opcode == OP_LOAD_FUNCTION_CLOSURE)
                );
                Push(res);
                Forward(4);
                break;
            }
            case OP_CALL_CTOR: {
                argc = _ReadInt32(uf->Codes, ip);
                Forward(4);
                cls  = Popp();
                res  = DoCallCtor(interpreter, rootEnvObj, envObj, cls, argc);
                if (ValueIsError(res)) RaiseError(res);
                break;
            }
            case OP_CALL: {
                argc = _ReadInt32(uf->Codes, ip);
                Forward(4);
                obj  = Popp();
                res  = DoCall(interpreter, rootEnvObj, envObj, obj, argc);
                if (ValueIsError(res)) RaiseError(res);
                break;
            }
            case OP_CALL_METHOD: {
                argc = _ReadInt32(uf->Codes, ip);
                Forward(4);
                key  = Popp(); // method
                obj  = Popp(); // 'this' object
                res  = DoCallMethod(interpreter, rootEnvObj, envObj, obj, key, argc);
                if (ValueIsError(res)) RaiseError(res);
                break;
            }
            case OP_NOT: {
                rhs = Popp();
                res = DoNot(interpreter, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_POS: {
                rhs = Popp();
                res = DoPos(interpreter, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_NEG: {
                rhs = Popp();
                res = DoNeg(interpreter, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_MUL: {
                rhs = Popp();
                lhs = Popp();
                res = DoMul(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_DIV: {
                rhs = Popp();
                lhs = Popp();
                res =  DoDiv(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_MOD: {
                rhs = Popp();
                lhs = Popp();
                res = DoMod(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_POSTINC: {
                // bot [obj, key, val] top
                lhs = Popp(); // old value
                res = DoInc(interpreter, lhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                Push(lhs);
                break;
            }
            case OP_INC: {
                rhs = Popp();
                res = DoInc(interpreter, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_ADD: {
                rhs = Popp();
                lhs = Popp();
                res = DoAdd(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_POSTDEC: {
                // bot [obj, key, val] top
                lhs = Popp(); // old value
                res = DoDec(interpreter, lhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                Push(lhs);
                break;
            }
            case OP_DEC: {
                rhs = Popp();
                res = DoDec(interpreter, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_SUB: {
                rhs = Popp();
                lhs = Popp();
                res = DoSub(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_LSHFT: {
                rhs = Popp();
                lhs = Popp();
                res = DoLShift(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_RSHFT: {
                rhs = Popp();
                lhs = Popp();
                res = DoRShift(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_LT: {
                rhs = Popp();
                lhs = Popp();
                res = DoLT(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_LTE: {
                rhs = Popp();
                lhs = Popp();
                res = DoLTE(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_GT: {
                rhs = Popp();
                lhs = Popp();
                res = DoGT(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_GTE: {
                rhs = Popp();
                lhs = Popp();
                res = DoGTE(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_EQ: {
                rhs = Popp();
                lhs = Popp();
                res = DoEQ(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_NE: {
                rhs = Popp();
                lhs = Popp();
                res = DoNE(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_AND: {
                rhs = Popp();
                lhs = Popp();
                res = DoAnd(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_OR: {
                rhs = Popp();
                lhs = Popp();
                res = DoOr(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
                Push(res);
                break;
            }
            case OP_XOR: {
                rhs = Popp();
                lhs = Popp();
                res = DoXor(interpreter, lhs, rhs);
                if (ValueIsError(res)) RaiseError(res);
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
                if (!CoerceToBool(val)) {
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
                if (CoerceToBool(val)) {
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
                if (CoerceToBool(val) == false) {
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
    Run(interpreter, fnValue, envObj, envObj);
    if (interpreter->StackC != 1) {
        InterpreterPanic(
            "internal error: stack not cleaned up after function '%s' execution, expected 1 value on stack but got %d values", 
            uf->Name != NULL ? uf->Name : "<anonymous>", 
            interpreter->StackC
        );
    }
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

#undef Push
#undef Popp
#undef PopN
#undef Peek
#undef PeekAt
#undef PushEH
#undef PoppEH
#undef PopNEH
#undef PeekEH
#undef DumpFrame
#undef DumpStack
#undef SetVar
#undef GetVar
#undef InterpreterPanic
#undef HandleError
