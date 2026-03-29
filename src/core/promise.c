#include "./promise.h"

Value* _PromiseThen(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc != 2) {
        return NewErrorValue(interpreter, "Promise.then expects 2 arguments");
    }

    Value* thisArg      = arguments[0];
    Value* thenCallback = arguments[1];

    if (!ValueIsPromise(thisArg)) {
        return NewErrorValue(interpreter, "First argument to Promise.then must be a promise");
    }

    if (!ValueIsCallable(thenCallback)) {
        return NewErrorValue(interpreter, "Second argument to Promise.then must be a function");
    }

    int argNeeded = ValueIsNativeFunction(thenCallback) ? CoerceToNativeFunction(thenCallback)->Argc
                                                        : CoerceToUserFunction(thenCallback)->Argc;

    if (argNeeded != 1 && argNeeded != VARARG) {
        return NewErrorValue(
            interpreter,
            "Callback function for Promise.then must take exactly 1 argument (value)");
    }

    StateMachine* originalSM = CoerceToStateMachine(thisArg);

    StateMachine* sm =
        CreateStateMachine(PENDING, true, 0, interpreter->CallEnv, thisArg, thenCallback);
    Value* newPromise = NewPromiseValue(interpreter, sm);

    if (originalSM->State == PENDING) {
        StateMachineAddWaitList(originalSM, newPromise);
    } else {
        int tail = (interpreter->TaskQueueHead + interpreter->TaskQueueC) % STACK_SIZE;
        interpreter->TaskQueue[tail] = newPromise;
        interpreter->TaskQueueC++;
    }

    return newPromise;
}

Value* _PromiseCatch(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc != 2) {
        return NewErrorValue(interpreter, "Promise.catch expects 2 arguments");
    }

    Value* thisArg       = arguments[0];
    Value* catchCallback = arguments[1];

    if (!ValueIsPromise(thisArg)) {
        return NewErrorValue(interpreter, "First argument to Promise.catch must be a promise");
    }

    if (!ValueIsCallable(catchCallback)) {
        return NewErrorValue(interpreter, "Second argument to Promise.catch must be a function");
    }

    int argNeeded = ValueIsNativeFunction(catchCallback)
                        ? CoerceToNativeFunction(catchCallback)->Argc
                        : CoerceToUserFunction(catchCallback)->Argc;

    if (argNeeded != 1 && argNeeded != VARARG) {
        return NewErrorValue(
            interpreter,
            "Callback function for Promise.catch must take exactly 1 argument (error)");
    }

    StateMachine* sm =
        CreateStateMachine(PENDING, true, 0, interpreter->CallEnv, thisArg, catchCallback);
    Value* newPromise = NewPromiseValue(interpreter, sm);

    StateMachineAddWaitList(CoerceToStateMachine(thisArg), newPromise);

    return newPromise;
}

static ModuleFunction _PromiseClassMethods[] = {
    // Promise class
    { .Name      = "then",
      .Argc      = 2,
      .CFunction = (NativeFunctionCallback) _PromiseThen,
      .Value     = NULL },
    { .Name      = "catch",
      .Argc      = 2,
      .CFunction = (NativeFunctionCallback) _PromiseCatch,
      .Value     = NULL },
    // end of module functions
    { .Name = NULL }
};

Value* CreatePromiseClass(Interpreter* interpreter) {
    Value* promiseClass = NewClassValue(interpreter, CreateUserClass("Promise", NULL));
    Class* cls          = CoerceToUserClass(promiseClass);

    for (int i = 0; _PromiseClassMethods[i].Name != NULL; i++) {
        ModuleFunction func = _PromiseClassMethods[i];

        if (func.CFunction != NULL) {
            ClassDefineMemberByString(
                cls,
                func.Name,
                NewNativeFunctionValue(
                    interpreter,
                    CreateNativeFunctionMeta((const String) func.Name, func.Argc, func.CFunction)),
                false);
        }
    }

    return promiseClass;
}

Value* LoadCorePromise(Interpreter* interpreter) {
    Value* val =
        (interpreter->Promise != NULL) ? interpreter->Promise : CreatePromiseClass(interpreter);

    Value*   module = NewObjectValue(interpreter);
    HashMap* map    = CoerceToHashMap(module);

    HashMapSet(map, "Promise", val);
    return module;
}