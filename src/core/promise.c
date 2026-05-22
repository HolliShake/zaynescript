

#include "./promise.h"

#include <sched.h>


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

static Value* _PromiseThen(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: Promise.then expects 2 arguments",
							  ARGUMENT_ERROR);
	}

	Value* thisArg		= arguments[0];
	Value* thenCallback = arguments[1];

	if (!ValueIsPromise(thisArg)) {
		return NewErrorFValue(interpreter,
							  "%s: first argument to Promise.then must be a promise",
							  TYPE_ERROR);
	}

	if (!ValueIsCallable(thenCallback)) {
		return NewErrorFValue(interpreter,
							  "%s: second argument to Promise.then must be a function",
							  TYPE_ERROR);
	}

	/* Validate arity using Argc (the declared parameter count),
	 * then separately compute LocalC for environment allocation. */
	int argcCheck = ValueIsNativeFunction(thenCallback)
						? CoerceToNativeFunction(thenCallback)->Argc
						: CoerceToUserFunction(thenCallback)->Argc;

	if (argcCheck != 1 && argcCheck != VARARG) {
		return NewErrorFValue(
			interpreter,
			"%s: callback for Promise.then must take exactly 1 argument (value)",
			ARGUMENT_ERROR);
	}

	/* LocalC covers all locals including the argument slot.
	 * Use it for the environment so every local has a cell. */
	int envSize = ValueIsUserFunction(thenCallback)
					  ? CoerceToUserFunction(thenCallback)->LocalC
					  : 1;

	Promise* parentPromise = CoerceToPromise(thisArg);

	/* Build a fresh call frame for the callback.  The resolved
	 * value is injected into local slot 0 by the event loop
	 * just before Run() is called — not here. */
	CallFrame* callbackFrame = InitCallFrame(
		NULL,
		parentPromise->Globals,
		NewEnvironmentValue(interpreter, CreateEnvironment(NULL, envSize)),
		thenCallback,
		false);

	Value* promiseValue =
		NewPromiseValue(interpreter, CreatePromise(PENDING, callbackFrame));
	Promise* promise = CoerceToPromise(promiseValue);
	promise->Globals = parentPromise->Globals;

	if (parentPromise->State == PENDING) {
		/* Parent not yet settled — register as a fulfill reaction.
		 * PromiseFulfill will enqueue this child when it fires. */
		PromiseFulfillReactionAdd(parentPromise, promiseValue);

	} else if (parentPromise->State == REJECTED) {
		/* .then() on a rejected promise is skipped (no callback).
		 * Propagate the rejection through the chain asynchronously
		 * so downstream .error() handlers can catch it.
		 *
		 * FIX: was calling PromiseReject() synchronously which
		 * caused out-of-order execution vs other queued tasks. */
		PromiseRejectReactionAdd(parentPromise, promiseValue);
		promise->Result = parentPromise->Result;
		promise->State	= REJECTED;
		EnqueueTask(interpreter, promiseValue); /* async, not synchronous */

	} else {
		/* Parent already fulfilled — copy result now so the event
		 * loop can pass it to the callback.
		 *
		 * FIX: was enqueuing without setting Result, so the
		 * callback received NULL. */
		promise->Result = parentPromise->Result;
		EnqueueTask(interpreter, promiseValue);
	}

	return promiseValue; /* enables .then().then() chaining */
}

static Value* _PromiseError(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: Promise.error expects 2 arguments",
							  ARGUMENT_ERROR);
	}

	Value* thisArg		 = arguments[0];
	Value* catchCallback = arguments[1];

	if (!ValueIsPromise(thisArg)) {
		return NewErrorFValue(interpreter,
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

	int locals = argNeeded;
	if (ValueIsUserFunction(catchCallback)) {
		locals = CoerceToUserFunction(catchCallback)->LocalC;
	}

	Promise* parentPromise = CoerceToPromise(thisArg);

	CallFrame* callbackFrame = InitCallFrame(
		NULL,
		parentPromise->Globals,
		NewEnvironmentValue(interpreter, CreateEnvironment(NULL, locals)),
		catchCallback,
		false);

	Value* promiseValue =
		NewPromiseValue(interpreter, CreatePromise(PENDING, callbackFrame));

	Promise* promise = CoerceToPromise(promiseValue);

	if (parentPromise->State == PENDING) {
		/* Parent not yet settled: register as a reject reaction only.
		 * If the parent fulfills it will be skipped; if rejected it
		 * will fire. */
		PromiseRejectReactionAdd(parentPromise, promiseValue);
	} else if (parentPromise->State == REJECTED) {
		/* Parent already rejected: enqueue only this new promise. */
		EnqueueTask(interpreter, promiseValue);
	} else {
		/* .error is skipped for fulfilled parents.  Propagate the
		 * fulfilled value so downstream .then handlers can receive it. */
		PromiseFulfill(interpreter, promise, parentPromise->Result);
	}

	return promiseValue;
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
		NewClassValue(interpreter, CreateUserClass("Promise", interpreter->Object));
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