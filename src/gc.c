#include "./gc.h"

extern String ValueToString(Value* value);

/**
 * @brief Frees a value and its associated memory
 * 
 * @param value The value to free
 */
static void _Free(Interpreter* interp, Value* value) {
    switch (value->Type) {
        case VLT_BINT:
        case VLT_BNUM: {
            bf_t* bf = (bf_t*) value->Value.Opaque;
            if (bf != NULL) {
                bf_delete(bf);
                bf_free(&interp->BfContext, bf);
                value->Value.Opaque = NULL;
            }
            break;
        }
        case VLT_STR:
            if (value->Value.Opaque != NULL) {
                free(value->Value.Opaque);
                value->Value.Opaque = NULL;
            }
            break;
        case VLT_ARRAY: {
            Array* array = CoerceToArray(value);
            if (array != NULL) {
                if (array->Items != NULL) {
                    free(array->Items);
                    array->Items = NULL;
                }
                free(array);
                value->Value.Opaque = NULL;
            }
            break;
        }
        case VLT_OBJECT:
            if (value->Value.Opaque != NULL) {
                // Note: deeply freeing HashMap keys/values would require more logic
                // For now, we just free the HashMap struct itself
                HashMap* hashMap = CoerceToHashMap(value);
                if (hashMap != NULL) {
                    // Buckets
                    for (size_t i = 0; i < hashMap->Size; i++) {
                        HashNode* node = &hashMap->Buckets[i];
                        // First node is in the array, only free key
                        if (node->Key != NULL) {
                            free(node->Key);
                            node->Key = NULL;
                        }

                        // Subsequent nodes are malloc'd
                        HashNode* current = node->Next;
                        while (current != NULL) {
                            HashNode* next = current->Next;
                            if (current->Key != NULL) {
                                free(current->Key);
                            }
                            free(current);
                            current = next;
                        }
                    }
                    free(hashMap->Buckets);
                }
                free(value->Value.Opaque);
                value->Value.Opaque = NULL;
            }
            break;
        case VLT_CLASS: {
            Class* classObj = CoerceToUserClass(value);
            if (classObj != NULL) {
                if (classObj->StaticMembers != NULL) {
                    free(classObj->StaticMembers->Buckets);
                    free(classObj->StaticMembers);
                    classObj->StaticMembers = NULL;
                }
                if (classObj->InstanceMembers != NULL) {
                    free(classObj->InstanceMembers->Buckets);
                    free(classObj->InstanceMembers);
                    classObj->InstanceMembers = NULL;
                }
                free(classObj->Name);
                free(classObj);
                value->Value.Opaque = NULL;
            }
            break;
        }
        case VLT_CLASS_INSTANCE: {
            ClassInstance* instance = CoerceToClassInstance(value);
            if (instance != NULL) {
                if (instance->Members != NULL) {
                    free(instance->Members->Buckets);
                    free(instance->Members);
                    instance->Members = NULL;
                }
                free(instance);
                value->Value.Opaque = NULL;
            }
            break;
        }
        case VLT_ENVIRONMENT: {
            Environment* env = CoerceToEnvironment(value);
            FreeEnvironment(env);
            value->Value.Opaque = NULL;
            break;
        }
        case VLT_USER_FUNCTION: {
            UserFunction* uf = CoerceToUserFunction(value);
            FreeUserFunction(uf);
            value->Value.Opaque = NULL;
            break;
        }
        case VLT_NATV_FUNCTION: {
            NativeFunction* nf = CoerceToNativeFunction(value);
            FreeNativeFunction(nf);
            value->Value.Opaque = NULL;
            break;
        }
        case VLT_PROMISE: {
            StateMachine* sm = CoerceToStateMachine(value);
            FreeStateMachine(sm);
            value->Value.Opaque = NULL;
            break;
        }
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
        case VLT_ARRAY: {
            Array* array = CoerceToArray(value);
            if (array != NULL) {
                for (size_t i = 0; i < array->Count; i++) {
                    Mark(array->Items[i]);
                }
            }
            break;
        }
        case VLT_OBJECT: {
            HashMap* hashMap = CoerceToHashMap(value);
            if (hashMap != NULL) {
                for (size_t i = 0; i < hashMap->Size; i++) {
                    HashNode* node = &hashMap->Buckets[i];
                    while (node != NULL) {
                        Mark(node->Val);
                        node = node->Next;
                    }
                }
            }
            break;
        }
        case VLT_CLASS: {
            Class* classObj = CoerceToUserClass(value);
            if (classObj != NULL) {
                Mark(classObj->Base);
                HashMap* staticMembers   = classObj->StaticMembers;
                HashMap* instanceMembers = classObj->InstanceMembers;
                if (staticMembers != NULL) {
                    for (size_t i = 0; i < staticMembers->Size; i++) {
                        HashNode* node = &staticMembers->Buckets[i];
                        while (node != NULL) {
                            Mark(node->Val);
                            node = node->Next;
                        }
                    }
                }
                if (instanceMembers != NULL) {
                    for (size_t i = 0; i < instanceMembers->Size; i++) {
                        HashNode* node = &instanceMembers->Buckets[i];
                        while (node != NULL) {
                            Mark(node->Val);
                            node = node->Next;
                        }
                    }
                }
            }
            break;
        }
        case VLT_CLASS_INSTANCE: {
            ClassInstance* instance = CoerceToClassInstance(value);
            if (instance != NULL) {
                Mark(instance->Proto);
                HashMap* members = instance->Members;
                if (members != NULL) {
                    for (size_t i = 0; i < members->Size; i++) {
                        HashNode* node = &members->Buckets[i];
                        while (node != NULL) {
                            Mark(node->Val);
                            node = node->Next;
                        }
                    }
                }
            }
            break;
        }
        case VLT_ENVIRONMENT: {
            Environment* env = CoerceToEnvironment(value);
            if (env != NULL) {
                Mark(env->Parent);
                for (int i = 0; i < env->LocalC; i++) {
                    if(env->Locals[i] != NULL && env->Locals[i]->Value != NULL) {
                        Mark(env->Locals[i]->Value);
                    }
                }
            }
            break;
        }
        case VLT_USER_FUNCTION: {
            UserFunction* uf = CoerceToUserFunction(value);
            if (uf != NULL) {
                Mark(uf->Scope);
                for (int i = 0; i < uf->CaptureC; i++) {
                    EnvCell* cell = uf->Captures[i];
                    if (cell != NULL && cell->Value != NULL) {
                        Mark(cell->Value);
                    }
                }
            }
            break;
        }
        case VLT_PROMISE: {
            StateMachine* sm = CoerceToStateMachine(value);
            if (sm != NULL) {
                Mark(sm->CallEnv);
                Mark(sm->WaitFor);
                Mark(sm->Value);
                Mark(sm->Function);
                for (int i = 0; i < sm->WaitListC; i++) {
                    Mark(sm->WaitList[i]);
                }
            }
            break;
        }
        default:
            break;
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
        Value* value = interpreter->Stacks[i];
        if (value != NULL) {
            Mark(value);
        }
    }
}

static void _MarkEnvs(Interpreter* interpreter) {
    for (int i = 0; i < interpreter->EnvC; i++) {
        Value* envObj = interpreter->Envs[i];
        if (envObj != NULL) {
            Mark(envObj);
        }
    }
}

static size_t _Sweep(Interpreter* interpreter) {
    size_t survivors = 0;
    Value** current = &interpreter->GcRoot;
    while (*current != NULL) {
        Value* value = *current;
        if (!value->Marked) {
            Value* unreached = value;
            *current = unreached->Next;
            _Free(interpreter, unreached);
        } else {
            ++survivors;
            value->Marked = 0;
            current = &value->Next;
        }
    }
    return survivors;
}

void GarbageCollect(Interpreter* interpreter) {
    // printf("GC: Starting garbage collection... Allocated = %d bytes, Threshold = %d bytes\n", interpreter->Allocated, interpreter->GcThreshold);
    Mark(interpreter->Array);
    Mark(interpreter->True);
    Mark(interpreter->False);
    Mark(interpreter->Null);
    Mark(interpreter->RootEnv);
    Mark(interpreter->CallEnv);
    _MarkConstants(interpreter);
    _MarkFunctions(interpreter);
    _MarkStack(interpreter);
    _MarkEnvs(interpreter);
    size_t srv = _Sweep(interpreter);
    size_t nxt = srv * GC_GROWTH_FACTOR;

    if (nxt < GC_THRESHOLD) {
        interpreter->GcThreshold = GC_THRESHOLD;
    } else {
        interpreter->GcThreshold = (int) nxt;
    }

    interpreter->Allocated = srv;
}

void ForceGarbageCollect(Interpreter* interpreter) {
    _Sweep(interpreter);
    interpreter->Allocated = 0;
}