#include "./value.h"

static Value* _CreateValue(Interpreter* interpreter, ValueType type) {
    Value* v  = Allocate(sizeof(Value));
    v->Type   = type;
    v->Marked = 0;
    v->Next   = NULL;
    interpreter->Allocated++;
    v->Next = interpreter->GcRoot;
    interpreter->GcRoot = v;
    return v;
}

Value* NewErrorValue(Interpreter* interpreter, String message) {
    Value* v = _CreateValue(interpreter, VLT_ERROR);
    v->Value.Opaque = StringToRunes(message);
    return v;
}

Value* NewErrorFValue(Interpreter* interpreter, String fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);
    String message = Allocate(size);
    va_start(args, fmt);
    vsnprintf(message, size, fmt, args);
    va_end(args);
    /***********/
    Value* v = _CreateValue(interpreter, VLT_ERROR);
    v->Value.Opaque = StringToRunes(message);
    free(message);
    return v;
}

Value* NewIntValue(Interpreter* interpreter, int value) {
    Value* v = _CreateValue(interpreter, VLT_INT);
    v->Value.I32 = value;
    return v;
}

Value* NewNumValue(Interpreter* interpreter, double value) {
    Value* v = _CreateValue(interpreter, VLT_NUM);
    v->Value.Num = value;
    return v;
}

Value* NewStrValue(Interpreter* interpreter, String value) {
    Value* v = _CreateValue(interpreter, VLT_STR);
    v->Value.Opaque = StringToRunes(value);
    return v;
}

Value* NewBoolValue(Interpreter* interpreter, int value) {
    Value* v = _CreateValue(interpreter, VLT_BOOL);
    v->Value.I32 = value ? 1 : 0;
    return v;
}

Value* NewNullValue(Interpreter* interpreter) {
    Value* v = _CreateValue(interpreter, VLT_NULL);
    v->Value.Opaque = NULL;
    return v;
}

Value* NewUserFunctionValue(Interpreter* interpreter, UserFunction* userFunction) {
    Value* v = _CreateValue(interpreter, VLT_USER_FUNCTION);
    v->Value.Opaque = userFunction;
    return v;
}

Value* NewNativeFunctionValue(Interpreter* interpreter, NativeFunction* nativeFunctionMeta) {
    Value* v = _CreateValue(interpreter, VLT_NATV_FUNCTION);
    v->Value.Opaque = nativeFunctionMeta;
    return v;
}

Value* NewEnvironmentValue(Interpreter* interpreter, Environment* environment) {
    Value* v = _CreateValue(interpreter, VLT_ENVIRONMENT);
    v->Value.Opaque = environment;
    return v;
}

Value* NewArrayValue(Interpreter* interpreter) {
    Value* v = _CreateValue(interpreter, VLT_ARRAY);
    v->Value.Opaque = CreateArray();
    return v;
}

Value* NewObjectValue(Interpreter* interpreter) {
    Value* v = _CreateValue(interpreter, VLT_OBJECT);
    v->Value.Opaque = CreateHashMap(16);
    return v;
}

Value* NewClassValue(Interpreter* interpreter, Class* cls) {
    Value* v = _CreateValue(interpreter, VLT_CLASS);
    v->Value.Opaque = cls;
    return v;
}

Value* NewClassInstanceValue(Interpreter* interpreter, ClassInstance* instance) {
    Value* v = _CreateValue(interpreter, VLT_CLASS_INSTANCE);
    v->Value.Opaque = instance;
    return v;
}

String ValueToString(Value* value) {
    char* buffer;
    switch (value->Type) {
        case VLT_INT:
            buffer = Allocate(32);
            snprintf(buffer, 32, "%d", value->Value.I32);
            return buffer;
        case VLT_NUM:
            buffer = Allocate(64);
            // Check if the number can be represented as an integer
            double num = value->Value.Num;
            if (floor(num) == num && num >= INT_MIN && num <= INT_MAX) {
                // It's a whole number that fits in an int
                snprintf(buffer, 64, "%d", (int)num);
            } else if (floor(num) == num && num >= LONG_MIN && num <= LONG_MAX) {
                // It's a whole number that fits in a long
                snprintf(buffer, 64, "%ld", (long)num);
            } else {
                // It's a fractional number, use %.15g for better precision
                snprintf(buffer, 64, "%.15g", num);
            }
            return buffer;
        case VLT_ERROR:
        case VLT_STR: {
            Rune* runes = (Rune*) value->Value.Opaque;
            // Convert runes back to UTF-8 string
            size_t runeCount = 0;
            while (runes[runeCount] != 0) {
                runeCount++;
            }
            
            // Estimate buffer size (max 4 bytes per rune + null terminator)
            size_t bufferSize = runeCount * 4 + 1;
            buffer = Allocate(bufferSize);
            
            size_t pos = 0;
            for (size_t i = 0; i < runeCount; i++) {
                unsigned char utf8Buffer[5];
                int utf8Size = utf_encode_char(runes[i], utf8Buffer);
                for (int j = 0; j < utf8Size; j++) {
                    buffer[pos++] = utf8Buffer[j];
                }
            }
            buffer[pos] = '\0';
            return buffer;
        }
        case VLT_BOOL:
            return AllocateString(value->Value.I32 ? "true" : "false");
        case VLT_NULL:
            return AllocateString("null");
        case VLT_USER_FUNCTION:
            return UserFunctionToString(CoerceToUserFunction(value));
        case VLT_ENVIRONMENT:
            return AllocateString("environment");
        case VLT_ARRAY:
            return ArrayToString(CoerceToArray(value));
        case VLT_OBJECT:
            return HashMapToString(CoerceToHashMap(value));
        case VLT_CLASS:
            return AllocateString("class");
        case VLT_CLASS_INSTANCE:
            return ClassInstanceToString(CoerceToClassInstance(value));
        case VLT_NATV_FUNCTION:
            return NativeFunctionMetaToString(CoerceToNativeFunctionMeta(value));
        default:
            return AllocateString("unknown");
    }
}

String ValueTypeOf(Value* value) {
    switch (value->Type) {
        case VLT_ERROR:
            return "error";
        case VLT_INT:
            return "int";
        case VLT_NUM:
            return "num";
        case VLT_STR:
            return "str";
        case VLT_BOOL:
            return "bool";
        case VLT_NULL:
            return "null";
        case VLT_USER_FUNCTION:
            return "function";
        case VLT_NATV_FUNCTION:
            return "native function";
        case VLT_ENVIRONMENT:
            return "environment";
        case VLT_ARRAY:
            return "array";
        case VLT_OBJECT:
            return "object";
        case VLT_CLASS:
            return "class";
        case VLT_CLASS_INSTANCE:
            return "class instance";
        default:
            return "unknown";
    }
}

bool ValueIsInt(Value* value) {
    return value->Type == VLT_INT;
}

bool ValueIsNum(Value* value) {
    return value->Type == VLT_NUM || value->Type == VLT_INT;
}

bool ValueIsStr(Value* value) {
    return value->Type == VLT_STR;
}

bool ValueIsBool(Value* value) {
    return value->Type == VLT_BOOL;
}

bool ValueIsNull(Value* value) {
    return value->Type == VLT_NULL;
}

bool ValueIsError(Value* value) {
    return value->Type == VLT_ERROR;
}

bool ValueIsUserFunction(Value* value) {
    return value->Type == VLT_USER_FUNCTION;
}

bool ValueIsNativeFunction(Value* value) {
    return value->Type == VLT_NATV_FUNCTION;
}

bool ValueIsCallable(Value* value) {
    return ValueIsUserFunction(value) || ValueIsNativeFunction(value);
}

bool ValueIsArray(Value* value) {
    return value->Type == VLT_ARRAY;
}

bool ValueIsObject(Value* value) {
    return value->Type == VLT_OBJECT;
}

bool ValueIsClass(Value* value) {
    return value->Type == VLT_CLASS;
}

bool ValueIsClassInstance(Value* value) {
    return value->Type == VLT_CLASS_INSTANCE;
}

bool ValueIsEqual(Value* a, Value* b) {
    if (a == b) return true;
    else if (ValueIsNum(a) && ValueIsNum(b)) {
        return CoerceToI64(a) == CoerceToI64(b);
    } else if (ValueIsStr(a) && ValueIsStr(b)) {
        Rune* lhsRunes = (Rune*) a->Value.Opaque;
        Rune* rhsRunes = (Rune*) b->Value.Opaque;
        // Compare rune by rune
        int i = 0;
        int equal = 1;
        while (lhsRunes[i] != 0 || rhsRunes[i] != 0) {
            if (lhsRunes[i] != rhsRunes[i]) {
                equal = 0;
                break;
            }
            i++;
        }
        return equal;
    } else if (ValueIsBool(a) && ValueIsBool(b)) {
        return CoerceToBool(a) == CoerceToBool(b);
    } else if (ValueIsNull(a) && ValueIsNull(b)) {
        return true;
    } else {
        return false;
    }
}