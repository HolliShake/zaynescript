#include "./gc.h"

#include "global.h"

/**
 * @brief Converts a Value to its string representation.
 * @param value The value to convert.
 * @return A newly allocated string (caller must free).
 * @origin src/value.c:150
 */
extern String ValueToString(Value* value);

/**
 * @brief Frees a value and its associated memory
 *
 * @param interp The interpreter instance (used for big-number
 * context cleanup)
 * @param value The value to free
 */
static void _Free(Interpreter* interp, Value* value) {
	switch (value->Type) {
		case VLT_BINT:
		case VLT_BNUM:
			{
				bf_t* bf = (bf_t*) value->Value.Opaque;
				if (bf != NULL) {
					bf_delete(bf);
					bf_free(&interp->BfContext, bf);
					value->Value.Opaque = NULL;
				}
				break;
			}
		case VLT_STR:
		case VLT_ERROR:
			if (value->Value.Opaque != NULL) {
				free(value->Value.Opaque);
				value->Value.Opaque = NULL;
			}
			break;
		case VLT_ARRAY:
			{
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
		case VLT_CLASS:
			{
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
		case VLT_CLASS_INSTANCE:
			{
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
		case VLT_ENVIRONMENT:
			{
				Environment* env = CoerceToEnvironment(value);
				FreeEnvironment(env);
				value->Value.Opaque = NULL;
				break;
			}
		case VLT_USER_FUNCTION:
			{
				UserFunction* uf = CoerceToUserFunction(value);
				FreeUserFunction(uf);
				value->Value.Opaque = NULL;
				break;
			}
		case VLT_NATV_FUNCTION:
			{
				NativeFunction* nf = CoerceToNativeFunction(value);
				FreeNativeFunction(nf);
				value->Value.Opaque = NULL;
				break;
			}
		case VLT_PROMISE:
			{
				StateMachine* sm = CoerceToStateMachine(value);
				FreeStateMachine(sm);
				value->Value.Opaque = NULL;
				break;
			}
		case VLT_BLOB:
			{
				Blob* blob = CoerceToBlob(value);
				if (blob != NULL) {
					if (blob->Data != NULL) {
						free(blob->Data);
						blob->Data = NULL;
					}
					free(blob);
					value->Value.Opaque = NULL;
				}
				break;
			}
		case VLT_OPAQUE:
			{
				//  Do not free!
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
		case VLT_ARRAY:
			{
				Array* array = CoerceToArray(value);
				if (array != NULL) {
					for (size_t i = 0; i < array->Count; i++) {
						Mark(array->Items[i]);
					}
				}
				break;
			}
		case VLT_OBJECT:
			{
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
		case VLT_CLASS:
			{
				Class* classObj = CoerceToUserClass(value);
				if (classObj != NULL) {
					Mark(classObj->Base);
					HashMap* staticMembers	 = classObj->StaticMembers;
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
		case VLT_CLASS_INSTANCE:
			{
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
		case VLT_ENVIRONMENT:
			{
				Environment* env = CoerceToEnvironment(value);
				if (env != NULL) {
					Mark(env->Parent);
					for (int i = 0; i < env->LocalC; i++) {
						if (env->Locals[i] != NULL
							&& env->Locals[i]->Value != NULL) {
							Mark(env->Locals[i]->Value);
						}
					}
				}
				break;
			}
		case VLT_USER_FUNCTION:
			{
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
		case VLT_PROMISE:
			{
				StateMachine* sm = CoerceToStateMachine(value);
				if (sm != NULL) {
					Mark(sm->CallEnv);
					Mark(sm->WaitFor);
					Mark(sm->Value);
					Mark(sm->Function);
					if (sm->EnvStack != NULL)
						for (int i = 0; i < sm->EnvrTop; i++) {
							Mark(sm->EnvStack[i]);
						}
					if (sm->Stacks != NULL)
						for (int i = 0; i < sm->StckTop; i++) {
							Mark(sm->Stacks[i]);
						}
					for (int i = 0; i < sm->WaitListC; i++) {
						Mark(sm->WaitList[i]);
					}
				}
				break;
			}
		case VLT_OPAQUE:
			{
				// logic here!
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
	for (int i = 0; i < interpreter->StckC; i++) {
		Mark(interpreter->Stacks[i]);
	}
}

static void _MarkEnvs(Interpreter* interpreter) {
	for (int i = 0; i < interpreter->EnvrC; i++) {
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
		Mark(interpreter->CallStack[i].Function);
	}
}

/* ── shared mark phase ──────────────────────────────────────────────────── */

static void _MarkAll(Interpreter* interpreter) {
	Mark(interpreter->GcRoot);
	Mark(interpreter->OldRoot);
	Mark(interpreter->Object);
	Mark(interpreter->Array);
	Mark(interpreter->Date);
	Mark(interpreter->Promise);
	Mark(interpreter->True);
	Mark(interpreter->False);
	Mark(interpreter->Null);
	Mark(interpreter->RootEnv);
	Mark(interpreter->CallEnv);
	Mark(interpreter->ActiveFunction);
	Mark(interpreter->ActiveTask);
	Mark(interpreter->Error);
	_MarkConstants(interpreter);
	_MarkFunctions(interpreter);
	_MarkStack(interpreter);
	_MarkEnvs(interpreter);
	_MarkTaskQueue(interpreter);
	_MarkCallStack(interpreter);
}

/* ── minor sweep: collect young gen, promote survivors to old gen ───────── */

static void _MinorSweep(Interpreter* interpreter) {
	Value* curr = interpreter->GcRoot;
	while (curr != NULL) {
		Value* next = curr->Next;
		if (!curr->Marked) {
			_Free(interpreter, curr);
		} else {
			curr->Marked		 = 0;
			curr->Generation	 = 1;
			curr->Next			 = interpreter->OldRoot;
			interpreter->OldRoot = curr;
			interpreter->OldCount++;
		}
		curr = next;
	}
	interpreter->GcRoot	   = NULL;
	interpreter->Allocated = 0;
}

/* ── reset marks on old gen after a minor GC (they were marked but kept) ── */

static void _UnmarkOld(Interpreter* interpreter) {
	Value* v = interpreter->OldRoot;
	while (v != NULL) {
		v->Marked = 0;
		v		  = v->Next;
	}
}

/* ── major sweep: collect both generations, promote young survivors ──────── */

static size_t _MajorSweep(Interpreter* interpreter) {
	/* First pass: sweep young gen.  Survivors keep Marked=1 so the old
	   sweep below will count and unmark them correctly. */
	Value* curr = interpreter->GcRoot;
	while (curr != NULL) {
		Value* next = curr->Next;
		if (!curr->Marked) {
			_Free(interpreter, curr);
		} else {
			curr->Generation	 = 1;
			curr->Next			 = interpreter->OldRoot;
			interpreter->OldRoot = curr;
		}
		curr = next;
	}
	interpreter->GcRoot	   = NULL;
	interpreter->Allocated = 0;

	/* Second pass: sweep old gen (now includes just-promoted values). */
	size_t	survivors = 0;
	Value** current	  = &interpreter->OldRoot;
	while (*current != NULL) {
		Value* value = *current;
		if (!value->Marked) {
			Value* unreached = value;
			*current		 = unreached->Next;
			_Free(interpreter, unreached);
		} else {
			++survivors;
			value->Marked = 0;
			current		  = &value->Next;
		}
	}
	interpreter->OldCount = survivors;
	return survivors;
}

/* ── public GC API ──────────────────────────────────────────────────────── */

void GarbageCollect(Interpreter* interpreter) {
	_MarkAll(interpreter);
	_MinorSweep(interpreter);
	_UnmarkOld(interpreter);
	interpreter->GcThreshold = GC_YOUNG_THRESHOLD;
}

void MajorGarbageCollect(Interpreter* interpreter) {
	_MarkAll(interpreter);

	size_t srv				  = _MajorSweep(interpreter);
	size_t nxt				  = srv * GC_GROWTH_FACTOR;
	interpreter->OldThreshold = (nxt < GC_THRESHOLD) ? GC_THRESHOLD : nxt;
}

void ForceGarbageCollect(Interpreter* interpreter) {
	_MarkAll(interpreter);
	_MajorSweep(interpreter);
	interpreter->GcThreshold  = GC_YOUNG_THRESHOLD;
	interpreter->OldThreshold = GC_THRESHOLD;
}

void GcDestroyHeap(Interpreter* interpreter) {
	Value* young		 = interpreter->GcRoot;
	Value* old			 = interpreter->OldRoot;
	interpreter->GcRoot	 = NULL;
	interpreter->OldRoot = NULL;

	interpreter->RootEnv				= NULL;
	interpreter->CallEnv				= NULL;
	interpreter->StckC					= 0;
	interpreter->EnvrC					= 0;
	interpreter->TaskQueueHead			= 0;
	interpreter->TaskQueueC				= 0;
	interpreter->CallStackC				= 0;
	interpreter->ActiveFunction			= NULL;
	interpreter->ActiveTask				= NULL;
	interpreter->Error					= NULL;
	interpreter->ExceptionHandlerStackC = 0;
	interpreter->Object					= NULL;
	interpreter->Array					= NULL;
	interpreter->Date					= NULL;
	interpreter->Promise				= NULL;
	interpreter->True					= NULL;
	interpreter->False					= NULL;
	interpreter->Null					= NULL;
	for (int i = 0; i < interpreter->ConstantC; i++) {
		interpreter->Constants[i] = NULL;
	}
	for (int i = 0; i < interpreter->FunctionC; i++) {
		interpreter->Functions[i] = NULL;
	}
	interpreter->ConstantC = 0;
	interpreter->FunctionC = 0;
	interpreter->Allocated = 0;
	interpreter->OldCount  = 0;

	while (young != NULL) {
		Value* next = young->Next;
		young->Next = NULL;
		_Free(interpreter, young);
		young = next;
	}
	while (old != NULL) {
		Value* next = old->Next;
		old->Next	= NULL;
		_Free(interpreter, old);
		old = next;
	}
}