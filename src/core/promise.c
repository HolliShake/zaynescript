#include "./promise.h"


/**
 * @brief Appends task to the ring buffer TaskQueue[(head + count) % STACK_SIZE]
 * and increments TaskQueueC.
 * @param interpreter VM whose bounded queue triggers InterpreterPanic if
 * TaskQueueC already equals STACK_SIZE.
 * @param task Promise/task Value retained by the queue until the event loop
 * dequeues it.
 * @origin src/interpreter.c
 */
extern void EnqueueTask(Interpreter* interpreter, Value* task);

static Value*
_PromiseThen(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: Promise.then expects 2 arguments",
							  ARGUMENT_ERROR);
	}

	Value* thisArg		= arguments[0];
	Value* thenCallback = arguments[1];

	if (!ValueIsPromise(thisArg)) {
		return NewErrorFValue(
			interpreter,
			"%s: first argument to Promise.then must be a promise",
			TYPE_ERROR);
	}

	if (!ValueIsCallable(thenCallback)) {
		return NewErrorFValue(
			interpreter,
			"%s: second argument to Promise.then must be a function",
			TYPE_ERROR);
	}

	int argNeeded = ValueIsNativeFunction(thenCallback)
						? CoerceToNativeFunction(thenCallback)->Argc
						: CoerceToUserFunction(thenCallback)->Argc;

	if (argNeeded != 1 && argNeeded != VARARG) {
		return NewErrorFValue(
			interpreter,
			"%s: callback function for Promise.then must take exactly 1 "
			"argument (value)",
			ARGUMENT_ERROR);
	}

	StateMachine* originalSM = CoerceToStateMachine(thisArg);

	StateMachine* sm = CreateStateMachine(PENDING,
										  true,
										  0,
										  interpreter->CallEnv,
										  thisArg,
										  thenCallback);

	sm->Line = originalSM->Line;

	Value* newPromise = NewPromiseValue(interpreter, sm);

	if (originalSM->State == PENDING) {
		StateMachineAddWaitList(originalSM, newPromise);
	} else {
		EnqueueTask(interpreter, newPromise);
	}

	return newPromise;
}

static Value*
_PromiseError(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: Promise.error expects 2 arguments",
							  ARGUMENT_ERROR);
	}

	Value* thisArg		 = arguments[0];
	Value* catchCallback = arguments[1];

	if (!ValueIsPromise(thisArg)) {
		return NewErrorFValue(
			interpreter,
			"%s: first argument to Promise.error must be a promise",
			TYPE_ERROR);
	}

	if (!ValueIsCallable(catchCallback)) {
		return NewErrorFValue(
			interpreter,
			"%s: second argument to Promise.error must be a function",
			TYPE_ERROR);
	}

	int argNeeded = ValueIsNativeFunction(catchCallback)
						? CoerceToNativeFunction(catchCallback)->Argc
						: CoerceToUserFunction(catchCallback)->Argc;

	if (argNeeded != 1 && argNeeded != VARARG) {
		return NewErrorFValue(
			interpreter,
			"%s: callback function for Promise.error must take exactly 1 "
			"argument (error)",
			ARGUMENT_ERROR);
	}

	StateMachine* originalSM = CoerceToStateMachine(thisArg);
	originalSM->IsCatched	 = true;

	StateMachine* sm = CreateStateMachine(PENDING,
										  true,
										  0,
										  interpreter->CallEnv,
										  thisArg,
										  catchCallback);

	sm->Line = originalSM->Line;

	Value* newPromise = NewPromiseValue(interpreter, sm);

	if (originalSM->State == PENDING) {
		// Queue the catch callback to run as soon as possible
		StateMachineAddWaitList(originalSM, newPromise);
	} else if (originalSM->State == REJECTED) {
		EnqueueTask(interpreter, newPromise);
	}

	return newPromise;
}

static ModuleFunction _PromiseClassMethods[] = {
	// Promise class
	{ .Name		 = "then",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _PromiseThen,
	  .Value	 = NULL },
	{ .Name		 = "error",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _PromiseError,
	  .Value	 = NULL },
	// end of module functions
	{ .Name = NULL }
};

Value* CreatePromiseClass(Interpreter* interpreter) {
	Value* promiseClass =
		NewClassValue(interpreter,
					  CreateUserClass("Promise", interpreter->Object));
	Class* cls = CoerceToUserClass(promiseClass);

	for (int i = 0; _PromiseClassMethods[i].Name != NULL; i++) {
		ModuleFunction func = _PromiseClassMethods[i];

		if (func.CFunction != NULL) {
			ClassDefineMemberByString(
				cls,
				func.Name,
				NewNativeFunctionValue(
					interpreter,
					CreateNativeFunctionMeta((const String) func.Name,
											 func.Argc,
											 func.CFunction)),
				false);
		}
	}

	return promiseClass;
}

Value* LoadCorePromise(Interpreter* interpreter) {
	Value* val = (interpreter->Promise != NULL)
					 ? interpreter->Promise
					 : (interpreter->Promise = CreatePromiseClass(interpreter));

	Value*	 module = NewObjectValue(interpreter);
	HashMap* map	= CoerceToHashMap(module);

	HashMapSet(map, "Promise", val);
	return module;
}