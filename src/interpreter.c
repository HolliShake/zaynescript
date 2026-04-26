
#include "./interpreter.h"

#include "global.h"

#include <sched.h>
#include <stdbool.h>

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
	interpreter->CurrentFrame  = NULL;
	interpreter->ActiveTask	   = NULL;
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
			FPush(interpreter,                                                 \
				  interpreter->CurrentFrame,                                   \
				  NewErrorValue(interpreter, message));                        \
			free(message);                                                     \
			break;                                                             \
		}                                                                      \
		InterpreterPanic(message);                                             \
		free(message);                                                         \
		break;                                                                 \
	}

void SetActiveTask(Interpreter* interpreter, Value* task) {
	interpreter->ActiveTask = task;
}

void SetCurrentFrame(Interpreter* interpreter, CallFrame* frame) {
	interpreter->CurrentFrame = frame;
}

void FPush(Interpreter* interpreter, CallFrame* frame, Value* value) {
	frame->Operand[frame->OperandC++] = value;
}

Value* FPopp(Interpreter* interpreter, CallFrame* frame) {
	return frame->Operand[--frame->OperandC];
}

void FPopN(Interpreter* interpreter, CallFrame* frame, int n) {
	frame->OperandC -= n;
	frame->Operand[frame->OperandC];
}

Value* FPeek(Interpreter* interpreter, CallFrame* frame) {
	return frame->Operand[frame->OperandC - 1];
}

Value* FPeekAt(Interpreter* interpreter, CallFrame* frame, int n) {
	return frame->Operand[(frame->OperandC) - (n)];
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
	return (LineInfo){
		.Path = NULL,
		.Line = -1,
		.Pc	  = -1,
	};
}

static void _RaiseError(Interpreter* interpreter,
						Value*		 fn,
						Value*		 error,
						size_t*		 ip,
						bool		 catchable) {
	UserFunction* uf = CoerceToUserFunction(fn);
	if (interpreter->ActiveTask != NULL
		&& ValueIsPromise(interpreter->ActiveTask) && catchable) {
		StateMachine* activeTask =
			CoerceToStateMachine(interpreter->ActiveTask);
		StateMachineReject(activeTask, error);
		FPush(interpreter, interpreter->CurrentFrame, interpreter->ActiveTask);
		JumpToError(ip, uf->CodeC);
		return;
	}

	if (isCatched() && catchable) {
		ExceptionHandler handler = _PeekTry(interpreter);
		CallFrame*		 target	 = interpreter->CurrentFrame;
		while (target != NULL && &target->Ip != handler.PausedAddress) {
			target = target->Parent;
		}
		if (target == NULL) {
			target = interpreter->CurrentFrame;
		}
		/* Caught: preserve the original error value as-is for
		 * the catch handler
		 */
		// Jump the current function to end
		JumpToError(ip, uf->CodeC);
		// Jump the main function to the handle
		JumpToError(handler.PausedAddress, handler.JumpAddress);
		FPush(interpreter, target, error);
		return;
	}
	/* Uncaught: format for display only, no new error Value is
	 * created */
	LineInfo line	= _GetLineFromPc(uf, *ip);
	String	 errStr = ValueToString(error);
	String	 path	= ValueToString(line.Path);
	String	 buf	= FormatString("[%s:%d]::%s\n", path, line.Line, errStr);
	free(path);
	free(errStr);

	for (int i = interpreter->CallStackC - 1; i >= 0; i--) {
		StackTrace trace = interpreter->CallStack[i];
		String	   path	 = ValueToString(trace.line.Path);
		String frame = FormatString("  |> [%s:%d]\n", path, trace.line.Line);
		String tmp	 = FormatString("%s%s", buf, frame);
		free(path);
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

void RetainFrame(Interpreter* interpreter, CallFrame* frame) {
	if (frame != NULL) {
		frame->RefCount++;
	}
}

void ReleaseFrame(Interpreter* interpreter, CallFrame* frame) {
	if (frame != NULL) {
		frame->RefCount--;

		if (frame->RefCount <= 0) {
			// Free any internal arrays if you have them (like Slots)
			// free(frame->Slots);

			// Free the actual frame
			free(frame);
		}
	}
}

void MarkCallFrame(CallFrame* frame) {
	CallFrame* current = frame;
	while (current != NULL) {
		Mark(current->Fn);
		Mark(current->Env);
		Mark(current->GlobalEnv);
		for (int i = 0; i < current->OperandC; i++) {
			Mark(current->Operand[i]);
		}
		current = current->Parent;
	}
}

void Run(Interpreter* interpreter, CallFrame* frame, Value* promise) {
	Value*		  fn			 = frame->Fn;
	StateMachine* sm			 = NULL;
	UserFunction* uf			 = CoerceToUserFunction(frame->Fn);
	uint8_t		  opcode		 = 0;
	Value*		  lhs			 = NULL;
	Value*		  rhs			 = NULL;
	Value*		  res			 = NULL;
	Value*		  ext			 = NULL;
	Value*		  arr			 = NULL;
	Value*		  obj			 = NULL;
	Value*		  cls			 = NULL;
	Value*		  key			 = NULL;
	Value*		  val			 = NULL;
	Value*		  err			 = NULL;
	Environment*  env			 = NULL;
	HashMap*	  map			 = NULL;
	Array*		  array			 = NULL;
	int			  offset		 = 0;
	int			  argc			 = 0;
	int			  flg			 = 0;
	int			  size			 = 0;
	bool		  catched		 = false;
	bool		  localhandler	 = false;
	bool		  ownsActiveTask = false;
	String		  str			 = NULL;
	Value*		  prevActiveTask = interpreter->ActiveTask;

	if (uf->Async && promise == NULL) {
		sm			  = CreateStateMachine(PENDING, false, NULL, frame->Fn);
		sm->Frame	  = frame;
		sm->GlobalEnv = frame->GlobalEnv;
		promise		  = NewPromiseValue(interpreter, sm);
	} else {
		sm = CoerceToStateMachine(promise);
	}

	if (uf->Async && promise != NULL && interpreter->ActiveTask == NULL) {
		SetActiveTask(interpreter, promise);
		ownsActiveTask = true;
	}

	SetCurrentFrame(interpreter, frame);

	if (uf == NULL)
		InterpreterPanic("Attempted to run a non-function value of type %s",
						 ValueTypeOf(frame->Fn));

#define ip					   frame->Ip
#define Forward(size)		   (frame->Ip += size)
#define JmpFrwd(addr)		   (frame->Ip = addr)
#define SetLocalHandler(value) (localhandler = value)

	while (frame->Ip != uf->CodeC) {
		if (interpreter->Allocated >= interpreter->GcThreshold) {
			MarkCallFrame(frame);
			GarbageCollect(interpreter);
		}

		opcode	= uf->Codes[frame->Ip++];
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
					FPush(interpreter, frame, res);
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
					FPush(interpreter, frame, res);
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
					FPush(interpreter, frame, res);
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
					FPush(interpreter, frame, val);
					Forward(4);
					break;
				}
			case OP_LOAD_NAME:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = GetVar(frame->GlobalEnv, offset);
					if (val == NULL) {
						err = _CreateError(
							interpreter,
							REFERENCE_ERROR,
							"variable is referenced before initialization");
						// Raise
						_RaiseError(interpreter, fn, err, &ip, false);
						break;
					}
					FPush(interpreter, frame, val);
					Forward(4);
					break;
				}
			case OP_LOAD_LOCAL:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = GetVar(frame->Env, offset);
					if (val == NULL) {
						err = _CreateError(
							interpreter,
							REFERENCE_ERROR,
							"variable is referenced before initialization");
						// Raise
						_RaiseError(interpreter, fn, err, &ip, false);
						break;
					}
					FPush(interpreter, frame, val);
					Forward(4);
					break;
				}
			case OP_LOAD_CONST:
				{
					offset = ReadInt32(uf->Codes, ip);
					FPush(interpreter, frame, interpreter->Constants[offset]);
					Forward(4);
					break;
				}
			case OP_LOAD_INT:
				{
					offset = ReadInt32(uf->Codes, ip);
					FPush(interpreter, frame, NewIntValue(interpreter, offset));
					Forward(4);
					break;
				}
			case OP_LOAD_BOOL:
				{
					offset = ReadInt32(uf->Codes, ip);
					FPush(interpreter,
						  frame,
						  offset ? interpreter->True : interpreter->False);
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
					str = ReadString(uf->Codes, ip);
					FPush(interpreter, frame, NewStrValue(interpreter, str));
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_ARRAY_EXTEND:
				{
					ext = FPopp(interpreter, frame);
					arr = FPeek(interpreter, frame);
					if (!ValueIsArray(ext)) {
						// Pop the array that was extended
						FPopp(interpreter, frame);
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
					val = FPopp(interpreter, frame);
					arr = FPeek(interpreter, frame);
					if (!ValueIsArray(arr)) {
						// Pop the array that was pushed
						FPopp(interpreter, frame);
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
						val = FPeekAt(interpreter, frame, size - i);
						ArrayPush(array, val);
					}
					FPopN(interpreter, frame, size);
					FPush(interpreter, frame, arr);
					Forward(4);
					break;
				}
			case OP_OBJECT_EXTEND:
				{
					ext = FPopp(interpreter, frame);
					obj = FPeek(interpreter, frame);
					if (!ValueIsObject(ext)) {
						// Pop the object that was extended
						FPopp(interpreter, frame);
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
						key			  = FPopp(interpreter, frame);
						val			  = FPopp(interpreter, frame);
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
					str = ReadString(uf->Codes, ip);
					obj = FPeek(interpreter, frame);
					map = CoerceToHashMap(obj);
					val = HashMapGet(map, str);
					FPush(interpreter,
						  frame,
						  (val == NULL) ? interpreter->Null : val);
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_CLASS_EXTEND:
				{
					ext = FPopp(interpreter, frame);  // super class
					cls = FPeek(interpreter, frame);  // class being extended
					if (!ValueIsClass(ext)) {
						// Pop the class that was extended
						FPopp(interpreter, frame);
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
					FPush(interpreter, frame, obj);
					Forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_CLASS_DEFINE_STATIC_MEMBER:
			case OP_CLASS_DEFINE_INSTANCE_MEMBER:
				{
					key = FPopp(interpreter, frame);
					val = FPopp(interpreter, frame);
					obj = FPeek(interpreter, frame);
					ClassDefineMember(
						CoerceToUserClass(obj),
						key,
						val,
						(opcode == OP_CLASS_DEFINE_STATIC_MEMBER));
					break;
				}
			case OP_CLASS_GETBASE:
				{
					obj = FPopp(interpreter, frame);
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
					val = FPopp(interpreter, frame);
					key = FPopp(interpreter, frame);
					obj = FPeek(interpreter, frame);
					res = DoSetIndex(interpreter, obj, key, val);
					if (ValueIsError(res)) {
						// Pop the object
						FPopp(interpreter, frame);
						//  Pop the duplicated value
						FPopp(interpreter, frame);
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					break;
				}
			case OP_GET_INDEX:
				{
					key = FPopp(interpreter, frame);
					obj = FPopp(interpreter, frame);
					res = DoGetIndex(interpreter, obj, key);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_LOAD_FUNCTION_CLOSURE:
			case OP_LOAD_FUNCTION:
				{
					offset = ReadInt32(uf->Codes, ip);
					res = DoLoadFunction(interpreter,
										 frame,
										 offset,
										 (opcode == OP_LOAD_FUNCTION_CLOSURE));
					FPush(interpreter, frame, res);
					Forward(4);
					break;
				}
			case OP_CALL_CTOR:
				{
					argc = ReadInt32(uf->Codes, ip);
					Forward(4);
					cls = FPopp(interpreter, frame);
					res = DoCallCtor(interpreter, frame, cls, argc);
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
					obj = FPopp(interpreter, frame);
					res = DoCall(interpreter, frame, obj, argc, false);
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
					key = FPopp(interpreter, frame);		  // method name
					obj = FPeekAt(interpreter, frame, argc);  // 'this' object
					res = DoCallMethod(interpreter, frame, obj, key, argc);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					break;
				}
			case OP_NOT:
				{
					rhs = FPopp(interpreter, frame);
					res = DoNot(interpreter, rhs);
					if (ValueIsError(res)) {
						// Raise
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_AWAIT:
				{
					val = FPeek(interpreter, frame);
					if (!ValueIsPromise(val))
						Panic("Expected a promise got %s", ValueTypeOf(val));

					RetainFrame(interpreter, frame);

					val						= FPopp(interpreter, frame);
					StateMachine* awaitedSM = CoerceToStateMachine(val);

					if (awaitedSM->State == FULFILLED) {
						EnqueueTask(interpreter, promise);
					} else {
						StateMachineAddWaitList(awaitedSM, promise);
					}

					// Wait
					StateMachineAwait(sm, val);
					sm->Line = _GetLineFromPc(uf, ip);

					// Push the promise
					FPush(interpreter, frame->Parent, promise);
					if (ownsActiveTask) {
						SetActiveTask(interpreter, prevActiveTask);
					}
					return;
				}
			case OP_GET_AWAITED_VALUE:
				{
					if (sm == NULL) {
						Panic("Invalid state machine: not currently in an "
							  "async function");
						break;
					}
					StateMachine* wait = CoerceToStateMachine(sm->WaitFor);
					if (wait->Value == NULL)
						Panic("Invalid state machine: WaitFor "
							  "is NULL");
					FPush(interpreter, frame, wait->Value);
					break;
				}
			case OP_GETTYPE:
				{
					val = FPopp(interpreter, frame);
					res = DoGetType(interpreter, val);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
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
						_RaiseError(interpreter, fn, res, &ip, true);
						break;
					}
					FPush(interpreter, frame, res);
					break;
				}
			case OP_STORE_CAPTURE:
				{
					offset = ReadInt32(uf->Codes, ip);
					SetCap(uf, offset, FPopp(interpreter, frame));
					Forward(4);
					break;
				}
			case OP_STORE_NAME:
				{
					offset = ReadInt32(uf->Codes, ip);
					SetVar(frame->GlobalEnv, offset, FPopp(interpreter, frame));
					Forward(4);
					break;
				}
			case OP_STORE_LOCAL:
				{
					offset = ReadInt32(uf->Codes, ip);
					SetVar(frame->Env, offset, FPopp(interpreter, frame));
					Forward(4);
					break;
				}
			case OP_LOCK_VAR:
				{
					offset = ReadInt32(uf->Codes, ip);
					LockVar(frame->Env, offset);
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
					val	   = FPeek(interpreter, frame);
					if (!CoerceToBool(val)) {
						JmpFrwd(offset);
					} else {
						FPopp(interpreter, frame);
						Forward(4);
					}
					break;
				}
			case OP_JUMP_IF_TRUE_OR_POP:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = FPeek(interpreter, frame);
					if (CoerceToBool(val)) {
						JmpFrwd(offset);
					} else {
						FPopp(interpreter, frame);
						Forward(4);
					}
					break;
				}
			case OP_POP_JUMP_IF_FALSE:
				{
					offset = ReadInt32(uf->Codes, ip);
					val	   = FPopp(interpreter, frame);
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
					val	   = FPopp(interpreter, frame);
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
					_RaiseError(interpreter,
								fn,
								FPopp(interpreter, frame),
								&ip,
								true);
					break;
				}
			case OP_RETURN:
				{
					val					   = FPopp(interpreter, frame);
					CallFrame* parentFrame = frame->Parent;
					CallFrame* deadFrame   = frame;

					if (uf->Async) {
						// Return resolved promise
						if (parentFrame != NULL) {
							FPush(interpreter, parentFrame, promise);
						}

						StateMachineFulfill(sm, val);
						sm->Frame = NULL;

						for (int i = 0; i < sm->WaitListC; i++) {
							// Queue all listeners waiting on
							// this state machine to be resumed
							EnqueueTask(interpreter, sm->WaitList[i]);
						}
					} else {
						// Synchronous call
						if (parentFrame != NULL) {
							FPush(interpreter, parentFrame, val);
						} else if (promise != NULL) {
							// Promise callback running without a caller frame.
							StateMachineFulfill(sm, val);
						} else {
							// Top-level/module execution path: preserve the
							// return value on this frame for callers that run
							// Run() directly.
							FPush(interpreter, frame, val);
						}
					}

					SetCurrentFrame(interpreter, parentFrame);
					if (ownsActiveTask) {
						SetActiveTask(interpreter, prevActiveTask);
					}
					return;
				}
			default:
				{
					InterpreterPanic("Unknown opcode: %s, %d %d\n",
									 uf->Name != NULL ? uf->Name
													  : "<anonymous>",
									 opcode,
									 OP_LOAD_NAME);
					if (ownsActiveTask) {
						SetActiveTask(interpreter, prevActiveTask);
					}
					return;
				}
		}
	}

	SetCurrentFrame(interpreter, frame->Parent);
	CallFrame* parentFrame = frame->Parent;

	// Handle implicit fallthrough when execution reaches uf->CodeC
	// without hitting OP_RETURN (e.g. jump-to-end from errors).
	if (uf->Async) {
		if (parentFrame != NULL) {
			FPush(interpreter, parentFrame, promise);
		}
		if (sm->State == PENDING) {
			StateMachineFulfill(sm, interpreter->Null);
		}
		sm->Frame = NULL;
		for (int i = 0; i < sm->WaitListC; i++) {
			EnqueueTask(interpreter, sm->WaitList[i]);
		}
	} else if (parentFrame != NULL) {
		FPush(interpreter, parentFrame, interpreter->Null);
	} else if (promise != NULL && sm->State == PENDING) {
		StateMachineFulfill(sm, interpreter->Null);
	} else if (parentFrame == NULL) {
		FPush(interpreter, frame, interpreter->Null);
	}

	if (ownsActiveTask) {
		SetActiveTask(interpreter, prevActiveTask);
	}
}

void _RunProgram(Interpreter* interpreter, Value* fn) {
	UserFunction* uf  = CoerceToUserFunction(fn);
	Value*		  env = NULL;
	env = NewEnvironmentValue(interpreter, CreateEnvironment(NULL, uf->LocalC));

	interpreter->ModulePath = uf->Name;

	CallFrame* gFrame = Allocate(sizeof(CallFrame));
	InitCallFrame(gFrame, NULL, env, env, fn);

	Run(interpreter, gFrame, NULL);

	// Event loop: process pending tasks and mongoose I/O
	// together. Keep spinning while there are queued tasks OR
	// active mongoose connections (which may produce new tasks).
	for (;;) {
		// Native event callbacks (e.g. mongoose) use CurrentFrame
		// for temporary argument passing and function dispatch.
		// Keep the root frame active while polling the event loop.
		SetCurrentFrame(interpreter, gFrame);

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

		SetActiveTask(interpreter, task);

		// Awaited
		StateMachine* sm	= CoerceToStateMachine(task);
		CallFrame*	  frame = sm->Frame;

		if (!sm->IsCallback) {
			// Continue
			Run(interpreter, frame, task);
			if (sm->State != PENDING) {
				sm->Frame = NULL;
			}
			ReleaseFrame(interpreter, frame);
		} else {
			// Callback
			StateMachine* parentSM	  = CoerceToStateMachine(sm->WaitFor);
			CallFrame*	  parentFrame = parentSM->Frame;

			if (parentSM->Value == NULL && parentSM->State == PENDING) {
				// Parent promise not settled yet; try again on a later tick.
				EnqueueTask(interpreter, task);
				SetActiveTask(interpreter, NULL);
				continue;
			}

			bool isParentRejected =
				parentSM->State == REJECTED
				|| (parentSM->Value != NULL && ValueIsError(parentSM->Value));

			if (isParentRejected && !sm->IsCatched) {
				StateMachineReject(sm, parentSM->Value);
				goto ENQUEUE_TASKS;
			} else if (!isParentRejected && sm->IsCatched) {
				StateMachineFulfill(sm, parentSM->Value);
				goto ENQUEUE_TASKS;
			}

			if (ValueIsNativeFunction(sm->Callback)) {
				Value*			arg		= parentSM->Value;
				Value*			args[1] = { arg };
				NativeFunction* nFMeta	= CoerceToNativeFunction(sm->Callback);
				Value* nativeResult		= nFMeta->FuncPtr(interpreter, 1, args);

				if (ValueIsError(nativeResult)) {
					StateMachineReject(sm, nativeResult);
				} else {
					StateMachineFulfill(sm, nativeResult);
				}
				goto ENQUEUE_TASKS;
			}

			if (frame == NULL) {
				UserFunction* cb = CoerceToUserFunction(sm->Callback);

				frame = Allocate(sizeof(CallFrame));
				InitCallFrame(frame,
							  NULL,
							  parentSM->GlobalEnv,
							  NewEnvironmentValue(
								  interpreter,
								  CreateEnvironment(cb->Scope, cb->LocalC)),
							  sm->Callback);
				sm->Frame = frame;
			}

			FPush(interpreter, frame, parentSM->Value);

			// Run the callback
			Run(interpreter, frame, task);

			Value* result = sm->Value;
			if (result == NULL) {
				result = interpreter->Null;
			}

			if (ValueIsError(result)) {
				StateMachineReject(sm, result);
			} else {
				StateMachineFulfill(sm, result);
			}

			sm->Frame = NULL;
			ReleaseFrame(interpreter, frame);

		ENQUEUE_TASKS:;
			for (size_t i = 0; i < sm->WaitListC; i++) {
				EnqueueTask(interpreter, sm->WaitList[i]);
			}
		}

		SetActiveTask(interpreter, NULL);
	}

	ReleaseFrame(interpreter, gFrame);

	ForceGarbageCollect(interpreter);
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

#undef SetVar
#undef GetVar
#undef SetCap
#undef GetCap
#undef DumpFrame
#undef DumpStack
#undef InterpreterPanic
#undef HandleError
