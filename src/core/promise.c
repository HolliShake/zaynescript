

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

	Promise* parentPromise = CoerceToPromise(thisArg);

	Value* promiseValue = NewPromiseValue(interpreter,
										  CreatePromise(PENDING,
														NULL,
														thisArg,
														parentPromise->Globals,
														thenCallback));

	Promise* promise = CoerceToPromise(promiseValue);

	if (parentPromise->State == PENDING) {
		/* Parent not yet settled: register the new promise as a
		 * fulfill-only reaction.  It will be enqueued when the parent
		 * fulfills.  Rejection will be handled by a .error reaction if
		 * one exists. */
		PromiseFulfillReactionAdd(parentPromise, promiseValue);
	} else if (parentPromise->State == REJECTED) {
		/* .then is skipped for rejected parents.  Propagate the rejection
		 * through the new promise so downstream .error handlers can catch it.
		 * Register the new promise as a reject-reaction of the parent so that
		 * unhandled-rejection tracking treats the parent as "handled" — the
		 * chain continuation (P_then) will be checked instead. */
		PromiseRejectReactionAdd(parentPromise, promiseValue);
		PromiseReject(promise, parentPromise->Result);
		/* P_then's own RejectReactions are empty now; .error() called next in
		 * the chain will enqueue its handler directly on the settled P_then. */
	} else {
		/* Parent already fulfilled: enqueue only this new promise. */
		EnqueueTask(interpreter, promiseValue);
	}

	return promiseValue;
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

	Promise* parentPromise = CoerceToPromise(thisArg);

	Value* promiseValue = NewPromiseValue(interpreter,
										  CreatePromise(PENDING,
														NULL,
														thisArg,
														parentPromise->Globals,
														catchCallback));

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
		PromiseFulfill(promise, parentPromise->Result);
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