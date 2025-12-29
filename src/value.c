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

Value* NewIntValue(Interpreter* interpreter, int value) {
    Value* v = _CreateValue(interpreter, VT_INT);
    v->Value.I32 = value;
    return v;
}

Value* NewNumValue(Interpreter* interpreter, double value) {
    Value* v = _CreateValue(interpreter, VT_NUM);
    v->Value.Num = value;
    return v;
}

Value* NewStrValue(Interpreter* interpreter, String value) {
    Value* v = _CreateValue(interpreter, VT_STR);
    v->Value.Opaque = StringToRunes(value);
    return v;
}

Value* NewBoolValue(Interpreter* interpreter, int value) {
    Value* v = _CreateValue(interpreter, VT_BOOL);
    v->Value.I32 = value ? 1 : 0;
    return v;
}

Value* NewNullValue(Interpreter* interpreter) {
    Value* v = _CreateValue(interpreter, VT_NULL);
    v->Value.Opaque = NULL;
    return v;
}

Value* NewUserFunctionValue(Interpreter* interpreter, UserFunction* userFunction) {
    Value* v = _CreateValue(interpreter, VT_USER_FUNCTION);
    v->Value.Opaque = userFunction;
    return v;
}

Value* NewNativeFunctionValue(Interpreter* interpreter, NativeFunction* nativeFunction) {
    Value* v = _CreateValue(interpreter, VT_NATV_FUNCTION);
    v->Value.Opaque = nativeFunction;
    return v;
}

Value* NewEnvironmentValue(Interpreter* interpreter, Environment* environment) {
    Value* v = _CreateValue(interpreter, VT_ENVIRONMENT);
    v->Value.Opaque = environment;
    return v;
}

Value* NewObjectValue(Interpreter* interpreter) {
    Value* v = _CreateValue(interpreter, VT_OBJECT);
    v->Value.Opaque = CreateHashMap(16);
    return v;
}

String ValueToString(Value* value) {
    char* buffer;
    switch (value->Type) {
        case VT_INT:
            buffer = Allocate(32);
            snprintf(buffer, 32, "%d", value->Value.I32);
            return buffer;
        case VT_NUM:
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
                // It's a fractional number, use %g to avoid trailing zeros
                snprintf(buffer, 64, "%g", num);
            }
            return buffer;
        case VT_STR: {
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
        case VT_BOOL:
            return AllocateString(value->Value.I32 ? "true" : "false");
        case VT_NULL:
            return AllocateString("null");
        case VT_USER_FUNCTION:
            return AllocateString("function");
        case VT_ENVIRONMENT:
            return AllocateString("environment");
        case VT_OBJECT:
            return HashMapToString((HashMap*) value->Value.Opaque);
        case VT_CLASS:
            return AllocateString("class");
        case VT_NATV_FUNCTION:
            return AllocateString("native function(){...}");
    }
    return AllocateString("unknown");
}

bool ValueToBool(Value* value) {
    switch (value->Type) {
        case VT_INT:
            return value->Value.I32 != 0;
        case VT_NUM:
            return value->Value.Num != 0.0;
        case VT_STR:
            return strlen(value->Value.Opaque) > 0;
        case VT_BOOL:
            return !!(value->Value.I32);
        case VT_NULL:
            return false;
    }
    printf("ValueToBool: Unknown value type: %d\n", value->Type);
    return false;
}

bool ValueIsInt(Value* value) {
    return value->Type == VT_INT;
}

bool ValueIsNum(Value* value) {
    return value->Type == VT_NUM || value->Type == VT_INT;
}

bool ValueIsStr(Value* value) {
    return value->Type == VT_STR;
}

bool ValueIsBool(Value* value) {
    return value->Type == VT_BOOL;
}

bool ValueIsNull(Value* value) {
    return value->Type == VT_NULL;
}

bool ValueIsUserFunction(Value* value) {
    return value->Type == VT_USER_FUNCTION;
}

bool ValueIsNativeFunction(Value* value) {
    return value->Type == VT_NATV_FUNCTION;
}

bool ValueIsCallable(Value* value) {
    return ValueIsUserFunction(value) || ValueIsNativeFunction(value);
}