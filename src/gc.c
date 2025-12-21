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
                value->Value.Opaque = NULL;
            }
            break;
        case VT_OBJECT:
            if (value->Value.Opaque != NULL) {
                // Note: deeply freeing HashMap keys/values would require more logic
                // For now, we just free the HashMap struct itself
                free(value->Value.Opaque);
                value->Value.Opaque = NULL;
            }
            break;
        default:
            break;
    }
    free(value);
}

void Mark(Value* value) {
    if (value == NULL || value->Marked) {
        return;
    }
    value->Marked = 1;
    switch (value->Type) {
        case VT_ENVIRONMENT: {
            Environment* env = (Environment*) value->Value.Opaque;
            if (env != NULL) {
                Mark(env->Parent);
                for (int i = 0; i < env->LocalC; i++) {
                    Mark(env->Locals[i]);
                }
            }
        }
    }
}

static void _MarkConstants(Interpreter* interpreter) {
    for (int i = 0; i < interpreter->ConstantC; i++) {
        Value* constant = interpreter->Constants[i];
        if (constant != NULL) {
            Mark(constant);
        }
    }
}

static void _MarkFunctions(Interpreter* interpreter) {
    for (int i = 0; i < interpreter->FunctionC; i++) {
        Value* function = interpreter->Functions[i];
        if (function != NULL) {
            Mark(function);
        }
    }
}

static void _MarkStack(Interpreter* interpreter) {
    for (int i = 0; i < interpreter->StackC; i++) {
        Value* value = interpreter->Stack[i];
        if (value != NULL) {
            Mark(value);
        }
    }
}

static void _Sweep(Interpreter* interpreter) {
    Value** current = &interpreter->GcRoot;
    while (*current != NULL) {
        Value* value = *current;
        if (!value->Marked) {
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
    Mark(interpreter->True);
    Mark(interpreter->False);
    Mark(interpreter->Null);
    _MarkConstants(interpreter);
    _MarkFunctions(interpreter);
    _MarkStack(interpreter);
    _Sweep(interpreter);
    int reset = interpreter->Allocated - GC_THRESHOLD;
    interpreter->Allocated = reset >= 0 ? reset : 0;
}

void ForceGarbageCollect(Interpreter* interpreter) {
    _Sweep(interpreter);
    interpreter->Allocated = 0;
}
