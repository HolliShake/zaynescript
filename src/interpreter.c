#include "./interpreter.h"

#include "global.h"
#include "statemachine.h"

static void* interpreter_bf_realloc(void* opaque, void* ptr, size_t size) {
	// libbf uses size == 0 to signal a free() operation
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	// If ptr is NULL, realloc behaves exactly like malloc
	return realloc(ptr, size);
}

Interpreter* CreateInterpreter(String execPath) {
	Interpreter* interpreter = Allocate(sizeof(Interpreter));
	bf_context_init(&(interpreter->BfContext), interpreter_bf_realloc, NULL);
	interpreter->ExecPath	   = AllocateString(execPath);
	interpreter->ModulePath	   = NULL;
	interpreter->ArgString	   = NULL;
	interpreter->ImportHead	   = NULL;
	interpreter->Imports	   = CreateHashMap(16);
	interpreter->Allocated	   = 0;
	interpreter->GcThreshold   = GC_THRESHOLD;
	interpreter->GcRoot		   = NULL;
	interpreter->Object		   = CreateObjectClass(interpreter);   // Singleton
	interpreter->Array		   = CreateArrayClass(interpreter);	   // Singleton
	interpreter->Date		   = CreateDateClass(interpreter);	   // Singleton
	interpreter->Promise	   = CreatePromiseClass(interpreter);  // Singleton
	interpreter->True		   = NewBoolValue(interpreter, 1);	   // Singletons
	interpreter->False		   = NewBoolValue(interpreter, 0);	   // Singletons
	interpreter->Null		   = NewNullValue(interpreter);		   // Singletons
	interpreter->Constants	   = Allocate(sizeof(Value*));
	interpreter->ConstantC	   = 0;
	interpreter->Constants[0]  = NULL;
	interpreter->Functions	   = Allocate(sizeof(Value*));
	interpreter->FunctionC	   = 0;
	interpreter->Functions[0]  = NULL;
	interpreter->TaskQueueHead = 0;
	interpreter->TaskQueueC	   = 0;
	mg_mgr_init(&interpreter->MgMgr);
	return interpreter;
}

#define SetVar(envObj, off, value)                                                    \
	EnvironmentSetLocal(CoerceToEnvironment(envObj), off, value)

#define GetVar(envObj, off)                                                           \
	EnvironmentGetLocal(CoerceToEnvironment(envObj), off)->Value

#define GetCap(uFunct, off) UserFunctionGetCapture(uFunct, off)

#define SetCap(uFunct, off, value) UserFunctionSetCapture(uFunct, off, value)

#define LockVar(envObj, off)                                                          \
	{                                                                                 \
		Environment* env  = CoerceToEnvironment(envObj);                              \
		EnvCell*	 cell = EnvironmentGetLocal(env, off);                            \
		if (cell->RefCount > 0 && cell->IsCaptured) {                                 \
			cell->RefCount--;                                                         \
			env->Locals[off] = CreateEnvCell(cell->Value);                            \
		}                                                                             \
	}


#define InterpreterPanic(message, ...)                                                \
	do {                                                                              \
		fprintf(stderr, "[%s:%d]::Panic: ", __FILE__, __LINE__);                      \
		fprintf(stderr, message, ##__VA_ARGS__);                                      \
		fprintf(stderr, "\n");                                                        \
		ForceGarbageCollect(interpreter);                                             \
		FreeInterpreter(interpreter);                                                 \
		fprintf(stderr, "Program exited with panic.\n");                              \
		exit(EXIT_FAILURE);                                                           \
	} while (0)

void FPush(Interpreter* interpreter, CallFrame* frame, Value* value) {
	frame->Operand[frame->OperandC++] = value;
}

Value* FPopp(Interpreter* interpreter, CallFrame* frame) {
	return frame->Operand[--frame->OperandC];
}

void FPopN(Interpreter* interpreter, CallFrame* frame, int n) {
	frame->OperandC -= n;
	// frame->Operand[frame->OperandC];
}

Value* FPeek(Interpreter* interpreter, CallFrame* frame) {
	return frame->Operand[frame->OperandC - 1];
}

Value* FPeekAt(Interpreter* interpreter, CallFrame* frame, int n) {
	return frame->Operand[(frame->OperandC) - (n)];
}

bool HasPendingTasks(Interpreter* interpreter) {
	return interpreter->TaskQueueC > 0;
}

void EnqueueTask(Interpreter* interpreter, Value* task) {
	if (interpreter->TaskQueueC >= STACK_SIZE) {
		InterpreterPanic("Task queue overflow");
	}
	int tail = (interpreter->TaskQueueHead + interpreter->TaskQueueC) % STACK_SIZE;
	interpreter->TaskQueue[tail] = task;
	interpreter->TaskQueueC++;
}

Value* DequeueTask(Interpreter* interpreter) {
	if (interpreter->TaskQueueC == 0) {
		return NULL;
	}
	Value* task				   = interpreter->TaskQueue[interpreter->TaskQueueHead];
	interpreter->TaskQueueHead = (interpreter->TaskQueueHead + 1) % STACK_SIZE;
	interpreter->TaskQueueC--;
	return task;
}

Value* DequeueTaskAt(Interpreter* interpreter, int index) {
	if (interpreter->TaskQueueC == 0 || index < 0
		|| index >= interpreter->TaskQueueC) {
		return NULL;
	}
	int	   phys = (interpreter->TaskQueueHead + index) % STACK_SIZE;
	Value* task = interpreter->TaskQueue[phys];
	// Shift all logical elements after 'index' one slot toward
	// the head
	for (int i = index; i < interpreter->TaskQueueC - 1; i++) {
		int cur	 = (interpreter->TaskQueueHead + i) % STACK_SIZE;
		int next = (interpreter->TaskQueueHead + i + 1) % STACK_SIZE;
		interpreter->TaskQueue[cur] = interpreter->TaskQueue[next];
	}
	interpreter->TaskQueueC--;
	return task;
}

String ReadString(uint8_t* codes, int alignStart) {
	String str	  = (String) (codes + alignStart);
	int	   length = strlen(str);
	String new	  = Allocate(length + 1);
	memcpy(new, str, length + 1);
	return new;
}

int ReadInt32(uint8_t* codes, int alignStart) {
	int off	 = 0;
	off		|= codes[alignStart + 0] << 24;
	off		|= codes[alignStart + 1] << 16;
	off		|= codes[alignStart + 2] << 8;
	off		|= codes[alignStart + 3] << 0;
	return off;
}

static int _GetArgc(Value* fn) {
	if (ValueIsClass(fn)) {
		Class* cls = CoerceToUserClass(fn);
		if (ClassHasMember(cls, CONSTRUCTOR_NAME, false, true)) {
			Value* constructor = ClassGetMember(cls, CONSTRUCTOR_NAME, false);
			return _GetArgc(constructor);
		} else {
			return 0;
		}
	} else if (ValueIsNativeFunction(fn)) {
		NativeFunction* nFMeta = CoerceToNativeFunction(fn);
		return nFMeta->Argc;
	} else if (ValueIsUserFunction(fn)) {
		UserFunction* uf = CoerceToUserFunction(fn);
		return uf->Argc;
	}
	return 0;
}

static int _GetArg2(Interpreter* interp, Value* obj, Value* methodName) {
	Value* method = GenericGetAttribute(interp, obj, methodName, true);
	if (ValueIsNull(method)) {
		return 0;
	}
	return _GetArgc(method);
}

#define JumpToError(ip, addr) (ip = addr)

static LineInfo _GetLineFromPc(UserFunction* uf, size_t pc) {
	if (uf->LineC == 0) {
		goto BAD;
	}

	int low	 = 0;
	int high = uf->LineC - 1;

	while (low <= high) {
		int mid = low + (high - low) / 2;
		if (uf->Lines[mid].Pc == pc) {
			return uf->Lines[mid];
		} else if (uf->Lines[mid].Pc < pc) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	// If exact match not found, return the line for the closest
	// lower PC
	if (high >= 0) {
		return uf->Lines[high];
	}
BAD:;
	return (LineInfo){
		.Path = NULL,
		.Line = -1,
		.Pc	  = -1,
	};
}

static String
_DumpTraceback(Interpreter* interpreter, CallFrame* frame, String message) {
	UserFunction* uf   = CoerceToUserFunction(frame->Fn);
	LineInfo	  line = _GetLineFromPc(uf, frame->Ip);
	frame			   = frame->Parent;
	String traceBack = NULL, prev = NULL;
	String path = ValueToString(line.Path);
	traceBack	= FormatString("[%s:%d]::%s\n", path, line.Line, message);
	free(path);

	while (frame != NULL) {
		UserFunction* currentUf = CoerceToUserFunction(frame->Fn);
		line					= _GetLineFromPc(currentUf, frame->Ip);
		path					= ValueToString(line.Path);
		prev					= traceBack;
		traceBack = FormatString("%s  |> [%s:%d]\n", traceBack, path, line.Line);
		free(prev);
		free(path);
		frame = frame->Parent;
	}

	prev	  = traceBack;
	traceBack = FormatString("%s  ;\n", traceBack);
	free(prev);

	return traceBack;
}

static void _RaiseError(Interpreter* interpreter,
						CallFrame*	 frame,
						Value*		 continuePromise,
						Value*		 error,
						bool		 catchable) {
	if (!catchable) {
		goto unhandled;
	}

	// Walk up the frame chain looking for a try handler
	CallFrame* current = frame;
	while (current != NULL) {
		if (current->TryHandlerC > 0) {
			int catchAddress = PeekTry(current);
			PoppTry(current);  // FIX: Pop from 'current', not 'frame'

			// Unwind operand stack of the catching frame;
			FPush(interpreter, current, error);
			current->Ip = catchAddress;

			// Suspend every frame between 'frame' and 'current'
			// so their Run() loops exit cleanly back to the catching frame
			CallFrame* unwind = frame;
			while (unwind != current) {
				SuspendFrame(unwind);
				unwind->Error = error;
				unwind		  = unwind->Parent;
			}

			return;	 // catching frame resumes at catch block
		} else if (current->IsAsync) {
			Promise* promise = CoerceToPromise(current->Promise);
			PromiseReject(interpreter, promise, error);
			SuspendFrame(current);
			return;
		}
		current = current->Parent;
	}

unhandled:;
	// 2. SYNCHRONOUS CONTEXT: Nothing caught it, crash the process
	String errStr	 = ValueToString(error);
	String traceback = _DumpTraceback(interpreter, frame, errStr);
	fprintf(stderr, "%s", traceback);
	free(errStr);
	free(traceback);
	InterpreterPanicExit(interpreter);
}

static Value*
_CreateError(Interpreter* interpreter, const String type, String message) {
	String fmt = FormatString("%s: %s", type, message);
	Value* err = NewErrorValue(interpreter, fmt);
	free(message);
	free(fmt);
	return err;
}

void MarkCallFrame(CallFrame* frame) {
	CallFrame* current = frame;
	while (current != NULL) {
		Mark(current->Fn);
		Mark(current->Env);
		Mark(current->GlobalEnv);
		Mark(current->Error);
		for (int i = 0; i < current->OperandC; i++) {
			Mark(current->Operand[i]);
		}
		current = current->Parent;
	}
}

Value* Run(Interpreter* interpreter, CallFrame* frame, Value* continuePromise) {
	Value*		  fn	 = frame->Fn;
	UserFunction* uf	 = CoerceToUserFunction(frame->Fn);
	uint8_t		  opcode = 0;
	Value*		  lhs	 = NULL;
	Value*		  rhs	 = NULL;
	Value*		  res	 = NULL;
	Environment*  env	 = NULL;
	int			  off	 = 0;

	if (uf == NULL) {
		InterpreterPanic("Attempted to run a non-function value of type %s",
						 ValueTypeOf(frame->Fn));
	}

#define ip					   (frame->Ip)
#define Running				   (frame->Ip < uf->CodeC && !frame->Suspend)
#define Forward(size)		   (frame->Ip += size)
#define JmpFrwd(addr)		   (frame->Ip = addr)
#define SetLocalHandler(value) (localhandler = value)

	while (Running) {
		if (interpreter->Allocated >= interpreter->GcThreshold) {
			MarkCallFrame(frame);
			GarbageCollect(interpreter);
		}

		opcode = uf->Codes[frame->Ip++];

		switch (opcode) {
			case OP_IMPORT_CORE:
				{
					String path = ReadString(uf->Codes, ip);
					res			= DoImportCore(interpreter, path);
					if (ValueIsError(res)) {
						free(path);
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, false);
						break;
					}
					FPush(interpreter, frame, res);
					Forward(strlen(path) + 1);
					free(path);
					break;
				}
			case OP_IMPORT_LIB:
				{
					String path = ReadString(uf->Codes, ip);
					res			= DoImportLib(interpreter, path);
					if (ValueIsError(res)) {
						free(path);
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, false);
						break;
					}
					FPush(interpreter, frame, res);
					Forward(strlen(path) + 1);
					free(path);
					break;
				}
			case OP_IMPORT_RELATIVE:
				{
					String path = ReadString(uf->Codes, ip);
					res			= DoImportFile(interpreter, path);
					if (ValueIsError(res)) {
						free(path);
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, false);
						break;
					}
					FPush(interpreter, frame, res);
					Forward(strlen(path) + 1);
					free(path);
					break;
				}
			case OP_LOAD_CAPTURE:
				{
					off		   = ReadInt32(uf->Codes, ip);
					Value* val = GetCap(uf, off);
					if (val == NULL) {
						Value* err = _CreateError(
							interpreter,
							REFERENCE_ERROR,
							"variable is referenced before initialization");
						// Raise
						_RaiseError(interpreter, frame, continuePromise, err, false);
						break;
					}
					FPush(interpreter, frame, val);
					Forward(4);
					break;
				}
			case OP_LOAD_NAME:
				{
					off		   = ReadInt32(uf->Codes, ip);
					Value* val = GetVar(frame->GlobalEnv, off);
					if (val == NULL) {
						Value* err = _CreateError(
							interpreter,
							REFERENCE_ERROR,
							"variable is referenced before initialization");
						// Raise
						_RaiseError(interpreter, frame, continuePromise, err, false);
						break;
					}
					FPush(interpreter, frame, val);
					Forward(4);
					break;
				}
			case OP_LOAD_LOCAL:
				{
					off		   = ReadInt32(uf->Codes, ip);
					Value* val = GetVar(frame->Env, off);
					if (val == NULL) {
						Value* err = _CreateError(
							interpreter,
							REFERENCE_ERROR,
							AllocateString(
								"variable is referenced before initialization"));
						// Raise
						_RaiseError(interpreter, frame, continuePromise, err, false);
						break;
					}
					FPush(interpreter, frame, val);
					Forward(4);
					break;
				}
			case OP_LOAD_CONST:
				{
					off = ReadInt32(uf->Codes, ip);
					FPush(interpreter, frame, interpreter->Constants[off]);
					Forward(4);
					break;
				}
			case OP_LOAD_INT:
				{
					off = ReadInt32(uf->Codes, ip);
					FPush(interpreter, frame, NewIntValue(interpreter, off));
					Forward(4);
					break;
				}
			case OP_LOAD_BOOL:
				{
					off = ReadInt32(uf->Codes, ip);
					FPush(interpreter,
						  frame,
						  off ? interpreter->True : interpreter->False);
					Forward(4);
					break;
				}
			case OP_LOAD_NULL:
				{
					FPush(interpreter, frame, interpreter->Null);
					break;
				}
			case OP_LOAD_STRING:
				{
					String str = ReadString(uf->Codes, ip);
					FPush(interpreter, frame, NewStrValue(interpreter, str));
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_ARRAY_EXTEND:
				{
					Value* ext = FPopp(interpreter, frame);
					Value* arr = FPeek(interpreter, frame);
					if (!ValueIsArray(ext)) {
						// Pop the array that was extended
						FPopp(interpreter, frame);
						Value* err =
							_CreateError(interpreter,
										 TYPE_ERROR,
										 FormatString("expected array to extend to "
													  "be an array, got %s",
													  ValueTypeOf(ext)));
						// Raise
						_RaiseError(interpreter, frame, continuePromise, err, true);
						break;
					}
					ArrayExtend(CoerceToArray(arr), CoerceToArray(ext));
					break;
				}
			case OP_ARRAY_PUSH:
				{
					Value* val = FPopp(interpreter, frame);
					Value* arr = FPeek(interpreter, frame);
					if (!ValueIsArray(arr)) {
						// Pop the array that was pushed
						FPopp(interpreter, frame);
						Value* err =
							_CreateError(interpreter,
										 TYPE_ERROR,
										 FormatString("expected array to push "
													  "to be an array, got %s",
													  ValueTypeOf(arr)));
						// Raise
						_RaiseError(interpreter, frame, continuePromise, err, true);
						break;
					}
					ArrayPush(CoerceToArray(arr), val);
					break;
				}
			case OP_ARRAY_MAKE:
				{
					int	   size	 = ReadInt32(uf->Codes, ip);
					Value* arr	 = NewArrayValue(interpreter);
					Array* array = CoerceToArray(arr);
					for (int i = 0; i < size; i++) {
						Value* val = FPeekAt(interpreter, frame, size - i);
						ArrayPush(array, val);
					}
					FPopN(interpreter, frame, size);
					FPush(interpreter, frame, arr);
					Forward(4);
					break;
				}
			case OP_OBJECT_EXTEND:
				{
					Value* ext = FPopp(interpreter, frame);
					Value* obj = FPeek(interpreter, frame);
					if (!ValueIsObject(ext)) {
						// Pop the object that was extended
						FPopp(interpreter, frame);
						Value* err =
							_CreateError(interpreter,
										 TYPE_ERROR,
										 FormatString("expected object to extend to "
													  "be an object, got %s",
													  ValueTypeOf(ext)));
						// Raise
						_RaiseError(interpreter, frame, continuePromise, err, true);
						break;
					}
					HashMapExtend(CoerceToHashMap(obj), CoerceToHashMap(ext));
					break;
				}
			case OP_OBJECT_MAKE:
				{
					int		 size = ReadInt32(uf->Codes, ip);
					Value*	 obj  = NewObjectValue(interpreter);
					HashMap* map  = CoerceToHashMap(obj);
					for (int i = 0; i < size; i++) {
						Value* key	  = FPopp(interpreter, frame);
						Value* val	  = FPopp(interpreter, frame);
						String keyStr = ValueToString(key);
						HashMapSet(map, keyStr, val);
						free(keyStr);
					}
					FPush(interpreter, frame, obj);
					Forward(4);
					break;
				}
			case OP_OBJECT_PLUCK_ATTRIBUTE:
				{
					String	 str = ReadString(uf->Codes, ip);
					Value*	 obj = FPeek(interpreter, frame);
					HashMap* map = CoerceToHashMap(obj);
					Value*	 val = HashMapGet(map, str);
					FPush(interpreter, frame, (val == NULL) ? interpreter->Null : val);
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_CLASS_EXTEND:
				{
					Value* ext = FPopp(interpreter, frame);	 // super class
					Value* cls = FPeek(interpreter, frame);	 // class being extended
					if (!ValueIsClass(ext)) {
						// Pop the class that was extended
						FPopp(interpreter, frame);
						Value* err =
							_CreateError(interpreter,
										 TYPE_ERROR,
										 FormatString("expected class to extend to "
													  "be a class, got %s",
													  ValueTypeOf(ext)));
						// Raise
						_RaiseError(interpreter, frame, continuePromise, err, false);
						break;
					}
					ClassExtend(CoerceToUserClass(cls), ext);
					break;
				}
			case OP_CLASS_MAKE:
				{
					String str = ReadString(uf->Codes, ip);
					Value* obj =
						NewClassValue(interpreter,
									  CreateUserClass(str, interpreter->Object));
					FPush(interpreter, frame, obj);
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_CLASS_DEFINE_STATIC_MEMBER:
			case OP_CLASS_DEFINE_INSTANCE_MEMBER:
				{
					Value* key = FPopp(interpreter, frame);
					Value* val = FPopp(interpreter, frame);
					Value* obj = FPeek(interpreter, frame);
					ClassDefineMember(CoerceToUserClass(obj),
									  key,
									  val,
									  (opcode == OP_CLASS_DEFINE_STATIC_MEMBER));
					break;
				}
			case OP_CLASS_GETBASE:
				{
					Value* obj = FPopp(interpreter, frame);
					if (!ValueIsClassInstance(obj)) {
						FPush(interpreter, frame, interpreter->Object);
						break;
					}
					ClassInstance* inst = CoerceToClassInstance(obj);
					Class*		   cls	= CoerceToUserClass(inst->Proto);
					FPush(interpreter,
						  frame,
						  cls->Base != NULL ? cls->Base : interpreter->Object);
					break;
				}
			case OP_SET_INDEX:
				{
					Value* val = FPopp(interpreter, frame);
					Value* key = FPopp(interpreter, frame);
					Value* obj = FPeek(interpreter, frame);
					Value* res = DoSetIndex(interpreter, obj, key, val);
					if (ValueIsError(res)) {
						// Pop the object
						FPopp(interpreter, frame);
						//  Pop the duplicated value
						FPopp(interpreter, frame);
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					break;
				}
			case OP_GET_INDEX:
				{
					Value* key = FPopp(interpreter, frame);
					Value* obj = FPopp(interpreter, frame);
					res		   = DoGetIndex(interpreter, obj, key);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_LOAD_FUNCTION_CLOSURE:
			case OP_LOAD_FUNCTION:
				{
					off = ReadInt32(uf->Codes, ip);
					res = DoLoadFunction(interpreter,
										 frame,
										 off,
										 (opcode == OP_LOAD_FUNCTION_CLOSURE));
					FPush(interpreter, frame, res);
					Forward(4);
					break;
				}
			case OP_CALL_CTOR:
				{
					int argc = ReadInt32(uf->Codes, ip);
					Forward(4);
					Value* cls = FPopp(interpreter, frame);
					res		   = DoCallCtor(interpreter, frame, cls, argc);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					break;
				}
			case OP_CALL:
				{
					int argc = ReadInt32(uf->Codes, ip);
					Forward(4);
					Value* obj = FPopp(interpreter, frame);
					Value* res = DoCall(interpreter, frame, obj, argc, false);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					} else {
						FPush(interpreter, frame, res);
					}
					break;
				}
			case OP_CALL_METHOD:
				{
					int argc = ReadInt32(uf->Codes, ip);
					Forward(4);
					Value* key = FPopp(interpreter, frame);	 // method name
					Value* obj =
						FPeekAt(interpreter, frame, argc);	 // receiver ('this')
					res = DoCallMethod(interpreter, frame, obj, key, argc);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					} else {
						FPush(interpreter, frame, res);
					}
					break;
				}
			case OP_NOT:
				{
					rhs = FPopp(interpreter, frame);
					res = DoNot(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_BITNOT:
				{
					rhs = FPopp(interpreter, frame);
					res = DoBitNot(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_POS:
				{
					rhs = FPopp(interpreter, frame);
					res = DoPos(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_NEG:
				{
					rhs = FPopp(interpreter, frame);
					res = DoNeg(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_AWAIT:
				{
					Value* val = FPopp(interpreter, frame);
					if (!ValueIsPromise(val)) {
						Value* err =
							_CreateError(interpreter,
										 TYPE_ERROR,
										 FormatString("expected a Promise, got %s",
													  ValueTypeOf(val)));
						_RaiseError(interpreter, frame, continuePromise, err, true);
						break;
					}

					Promise* promiseToWait = CoerceToPromise(val);

					if (promiseToWait->State == FULFILLED) {
						FPush(interpreter, frame, promiseToWait->Result);
						break;
					}

					if (promiseToWait->State == REJECTED) {
						// throw into the coroutine or propagate
						_RaiseError(interpreter,
									frame,
									continuePromise,
									promiseToWait->Result,
									true);
						break;
					}

					// Suspend current frame
					SuspendFrame(frame);

					if (continuePromise != NULL) {
						PromiseAddReaction(promiseToWait, continuePromise);
						EnqueueTask(interpreter, continuePromise);
						return continuePromise;
					} else {
						Value* promise =
							NewPromiseValue(interpreter,
											CreatePromise(PENDING, frame));
						PromiseAddReaction(promiseToWait, promise);
						EnqueueTask(interpreter, promise);
						return promise;
					}
				}
			case OP_GET_AWAITED_VALUE:
				{
					// The continuation promise's result was placed here when the frame
					// was resumed Promise* p = CoerceToPromise(frame->AsyncPromise);
					// FPush(interpreter, frame, p->Result);
					break;
				}
			case OP_GETTYPE:
				{
					Value* val = FPopp(interpreter, frame);
					res		   = DoGetType(interpreter, val);
					FPush(interpreter, frame, res);
					break;
				}
			case OP_MUL:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoMul(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_DIV:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoDiv(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_MOD:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoMod(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_POSTINC:
				{
					// bot [obj, key, val] top
					lhs = FPopp(interpreter, frame);  // old value
					res = DoInc(interpreter, lhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					FPush(interpreter, frame, lhs);
					break;
				}
			case OP_INC:
				{
					rhs = FPopp(interpreter, frame);
					res = DoInc(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_ADD:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoAdd(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_POSTDEC:
				{
					// bot [obj, key, val] top
					lhs = FPopp(interpreter, frame);  // old value
					res = DoDec(interpreter, lhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					FPush(interpreter, frame, lhs);
					break;
				}
			case OP_DEC:
				{
					rhs = FPopp(interpreter, frame);
					res = DoDec(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_SUB:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoSub(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_LSHFT:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoLShift(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_RSHFT:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoRShift(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_LT:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoLT(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_LTE:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoLTE(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_GT:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoGT(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_GTE:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoGTE(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_EQ:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoEQ(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_NE:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoNE(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_AND:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoAnd(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_OR:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoOr(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_XOR:
				{
					rhs = FPopp(interpreter, frame);
					lhs = FPopp(interpreter, frame);
					res = DoXor(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, frame, continuePromise, res, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_STORE_CAPTURE:
				{
					off = ReadInt32(uf->Codes, ip);
					SetCap(uf, off, FPopp(interpreter, frame));
					Forward(4);
					break;
				}
			case OP_STORE_NAME:
				{
					off = ReadInt32(uf->Codes, ip);
					SetVar(frame->GlobalEnv, off, FPopp(interpreter, frame));
					Forward(4);
					break;
				}
			case OP_STORE_LOCAL:
				{
					off = ReadInt32(uf->Codes, ip);
					SetVar(frame->Env, off, FPopp(interpreter, frame));
					Forward(4);
					break;
				}
			case OP_LOCK_VAR:
				{
					off = ReadInt32(uf->Codes, ip);
					LockVar(frame->Env, off);
					Forward(4);
					break;
				}
			case OP_DUPTOP:
				{
					FPush(interpreter, frame, FPeek(interpreter, frame));
					break;
				}
			case OP_DUP2:
				{
					Value* a = FPeekAt(interpreter, frame, 1);
					Value* b = FPeekAt(interpreter, frame, 2);
					FPush(interpreter, frame, b);
					FPush(interpreter, frame, a);
					break;
				}
			case OP_POPTOP:
				{
					FPopp(interpreter, frame);
					break;
				}
			case OP_ROT2:
				{
					// A B -> B A
					Value* a = FPeekAt(interpreter, frame, 1);
					Value* b = FPeekAt(interpreter, frame, 2);
					frame->Operand[frame->OperandC - 1] = b;
					frame->Operand[frame->OperandC - 2] = a;
					break;
				}
			case OP_ROT3:
				{
					// A B C -> C A B
					Value* a = FPeekAt(interpreter, frame, 1);
					Value* b = FPeekAt(interpreter, frame, 2);
					Value* c = FPeekAt(interpreter, frame, 3);
					frame->Operand[frame->OperandC - 1] = c;
					frame->Operand[frame->OperandC - 2] = a;
					frame->Operand[frame->OperandC - 3] = b;
					break;
				}
			case OP_ROT4:
				{
					// A B C D -> D A B C
					Value* d = FPeekAt(interpreter, frame, 1);
					Value* c = FPeekAt(interpreter, frame, 2);
					Value* b = FPeekAt(interpreter, frame, 3);
					Value* a = FPeekAt(interpreter, frame, 4);
					frame->Operand[frame->OperandC - 1] = c;
					frame->Operand[frame->OperandC - 2] = b;
					frame->Operand[frame->OperandC - 3] = a;
					frame->Operand[frame->OperandC - 4] = d;
					break;
				}
			case OP_SETUP_TRY:
				{
					off = ReadInt32(uf->Codes, ip);
					PushTry(frame, off);
					Forward(4);
					break;
				}
			case OP_POP_TRY:
				{
					PoppTry(frame);
					break;
				}
			case OP_POPN_TRY:
				{
					int size = ReadInt32(uf->Codes, ip);
					PopNTry(frame, size);
					Forward(4);
					break;
				}
			case OP_JUMP_IF_FALSE_OR_POP:
				{
					off				 = ReadInt32(uf->Codes, ip);
					Value* condition = FPeek(interpreter, frame);
					if (!CoerceToBool(condition)) {
						JmpFrwd(off);
					} else {
						FPopp(interpreter, frame);
						Forward(4);
					}
					break;
				}
			case OP_JUMP_IF_TRUE_OR_POP:
				{
					off				 = ReadInt32(uf->Codes, ip);
					Value* condition = FPeek(interpreter, frame);
					if (CoerceToBool(condition)) {
						JmpFrwd(off);
					} else {
						FPopp(interpreter, frame);
						Forward(4);
					}
					break;
				}
			case OP_POP_JUMP_IF_FALSE:
				{
					off				 = ReadInt32(uf->Codes, ip);
					Value* condition = FPopp(interpreter, frame);
					if (CoerceToBool(condition) == false) {
						JmpFrwd(off);
					} else {
						Forward(4);
					}
					break;
				}
			case OP_POP_JUMP_IF_TRUE:
				{
					off				 = ReadInt32(uf->Codes, ip);
					Value* condition = FPopp(interpreter, frame);
					if (CoerceToBool(condition) == true) {
						JmpFrwd(off);
					} else {
						Forward(4);
					}
					break;
				}
			case OP_JUMP:
				{
					off = ReadInt32(uf->Codes, ip);
					JmpFrwd(off);
					break;
				}
			case OP_ABSOLUTE_JUMP:
				{
					off = ReadInt32(uf->Codes, ip);
					JmpFrwd(off);
					break;
				}
			case OP_RAISE:
				{
					_RaiseError(interpreter,
								frame,
								continuePromise,
								FPopp(interpreter, frame),
								true);
					break;
				}
			case OP_RETURN:
				{
					Value* val = FPopp(interpreter, frame);

					if (continuePromise != NULL) {
						PromiseFulfill(interpreter,
									   CoerceToPromise(continuePromise),
									   val);
						return continuePromise;
					}

					if (uf->Async) {
						// Creates a NEW promise — but nobody holds a reference to it
						return NewPromiseValue(interpreter,
											   CreatePromise(FULFILLED, frame));
					}

					return val;
				}
			default:
				{
					InterpreterPanic("Unknown opcode: %s, %d %d\n",
									 uf->Name != NULL ? uf->Name : "<anonymous>",
									 opcode,
									 OP_LOAD_NAME);
					return interpreter->Null;
				}
		}
	}

	// Sync
	if (frame->Error != NULL) {
		String errorStr	 = ValueToString(frame->Error);
		String trabeBack = _DumpTraceback(interpreter, frame, errorStr);
		fprintf(stderr, "%s", trabeBack);
		free(trabeBack);
		free(errorStr);
		InterpreterPanicExit(interpreter);
	}

	// Async
	if (frame->IsAsync) {
		return frame->Promise;
	}

	return interpreter->Null;
}

static void _DispatchTask(Interpreter* interpreter, Value* task);

void _RunProgram(Interpreter* interpreter, Value* fn) {
	UserFunction* uf  = CoerceToUserFunction(fn);
	Value*		  env = NULL;
	env = NewEnvironmentValue(interpreter, CreateEnvironment(NULL, uf->LocalC));

	interpreter->ModulePath = uf->Name;

	CallFrame* gFrame = InitCallFrame(NULL, env, env, fn, false);

	Run(interpreter, gFrame, NULL);

	// Event loop: process pending tasks and mongoose I/O
	// together. Keep spinning while there are queued tasks OR
	// active mongoose connections (which may produce new tasks).
	for (;;) {
		// Native event callbacks (e.g. mongoose) use CurrentFrame
		// for temporary argument passing and function dispatch.
		// Keep the root frame active while polling the event loop.
		// SetCurrentFrame(interpreter, gFrame);

		// Poll mongoose for I/O events (non-blocking with a
		// short timeout). This may trigger callbacks that
		// resolve promises and enqueue new tasks.
		int timeoutMs = HasPendingTasks(interpreter)
							? 0
							: 50;  // Adjust as needed for responsiveness
		mg_mgr_poll(&interpreter->MgMgr, timeoutMs);

		Value* task = DequeueTask(interpreter);
		if (task == NULL) {
			// No tasks right now.  If mongoose still has active
			// connections keep polling; otherwise we are done.
			if (interpreter->MgMgr.conns == NULL)
				break;
			continue;
		}

		_DispatchTask(interpreter, task);
	}

	ForceGarbageCollect(interpreter);
}

/* Called after a .then()/.catch() callback returns.
 * Fulfills or rejects the next promise in the chain. */
static void
_ChainResult(Interpreter* interpreter, Promise* current, Value* returnVal) {
	/* Walk current's FulfillReactions — these are the
	 * next .then()s waiting on this callback's result */
	ListStateMachineNode* node = current->FullfillReactions;
	if (node == NULL)
		return; /* end of chain */

	if (returnVal == NULL || ValueIsNull(returnVal)) {
		/* callback returned void/null — fulfill with null */
		while (node != NULL) {
			PromiseFulfill(interpreter,
						   CoerceToPromise(node->Promise),
						   interpreter->Null);
			node = node->Next;
		}

	} else if (ValueIsError(returnVal)) {
		/* callback threw — reject the next promise */
		while (node != NULL) {
			PromiseReject(interpreter, CoerceToPromise(node->Promise), returnVal);
			node = node->Next;
		}

	} else if (ValueIsPromise(returnVal)) {
		/* callback returned a promise — next promise waits
		 * on it instead of resolving immediately.
		 *
		 * Example:
		 *   fetch().then(r => r.json())   // r.json() is a promise
		 *          .then(d => use(d))     // waits for json() to resolve
		 */
		Promise* returned = CoerceToPromise(returnVal);
		while (node != NULL) {
			if (returned->State == FULFILLED) {
				PromiseFulfill(interpreter,
							   CoerceToPromise(node->Promise),
							   returned->Result);
			} else if (returned->State == REJECTED) {
				PromiseReject(interpreter,
							  CoerceToPromise(node->Promise),
							  returned->Result);
			} else {
				/* Still pending — register as reaction on the
				 * returned promise so it resolves when ready */
				PromiseFulfillReactionAdd(returned, node->Promise);
				PromiseRejectReactionAdd(returned, node->Promise);
			}
			node = node->Next;
		}

	} else {
		/* Plain value — fulfill next promise with it */
		while (node != NULL) {
			PromiseFulfill(interpreter, CoerceToPromise(node->Promise), returnVal);
			node = node->Next;
		}
	}
}

static void _DispatchTask(Interpreter* interpreter, Value* task) {
	Promise* p = CoerceToPromise(task);

	switch (p->State) {
		case FULFILLED:
			{
				if (p->SuspendedCallFrame != NULL) {
					/* await path — just resume, no chaining needed */
					Run(interpreter, p->SuspendedCallFrame, task);

				} else if (p->Callback != NULL) {
					/* .then() path */
					CallFrame*	 frame = p->SuspendedCallFrame;
					Environment* env   = CoerceToEnvironment(frame->Env);

					/* Inject the input value as argument slot 0 */
					EnvironmentSetLocal(env, 0, p->Result);

					Value* returnVal = Run(interpreter, frame, task);

					/* ── THE CHAINING LOGIC ──────────────────────────
					 * What the callback returned determines what
					 * happens to the NEXT promise in the chain. */
					_ChainResult(interpreter, p, returnVal);
				}
				break;
			}

		case REJECTED:
			{
				if (p->SuspendedCallFrame != NULL) {
					/* await rejection — raise into the frame */
					Value* err					 = p->SuspendedCallFrame->Error;
					p->SuspendedCallFrame->Error = NULL;
					_RaiseError(interpreter, p->SuspendedCallFrame, task, err, true);
					if (!p->SuspendedCallFrame->Suspend) {
						Run(interpreter, p->SuspendedCallFrame, task);
					}

				} else if (p->Callback != NULL) {
					/* .catch() path — same as .then() but called on reject */
					CallFrame*	 frame = p->SuspendedCallFrame;
					Environment* env   = CoerceToEnvironment(frame->Env);

					/* Inject rejection reason as argument slot 0 */
					EnvironmentSetLocal(env, 0, p->Result);

					Value* returnVal = Run(interpreter, frame, task);

					/* After .catch() runs, result flows forward
					 * as a fulfillment (error was handled) */
					_ChainResult(interpreter, p, returnVal);
				}
				break;
			}

		case PENDING:
			assert(0 && "dequeued a PENDING promise — enqueue logic broken");
			break;
	}
}

void Interpret(Interpreter* interpreter, Value* fn /*UserFunction*/) {
	_RunProgram(interpreter, fn);
}

void FreeInterpreter(Interpreter* interpreter) {
	mg_mgr_free(&interpreter->MgMgr);
	FreeHashMap(interpreter->Imports);
	FreeImportNode(interpreter->ImportHead);
	bf_context_end(&interpreter->BfContext);
	if (interpreter->ExecPath)
		free(interpreter->ExecPath);
	if (interpreter->ArgString)
		free(interpreter->ArgString);
	free(interpreter->Constants);
	free(interpreter->Functions);
	free(interpreter);
}

void InterpreterPanicExit(Interpreter* interpreter) {
	ForceGarbageCollect(interpreter);
	FreeInterpreter(interpreter);
	fprintf(stderr, "Program exited with panic.\n");
	exit(EXIT_FAILURE);
}

#undef SetVar
#undef GetVar
#undef SetCap
#undef GetCap
#undef LockVar
#undef InterpreterPanic
#undef JumpToError
#undef ip
#undef Running
#undef Forward
#undef JmpFrwd
#undef SetLocalHandler
