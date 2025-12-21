#include "./gc.h"

extern String ValueToString(Value* value);

/**
 * @brief Frees a value and its associated memory
 * 
 * @param value The value to free
 */
static void _Free(Value* value) {
    switch (value->Type) {
        case VT_STR:
            if (value->Value.Opaque != NULL) {
                free(value->Value.Opaque);
            }
            break;
        case VT_OBJECT:
            if (value->Value.Opaque != NULL) {
                // Note: deeply freeing HashMap keys/values would require more logic
                // For now, we just free the HashMap struct itself
                free(value->Value.Opaque);
            }
            break;
        default:
            break;
    }
    free(value);
}

static void _Mark(Value* value) {
    if (value == NULL || value->Marked) {
        return;
    }
    value->Marked = 1;
}

static void _MarkConstants(Interpreter* interpreter) {
    for (int i = 0; i < interpreter->ConstantC; i++) {
        Value* constant = interpreter->Constants[i];
        if (constant != NULL) {
            _Mark(constant);
        }
    }
}

static void _Sweep(Interpreter* interpreter) {
    Value** current = &interpreter->GcRoot;
    while (*current != NULL) {
        Value* value = *current;
        if (!value->Marked) {
            printf("Freeing value: %s\n", ValueToString(value));
            Value* unreached = value;
            *current = unreached->Next;
            _Free(unreached);
        } else {
            value->Marked = 0;
            current = &value->Next;
        }
    }
}

void GarbageCollect(Interpreter* interpreter) {
    printf("Garbage collecting...\n");
    _Mark(interpreter->True);
    _Mark(interpreter->False);
    _Mark(interpreter->Null);
    _MarkConstants(interpreter);
    _Sweep(interpreter);
    int reset = interpreter->Allocated - GC_THRESHOLD;
    interpreter->Allocated = reset >= 0 ? reset : 0;
}
