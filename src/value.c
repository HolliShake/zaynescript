#include "./value.h"


static Value* _CreateValue(Interpreter* interpreter, ValueType type) {
    Value* v = Allocate(sizeof(Value));
    v->Type = type;
    // GC
    if (interpreter->Allocated >= GC_THRESHOLD) {
        GarbageCollect(interpreter);
    }
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

Value* NewObjectValue(Interpreter* interpreter, String zKey[], Value* zVal[], int elementCount) {
    Value* v = _CreateValue(interpreter, VT_OBJECT);
    v->Value.Opaque = CreateHashMap(elementCount);
    for (int i = 0; i < elementCount; i++) {
        HashMapSet((HashMap*) v->Value.Opaque, zKey[i], zVal[i]);
    }
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
            return value->Value.I32 ? "true" : "false";
        case VT_NULL:
            return "null";
    }
    return "unknown";
}

int ValueIsInt(Value* value) {
    return value->Type == VT_INT;
}

int ValueIsNum(Value* value) {
    return value->Type == VT_NUM || value->Type == VT_INT;
}

int ValueIsStr(Value* value) {
    return value->Type == VT_STR;
}

int ValueIsBool(Value* value) {
    return value->Type == VT_BOOL;
}

int ValueIsNull(Value* value) {
    return value->Type == VT_NULL;
}