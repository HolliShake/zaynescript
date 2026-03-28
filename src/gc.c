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
                HashMap* hashMap = CoerceToHashMap(value);
                if (hashMap != NULL) {
                    FreeHashMap(hashMap);
                }
                value->Value.Opaque = NULL;
            }
            break;
        case VLT_CLASS: {
            Class* classObj = CoerceToUserClass(value);
            if (classObj != NULL) {
                if (classObj->StaticMembers != NULL) {
                    FreeHashMap(classObj->StaticMembers);
                    classObj->StaticMembers = NULL;
                }
                if (classObj->InstanceMembers != NULL) {
                    FreeHashMap(classObj->InstanceMembers);
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
                    FreeHashMap(instance->Members);
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
                for (int i = 0; i < sm->StackTop; i++) {
                    if (sm->Stacks != NULL) Mark(sm->Stacks[i]);
                }
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
        Mark(interpreter->Constants[i]);
    }
}

static void _MarkFunctions(Interpreter* interpreter) {
    for (int i = 0; i < interpreter->FunctionC; i++) {
        Mark(interpreter->Functions[i]);
    }
}

static void _MarkStack(Interpreter* interpreter) {
    for (int i = 0; i < interpreter->StackC; i++) {
        Mark(interpreter->Stacks[i]);
    }
}

static void _MarkEnvs(Interpreter* interpreter) {
    for (int i = 0; i < interpreter->EnvC; i++) {
        Mark(interpreter->Envs[i]);
    }
}

static void _MarkTaskQueue(Interpreter* interpreter) {
    for (int i = 0; i < interpreter->TaskQueueC; i++) {
        int idx = (interpreter->TaskQueueHead + i) % STACK_SIZE;
        Mark(interpreter->TaskQueue[idx]);
    }
}

static void _MarkCallStack(Interpreter* interpreter) {
    for (int i = 0; i < interpreter->CallStackC; i++) {
        Mark(interpreter->CallStack[i]);
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
    Mark(interpreter->GcRoot);
    Mark(interpreter->Array);
    Mark(interpreter->Date);
    Mark(interpreter->Promise);
    Mark(interpreter->True);
    Mark(interpreter->False);
    Mark(interpreter->Null);
    Mark(interpreter->RootEnv);
    Mark(interpreter->CallEnv);
    if (interpreter->ActiveTask != NULL) Mark(interpreter->ActiveTask);
    _MarkConstants(interpreter);
    _MarkFunctions(interpreter);
    _MarkStack(interpreter);
    _MarkEnvs(interpreter);
    _MarkTaskQueue(interpreter);
    _MarkCallStack(interpreter);

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