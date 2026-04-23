#include "./interpreter.h"

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
	interpreter->ExecPath	  = AllocateString(execPath);
	interpreter->ModulePath	  = NULL;
	interpreter->ArgString	  = NULL;
	interpreter->ImportHead	  = NULL;
	interpreter->Imports	  = CreateHashMap(16);
	interpreter->Allocated	  = 0;
	interpreter->GcThreshold  = GC_THRESHOLD;
	interpreter->GcRoot		  = NULL;
	interpreter->RootEnv	  = NULL;
	interpreter->CallEnv	  = NULL;
	interpreter->Object		  = CreateObjectClass(interpreter);	  // Singleton
	interpreter->Array		  = CreateArrayClass(interpreter);	  // Singleton
	interpreter->Date		  = CreateDateClass(interpreter);	  // Singleton
	interpreter->Promise	  = CreatePromiseClass(interpreter);  // Singleton
	interpreter->True		  = NewBoolValue(interpreter, 1);	  // Singletons
	interpreter->False		  = NewBoolValue(interpreter, 0);	  // Singletons
	interpreter->Null		  = NewNullValue(interpreter);		  // Singletons
	interpreter->Constants	  = Allocate(sizeof(Value*));
	interpreter->ConstantC	  = 0;
	interpreter->Constants[0] = NULL;
	interpreter->Functions	  = Allocate(sizeof(Value*));
	interpreter->FunctionC	  = 0;
	interpreter->Functions[0] = NULL;
	// interpreter->Stacks[STACK_SIZE];
	interpreter->StckC = 0;
	// interpreter->Envs[STACK_SIZE];
	interpreter->EnvrC = 0;
	// interpreter->ExceptionHandlerStacks[STACK_SIZE];
	interpreter->ExceptionHandlerStackC = 0;
	// interpreter->TaskQueue[STACK_SIZE];
	interpreter->TaskQueueHead	= 0;
	interpreter->TaskQueueC		= 0;
	interpreter->ActiveFunction = NULL;
	interpreter->ActiveTask		= NULL;
	mg_mgr_init(&interpreter->MgMgr);
	return interpreter;
}

#define SetVar(envObj, offset, value)                                          \
	EnvironmentSetLocal(CoerceToEnvironment(envObj), offset, value)

#define GetVar(envObj, offset)                                                 \
	EnvironmentGetLocal(CoerceToEnvironment(envObj), offset)->Value

#define GetCap(uFunct, offset) UserFunctionGetCapture(uFunct, offset)

#define SetCap(uFunct, offset, value)                                          \
	UserFunctionSetCapture(uFunct, offset, value)

#define LockVar(envObj, offset)                                                \
	{                                                                          \
		Environment* env  = CoerceToEnvironment(envObj);                       \
		EnvCell*	 cell = EnvironmentGetLocal(env, offset);                  \
		if (cell->RefCount > 0 && cell->IsCaptured) {                          \
			cell->RefCount--;                                                  \
			env->Locals[offset] = CreateEnvCell(cell->Value);                  \
		}                                                                      \
	}

#define DumpStack()                                                            \
	do {                                                                       \
		printf("Stack [%d items] Pointer(%zu): [ ",                            \
			   interpreter->StckC,                                             \
			   (size_t) interpreter->StckC);                                   \
		for (int i = 0; i < interpreter->StckC; i++) {                         \
			if (i > 0)                                                         \
				printf(", ");                                                  \
			String str = ValueToString(interpreter->Stacks[i]);                \
			printf("%s", str);                                                 \
			free(str);                                                         \
		}                                                                      \
		printf(" ]\n");                                                        \
	} while (0)

#define InterpreterPanic(message, ...)                                         \
	do {                                                                       \
		fprintf(stderr, "[%s:%d]::Panic: ", __FILE__, __LINE__);               \
		fprintf(stderr, message, ##__VA_ARGS__);                               \
		fprintf(stderr, "\n");                                                 \
		ForceGarbageCollect(interpreter);                                      \
		FreeInterpreter(interpreter);                                          \
		fprintf(stderr, "Program exited with panic.\n");                       \
		exit(EXIT_FAILURE);                                                    \
	} while (0)

#define HandleError(messageFormat, ...)                                        \
	{                                                                          \
		int size =                                                             \
			snprintf(NULL, 0, (String) messageFormat, ##__VA_ARGS__) + 1;      \
		String message = (String) Allocate(size);                              \
		snprintf(message, size, (String) messageFormat, ##__VA_ARGS__);        \
		if (catched) {                                                         \
			JmpFrwd(PeekEH());                                                 \
			PoppEH();                                                          \
			Push(interpreter, NewErrorValue(interpreter, message));            \
			free(message);                                                     \
			break;                                                             \
		}                                                                      \
		InterpreterPanic(message);                                             \
		free(message);                                                         \
		break;                                                                 \
	}

void SetActiveFunction(Interpreter* interpreter, Value* function) {
	interpreter->ActiveFunction = function;
}

void SetActiveTask(Interpreter* interpreter, Value* task) {
	interpreter->ActiveTask = task;
}

void Push(Interpreter* interpreter, Value* value) {
	interpreter->Stacks[interpreter->StckC++] = value;
}

Value* Popp(Interpreter* interpreter) {
	return interpreter->Stacks[--interpreter->StckC];
}

void PopN(Interpreter* interpreter, int n) {
	interpreter->Stacks[interpreter->StckC -= (n)];
}

Value* Peek(Interpreter* interpreter) {
	return interpreter->Stacks[interpreter->StckC - 1];
}

Value* PeekAt(Interpreter* interpreter, int n) {
	return interpreter->Stacks[(interpreter->StckC) - (n)];
}

void PushTrace(Interpreter* interpreter, LineInfo line, Value* fn) {
	interpreter->CallStack[interpreter->CallStackC++] = (StackTrace){
		.line	  = line,
		.Function = fn,
	};
}

void PopTrace(Interpreter* interpreter) {
	--interpreter->CallStackC;
}

/******* Task Queue Management */
bool HasPendingTasks(Interpreter* interpreter) {
	return interpreter->TaskQueueC > 0;
}

void EnqueueTask(Interpreter* interpreter, Value* task) {
	if (interpreter->TaskQueueC >= STACK_SIZE) {
		InterpreterPanic("Task queue overflow");
	}
	int tail =
		(interpreter->TaskQueueHead + interpreter->TaskQueueC) % STACK_SIZE;
	interpreter->TaskQueue[tail] = task;
	interpreter->TaskQueueC++;
}

Value* DequeueTask(Interpreter* interpreter) {
	if (interpreter->TaskQueueC == 0) {
		return NULL;
	}
	Value* task = interpreter->TaskQueue[interpreter->TaskQueueHead];
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
	int offset	= 0;
	offset	   |= codes[alignStart + 0] << 24;
	offset	   |= codes[alignStart + 1] << 16;
	offset	   |= codes[alignStart + 2] << 8;
	offset	   |= codes[alignStart + 3] << 0;
	return offset;
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

/******* TryCatch manipulation */
static void _PushTry(Interpreter* interpreter, int jmp, size_t* pausedAddress) {
	interpreter->ExceptionHandlerStacks[interpreter->ExceptionHandlerStackC++] =
		(ExceptionHandler){
			.JumpAddress   = jmp,
			.PausedAddress = pausedAddress,
		};
}

static void _PopNTry(Interpreter* interpreter, int n) {
	interpreter
		->ExceptionHandlerStacks[interpreter->ExceptionHandlerStackC -= (n)];
}

static void _PoppTry(Interpreter* interpreter) {
	_PopNTry(interpreter, 1);
}

static ExceptionHandler _PeekTry(Interpreter* interpreter) {
	return interpreter
		->ExceptionHandlerStacks[interpreter->ExceptionHandlerStackC - 1];
}

/******* Exception manipulation */

#define isCatched() (interpreter->ExceptionHandlerStackC != 0)

#define JumpToError(ip, addr) (*ip = addr)

#define DumpTraceBack(uf, ip)                                                  \
	do {                                                                       \
		for (int i = 0; i < uf->LineC; i++) {                                  \
			fprintf(stderr,                                                    \
					"[%s:%d] == %d\n",                                         \
					uf->Lines[i].Path,                                         \
					uf->Lines[i].Line,                                         \
					ip);                                                       \
		}                                                                      \
		fprintf(stderr, "\n");                                                 \
	} while (0)

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
	return (LineInfo){};
}

static void _RaiseError(Interpreter* interpreter,
						Value*		 fn,
						Value*		 error,
						size_t*		 ip,
						bool		 catchable) {
	UserFunction* uf = CoerceToUserFunction(fn);
	if (interpreter->ActiveTask != NULL && catchable) {
		StateMachine* activeTask =
			CoerceToStateMachine(interpreter->ActiveTask);
		StateMachineReject(activeTask, error);
		Push(interpreter, interpreter->ActiveTask);
		JumpToError(ip, uf->CodeC);
		return;
	}

	if (isCatched() && catchable) {
		ExceptionHandler handler = _PeekTry(interpreter);
		/* Caught: preserve the original error value as-is for
		 * the catch handler
		 */
		// Jump the current function to end
		JumpToError(ip, uf->CodeC);
		// Jump the main function to the handle
		JumpToError(handler.PausedAddress, handler.JumpAddress);
		Push(interpreter, error);
		return;
	}
	/* Uncaught: format for display only, no new error Value is
	 * created */
	LineInfo line	= _GetLineFromPc(uf, *ip);
	String	 errStr = ValueToString(error);
	String	 buf = FormatString("[%s:%d]::%s\n", line.Path, line.Line, errStr);
	free(errStr);

	for (int i = interpreter->CallStackC - 1; i >= 0; i--) {
		StackTrace trace = interpreter->CallStack[i];
		String	   frame =
			FormatString("  |> [%s:%d]\n", trace.line.Path, trace.line.Line);
		String tmp = FormatString("%s%s", buf, frame);
		free(frame);
		free(buf);
		buf = tmp;
	}

	fprintf(stderr, "%s", buf);
	free(buf);
	ForceGarbageCollect(interpreter);
	FreeInterpreter(interpreter);
	fprintf(stderr, "Program exited with panic.\n");
	exit(EXIT_FAILURE);
}

static Value*
_CreateError(Interpreter* interpreter, const String type, String message) {
	String fmt = FormatString("%s: %s", type, message);
	Value* err = NewErrorValue(interpreter, fmt);
	free(message);
	free(fmt);
	return err;
}

/******* Main interpreter loop */
void Run(Interpreter* interpreter, Value* fnOrSm) {
	Value*		  fn = fnOrSm;
	StateMachine* sm = NULL;
	UserFunction* uf =
		ValueIsUserFunction(fnOrSm)
			? CoerceToUserFunction(fnOrSm)
			: CoerceToUserFunction(
				  (fn = (sm = CoerceToStateMachine(fnOrSm))->Function));
	uint8_t		 opcode		  = 0;
	Value*		 lhs		  = NULL;
	Value*		 rhs		  = NULL;
	Value*		 res		  = NULL;
	Value*		 ext		  = NULL;
	Value*		 arr		  = NULL;
	Value*		 obj		  = NULL;
	Value*		 cls		  = NULL;
	Value*		 key		  = NULL;
	Value*		 val		  = NULL;
	Value*		 err		  = NULL;
	Environment* env		  = NULL;
	HashMap*	 map		  = NULL;
	Array*		 array		  = NULL;
	size_t		 ip			  = 0;
	int			 offset		  = 0;
	int			 argc		  = 0;
	int			 flg		  = 0;
	int			 size		  = 0;
	bool		 catched	  = false;
	bool		 localhandler = false;
	String		 str		  = NULL;

	if (ValueIsPromise(fnOrSm))
		ip = sm->Ip;

	if (uf == NULL)
		InterpreterPanic("Attempted to run a non-function value of type %s",
						 ValueTypeOf(fnOrSm));

#define Forward(size)		   (ip += size)
#define JmpFrwd(addr)		   (ip = addr)
#define SetLocalHandler(value) (localhandler = value)

	while (ip != uf->CodeC) {
		if (interpreter->Allocated >= interpreter->GcThreshold) {
			Mark(fn);
			Mark(fnOrSm);
			GarbageCollect(interpreter);
		}

		opcode	= uf->Codes[ip++];
		catched = interpreter->ExceptionHandlerStackC != 0;

		switch (opcode) {
			case OP_IMPORT_CORE:
				{
					str = ReadString(uf->Codes, ip);
					res = DoImportCore(interpreter, str);
					if (ValueIsError(res)) {
						free(str);
						// Raise
						_RaiseError(interpreter, fn, res, &ip, false);
						break;
					}
					Push(interpreter, res);
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_IMPORT_LIB:
				{
					str = ReadString(uf->Codes, ip);
					res = DoImportLib(interpreter, str);
					if (ValueIsError(res)) {
						free(str);
						// Raise
						_RaiseError(interpreter, fn, res, &ip, false);
						break;
					}
					Push(interpreter, res);
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_IMPORT_RELATIVE:
				{
					str = ReadString(uf->Codes, ip);
					res = DoImportFile(interpreter, str);
					if (ValueIsError(res)) {
						free(str);
						// Raise
						_RaiseError(interpreter, fn, res, &ip, false);
						break;
					}
					Push(interpreter, res);
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_LOAD_CAPTURE:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = GetCap(uf, offset);
					if (val == NULL) {
						err = _CreateError(
							interpreter,
							REFERENCE_ERROR,
							"variable is referenced before initialization");
						// Raise
						_RaiseError(interpreter, fn, err, &ip, false);
						break;
					}
					Push(interpreter, val);
					Forward(4);
					break;
				}
			case OP_LOAD_NAME:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = GetVar(interpreter->RootEnv, offset);
					if (val == NULL) {
						err = _CreateError(
							interpreter,
							REFERENCE_ERROR,
							"variable is referenced before initialization");
						// Raise
						_RaiseError(interpreter, fn, err, &ip, false);
						break;
					}
					Push(interpreter, val);
					Forward(4);
					break;
				}
			case OP_LOAD_LOCAL:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = GetVar(interpreter->CallEnv, offset);
					if (val == NULL) {
						err = _CreateError(
							interpreter,
							REFERENCE_ERROR,
							"variable is referenced before initialization");
						// Raise
						_RaiseError(interpreter, fn, err, &ip, false);
						break;
					}
					Push(interpreter, val);
					Forward(4);
					break;
				}
			case OP_LOAD_CONST:
				{
					offset = ReadInt32(uf->Codes, ip);
					Push(interpreter, interpreter->Constants[offset]);
					Forward(4);
					break;
				}
			case OP_LOAD_INT:
				{
					offset = ReadInt32(uf->Codes, ip);
					Push(interpreter, NewIntValue(interpreter, offset));
					Forward(4);
					break;
				}
			case OP_LOAD_BOOL:
				{
					offset = ReadInt32(uf->Codes, ip);
					Push(interpreter,
						 offset ? interpreter->True : interpreter->False);
					Forward(4);
					break;
				}
			case OP_LOAD_NULL:
				{
					Push(interpreter, interpreter->Null);
					break;
				}
			case OP_LOAD_STRING:
				{
					str = ReadString(uf->Codes, ip);
					Push(interpreter, NewStrValue(interpreter, str));
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_ARRAY_EXTEND:
				{
					ext = Popp(interpreter);
					arr = Peek(interpreter);
					if (!ValueIsArray(ext)) {
						// Pop the array that was extended
						Popp(interpreter);
						err = _CreateError(
							interpreter,
							TYPE_ERROR,
							FormatString("expected array to extend to "
										 "be an array, got %s",
										 ValueTypeOf(ext)));
						// Raise
						_RaiseError(interpreter, fn, err, &ip, true);
						break;
					}
					ArrayExtend(CoerceToArray(arr), CoerceToArray(ext));
					break;
				}
			case OP_ARRAY_PUSH:
				{
					val = Popp(interpreter);
					arr = Peek(interpreter);
					if (!ValueIsArray(arr)) {
						// Pop the array that was pushed
						Popp(interpreter);
						err =
							_CreateError(interpreter,
										 TYPE_ERROR,
										 FormatString("expected array to push "
													  "to be an array, got %s",
													  ValueTypeOf(ext)));
						// Raise
						_RaiseError(interpreter, fn, err, &ip, true);
						break;
					}
					ArrayPush(CoerceToArray(arr), val);
					break;
				}
			case OP_ARRAY_MAKE:
				{
					size  = ReadInt32(uf->Codes, ip);
					arr	  = NewArrayValue(interpreter);
					array = CoerceToArray(arr);
					for (int i = 0; i < size; i++) {
						val = PeekAt(interpreter, size - i);
						ArrayPush(array, val);
					}
					PopN(interpreter, size);
					Push(interpreter, arr);
					Forward(4);
					break;
				}
			case OP_OBJECT_EXTEND:
				{
					ext = Popp(interpreter);
					obj = Peek(interpreter);
					if (!ValueIsObject(ext)) {
						// Pop the object that was extended
						Popp(interpreter);
						err = _CreateError(
							interpreter,
							TYPE_ERROR,
							FormatString("expected object to extend to "
										 "be an object, got %s",
										 ValueTypeOf(ext)));
						// Raise
						_RaiseError(interpreter, fn, err, &ip, true);
						break;
					}
					HashMapExtend(CoerceToHashMap(obj), CoerceToHashMap(ext));
					break;
				}
			case OP_OBJECT_MAKE:
				{
					size = ReadInt32(uf->Codes, ip);
					obj	 = NewObjectValue(interpreter);
					map	 = CoerceToHashMap(obj);
					for (int i = 0; i < size; i++) {
						key			  = Popp(interpreter);
						val			  = Popp(interpreter);
						String keyStr = ValueToString(key);
						HashMapSet(map, keyStr, val);
						free(keyStr);
					}
					Push(interpreter, obj);
					Forward(4);
					break;
				}
			case OP_OBJECT_PLUCK_ATTRIBUTE:
				{
					str = ReadString(uf->Codes, ip);
					obj = Peek(interpreter);
					map = CoerceToHashMap(obj);
					val = HashMapGet(map, str);
					Push(interpreter, (val == NULL) ? interpreter->Null : val);
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_CLASS_EXTEND:
				{
					ext = Popp(interpreter);  // super class
					cls = Peek(interpreter);  // class being extended
					if (!ValueIsClass(ext)) {
						// Pop the class that was extended
						Popp(interpreter);
						err = _CreateError(
							interpreter,
							TYPE_ERROR,
							FormatString("expected class to extend to "
										 "be a class, got %s",
										 ValueTypeOf(ext)));
						// Raise
						_RaiseError(interpreter, fn, err, &ip, false);
						break;
					}
					ClassExtend(CoerceToUserClass(cls), ext);
					break;
				}
			case OP_CLASS_MAKE:
				{
					str = ReadString(uf->Codes, ip);
					obj = NewClassValue(
						interpreter,
						CreateUserClass(str, interpreter->Object));
					Push(interpreter, obj);
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_CLASS_DEFINE_STATIC_MEMBER:
			case OP_CLASS_DEFINE_INSTANCE_MEMBER:
				{
					key = Popp(interpreter);
					val = Popp(interpreter);
					obj = Peek(interpreter);
					ClassDefineMember(
						CoerceToUserClass(obj),
						key,
						val,
						(opcode == OP_CLASS_DEFINE_STATIC_MEMBER));
					break;
				}
			case OP_SET_INDEX:
				{
					val = Popp(interpreter);
					key = Popp(interpreter);
					obj = Peek(interpreter);
					res = DoSetIndex(interpreter, obj, key, val);
					if (ValueIsError(res)) {
						// Pop the object
						Popp(interpreter);
						//  Pop the duplicated value
						Popp(interpreter);
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					break;
				}
			case OP_GET_INDEX:
				{
					key = Popp(interpreter);
					obj = Popp(interpreter);
					res = DoGetIndex(interpreter, obj, key);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_LOAD_FUNCTION_CLOSURE:
			case OP_LOAD_FUNCTION:
				{
					offset = ReadInt32(uf->Codes, ip);
					res = DoLoadFunction(interpreter,
										 offset,
										 (opcode == OP_LOAD_FUNCTION_CLOSURE));
					Push(interpreter, res);
					Forward(4);
					break;
				}
			case OP_CALL_CTOR:
				{
					argc = ReadInt32(uf->Codes, ip);
					Forward(4);
					cls = Popp(interpreter);
					res = DoCallCtor(interpreter, cls, argc);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					break;
				}
			case OP_CALL:
				{
					argc = ReadInt32(uf->Codes, ip);
					Forward(4);
					obj = Popp(interpreter);
					res = DoCall(interpreter, obj, argc, false);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					break;
				}
			case OP_CALL_METHOD:
				{
					argc = ReadInt32(uf->Codes, ip);
					Forward(4);
					key = Popp(interpreter);  // method name
					obj = Popp(interpreter);  // 'this' object
					res = DoCallMethod(interpreter, obj, key, argc);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					break;
				}
			case OP_NOT:
				{
					rhs = Popp(interpreter);
					res = DoNot(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_POS:
				{
					rhs = Popp(interpreter);
					res = DoPos(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_NEG:
				{
					rhs = Popp(interpreter);
					res = DoNeg(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_AWAIT:
				{
					if (!ValueIsPromise(Peek(interpreter)))
						Panic("Expected a promise got %s",
							  ValueTypeOf(Peek(interpreter)));

					val = Popp(interpreter);  // The awaited promise

					StateMachineAwait(sm, ip, val);

					sm->Line	= _GetLineFromPc(uf, ip);
					sm->CallEnv = interpreter->CallEnv;

					// 1. Calculate the exact size of the current
					// stack frame
					int size	= interpreter->StckC - sm->StckBot;
					int envsize = interpreter->EnvrC - sm->EnvrBot;

					// Now your Panic message makes perfect
					// sense!
					if (size < 0)
						Panic("Invalid stack state: StckC (%d) "
							  "is less "
							  "than "
							  "StackBot (%d)",
							  interpreter->StckC,
							  (int) sm->StckBot);

					// 2. Free old memory (Make sure 'free'
					// matches 'Allocate'!)
					if (sm->Stacks != NULL) {
						free(sm->Stacks);  // Or your engine's
										   // equivalent memory
										   // freer
						sm->Stacks = NULL;
					}

					if (sm->EnvStack != NULL) {
						free(sm->EnvStack);	 // Or your engine's
											 // equivalent memory
											 // freer
						sm->EnvStack = NULL;
					}

					sm->StckTop = size;
					sm->EnvrTop = envsize;

					// 3. Allocate and copy ONLY this function's
					// variables
					if (size > 0) {
						sm->Stacks = Allocate(sizeof(Value*) * size);

						// This now perfectly copies exactly from
						// StackBot to StckC
						memcpy(sm->Stacks,
							   &interpreter->Stacks[sm->StckBot],
							   sizeof(Value*) * size);
					}

					if (envsize > 0) {
						sm->EnvStack = Allocate(sizeof(Value*) * envsize);

						// This now perfectly copies exactly from
						// EnvBot to EnvrC
						memcpy(sm->EnvStack,
							   &interpreter->Envs[sm->EnvrBot],
							   sizeof(Value*) * envsize);
					}

					// 4. Update StckC to reflect that this
					// function's variables are popped off the
					// main stack
					interpreter->StckC = sm->StckBot;

					// 5. Update EnvrC to reflect that this
					// function's variables are popped off the
					// main env stack
					interpreter->EnvrC = sm->EnvrBot;

					// =================================================================
					// 6. FIX: RESTORE THE CALLER'S ENVIRONMENT
					// BEFORE RETURNING
					// =================================================================
					if (interpreter->EnvrC > 0) {
						interpreter->CallEnv =
							interpreter->Envs[interpreter->EnvrC - 1];
					} else {
						// Fallback: If the stack is empty, we
						// are back at the top level. Replace
						// 'interpreter->GlobalEnv' with whatever
						// your global env is actually named!
						interpreter->CallEnv = interpreter->RootEnv;
					}
					// =================================================================

					StateMachine* awaitedSM = CoerceToStateMachine(val);

					if (awaitedSM->State == FULFILLED) {
						EnqueueTask(interpreter, fnOrSm);
					} else {
						StateMachineAddWaitList(awaitedSM, fnOrSm);
					}

					Push(interpreter, fnOrSm);
					return;
				}
			case OP_GET_AWAITED_VALUE:
				{
					StateMachine* wait = CoerceToStateMachine(sm->WaitFor);
					if (wait->Value == NULL)
						Panic("Invalid state machine: WaitFor "
							  "is NULL");
					Push(interpreter, wait->Value);
					break;
				}
			case OP_MUL:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoMul(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_DIV:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoDiv(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_MOD:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoMod(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_POSTINC:
				{
					// bot [obj, key, val] top
					lhs = Popp(interpreter);  // old value
					res = DoInc(interpreter, lhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					Push(interpreter, lhs);
					break;
				}
			case OP_INC:
				{
					rhs = Popp(interpreter);
					res = DoInc(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_ADD:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoAdd(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_POSTDEC:
				{
					// bot [obj, key, val] top
					lhs = Popp(interpreter);  // old value
					res = DoDec(interpreter, lhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					Push(interpreter, lhs);
					break;
				}
			case OP_DEC:
				{
					rhs = Popp(interpreter);
					res = DoDec(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_SUB:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoSub(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_LSHFT:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoLShift(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_RSHFT:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoRShift(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_LT:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoLT(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_LTE:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoLTE(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_GT:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoGT(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_GTE:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoGTE(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_EQ:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoEQ(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_NE:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoNE(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_AND:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoAnd(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_OR:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoOr(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_XOR:
				{
					rhs = Popp(interpreter);
					lhs = Popp(interpreter);
					res = DoXor(interpreter, lhs, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					Push(interpreter, res);
					break;
				}
			case OP_STORE_CAPTURE:
				{
					offset = ReadInt32(uf->Codes, ip);
					SetCap(uf, offset, Popp(interpreter));
					Forward(4);
					break;
				}
			case OP_STORE_NAME:
				{
					offset = ReadInt32(uf->Codes, ip);
					SetVar(interpreter->RootEnv, offset, Popp(interpreter));
					Forward(4);
					break;
				}
			case OP_STORE_LOCAL:
				{
					offset = ReadInt32(uf->Codes, ip);
					SetVar(interpreter->CallEnv, offset, Popp(interpreter));
					Forward(4);
					break;
				}
			case OP_LOCK_VAR:
				{
					offset = ReadInt32(uf->Codes, ip);
					LockVar(interpreter->CallEnv, offset);
					Forward(4);
					break;
				}
			case OP_DUPTOP:
				{
					Push(interpreter, Peek(interpreter));
					break;
				}
			case OP_DUP2:
				{
					Value* a = PeekAt(interpreter, 1);
					Value* b = PeekAt(interpreter, 2);
					Push(interpreter, b);
					Push(interpreter, a);
					break;
				}
			case OP_POPTOP:
				{
					Popp(interpreter);
					break;
				}
			case OP_ROT2:
				{
					// A B -> B A
					Value* a = PeekAt(interpreter, 1);
					Value* b = PeekAt(interpreter, 2);
					interpreter->Stacks[interpreter->StckC - 1] = b;
					interpreter->Stacks[interpreter->StckC - 2] = a;
					break;
				}
			case OP_ROT3:
				{
					// A B C -> C A B
					Value* a = PeekAt(interpreter, 1);
					Value* b = PeekAt(interpreter, 2);
					Value* c = PeekAt(interpreter, 3);
					interpreter->Stacks[interpreter->StckC - 1] = c;
					interpreter->Stacks[interpreter->StckC - 2] = a;
					interpreter->Stacks[interpreter->StckC - 3] = b;
					break;
				}
			case OP_ROT4:
				{
					// A B C D -> D A B C
					Value* d = PeekAt(interpreter, 1);
					Value* c = PeekAt(interpreter, 2);
					Value* b = PeekAt(interpreter, 3);
					Value* a = PeekAt(interpreter, 4);
					interpreter->Stacks[interpreter->StckC - 1] = c;
					interpreter->Stacks[interpreter->StckC - 2] = b;
					interpreter->Stacks[interpreter->StckC - 3] = a;
					interpreter->Stacks[interpreter->StckC - 4] = d;
					break;
				}
			case OP_SETUP_TRY:
				{
					SetLocalHandler(true);
					offset = ReadInt32(uf->Codes, ip);
					_PushTry(interpreter, offset, &ip);
					Forward(4);
					break;
				}
			case OP_POP_TRY:
				{
					SetLocalHandler(false);
					_PoppTry(interpreter);
					break;
				}
			case OP_POPN_TRY:
				{
					size = ReadInt32(uf->Codes, ip);
					_PopNTry(interpreter, size);
					Forward(4);
					break;
				}
			case OP_JUMP_IF_FALSE_OR_POP:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = Peek(interpreter);
					if (!CoerceToBool(val)) {
						JmpFrwd(offset);
					} else {
						Popp(interpreter);
						Forward(4);
					}
					break;
				}
			case OP_JUMP_IF_TRUE_OR_POP:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = Peek(interpreter);
					if (CoerceToBool(val)) {
						JmpFrwd(offset);
					} else {
						Popp(interpreter);
						Forward(4);
					}
					break;
				}
			case OP_POP_JUMP_IF_FALSE:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = Popp(interpreter);
					if (CoerceToBool(val) == false) {
						JmpFrwd(offset);
					} else {
						Forward(4);
					}
					break;
				}
			case OP_POP_JUMP_IF_TRUE:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = Popp(interpreter);
					if (CoerceToBool(val) == true) {
						JmpFrwd(offset);
					} else {
						Forward(4);
					}
					break;
				}
			case OP_JUMP:
				{
					offset = ReadInt32(uf->Codes, ip);
					JmpFrwd(offset);
					break;
				}
			case OP_ABSOLUTE_JUMP:
				{
					offset = ReadInt32(uf->Codes, ip);
					JmpFrwd(offset);
					break;
				}
			case OP_RAISE:
				{
					_RaiseError(interpreter, fn, Popp(interpreter), &ip, true);
					break;
				}
			case OP_RETURN:
				{
					if (uf->Async) {
						val = Popp(interpreter);

						StateMachineFulfill(sm, val);
						Push(interpreter, fnOrSm);

						for (int i = 0; i < sm->WaitListC; i++) {
							// Queue all listeners waiting on
							// this state machine to be resumed
							EnqueueTask(interpreter, sm->WaitList[i]);
						}
					}
					interpreter->ActiveFunction = NULL;
					return;
				}
			default:
				{
					InterpreterPanic("Unknown opcode: %s, %d %d\n",
									 uf->Name != NULL ? uf->Name
													  : "<anonymous>",
									 opcode,
									 OP_LOAD_NAME);
					return;
				}
		}
	}
}

void _RunProgram(Interpreter* interpreter, Value* fnValue) {
	UserFunction* uf  = CoerceToUserFunction(fnValue);
	Value *		  env = NULL, *saveGbl = NULL;
	env = saveGbl =
		NewEnvironmentValue(interpreter, CreateEnvironment(NULL, uf->LocalC));
	interpreter->ModulePath = uf->Name;
	SaveRootEnv(interpreter, env);
	Run(interpreter, fnValue);
	RestoreEnv(interpreter);

	int old = interpreter->StckC;

	// Event loop: process pending tasks and mongoose I/O
	// together. Keep spinning while there are queued tasks OR
	// active mongoose connections (which may produce new tasks).
	for (;;) {
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

		// Awaited
		StateMachine* sm = CoerceToStateMachine(task);

		if (!sm->IsCallback) {
			DoCall(interpreter, task, 0, false);
		} else {
			StateMachine* parentSM = CoerceToStateMachine(sm->WaitFor);

			bool isParentRejected =
				ValueIsError(parentSM->Value) || parentSM->State == REJECTED;

			if (isParentRejected && !sm->IsCatched) {
				StateMachineReject(sm, parentSM->Value);
				goto ENQUEUE_TASKS;
			} else if (!isParentRejected && sm->IsCatched) {
				StateMachineFulfill(sm, parentSM->Value);
				goto ENQUEUE_TASKS;
			}

			Push(interpreter, parentSM->Value);

			Value* result = DoCall(interpreter, sm->Function, 1, false);
			if (ValueIsError(result)) {
				StateMachineReject(sm, result);
				goto ENQUEUE_TASKS;
			}

			result = Popp(interpreter);

			if (ValueIsError(result)) {
				StateMachineReject(sm, result);
			} else {
				StateMachineFulfill(sm, result);
			}

		ENQUEUE_TASKS:;
			for (size_t i = 0; i < sm->WaitListC; i++) {
				EnqueueTask(interpreter, sm->WaitList[i]);
			}
		}
	}

	interpreter->StckC	 = old;
	interpreter->RootEnv = saveGbl;

	if (interpreter->StckC != 0)
		PopN(interpreter, interpreter->StckC);

	assert(interpreter->StckC == 0);

	ForceGarbageCollect(interpreter);
}

void Interpret(Interpreter* interpreter, Value* fnValue /*UserFunction*/) {
	_RunProgram(interpreter, fnValue);
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

#undef Push
#undef Popp
#undef PopN
#undef Peek
#undef PeekAt
#undef SetVar
#undef GetVar
#undef SetCap
#undef GetCap
#undef DumpFrame
#undef DumpStack
#undef InterpreterPanic
#undef HandleError
