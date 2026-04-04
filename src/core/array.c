#include "./array.h"

/**
 * @brief Pushes a value onto the interpreter's stack.
 * @param interpreter The interpreter instance.
 * @param value The value to push.
 * @origin src/interpreter.c:103
 */
extern void Push(Interpreter* interpreter, Value* value);

/**
 * @brief Pops and returns the top value from the interpreter's
 * stack.
 * @param interpreter The interpreter instance.
 * @return The popped value.
 * @origin src/interpreter.c:107
 */
extern Value* Popp(Interpreter* interpreter);

/**
 * @brief Peeks at the top value on the interpreter's stack
 * without removing it.
 * @param interpreter The interpreter instance.
 * @return The top value on the stack.
 * @origin src/interpreter.c:115
 */
extern Value* Peek(Interpreter* interpreter);

/**
 * @brief Calls a function value with the given arguments.
 * @param interp The interpreter instance.
 * @param fn The function value to call.
 * @param argc The number of arguments.
 * @param withThis Whether the call includes a 'this' context.
 * @return The return value of the function call.
 * @origin src/operation.c:717
 */
extern Value* DoCall(Interpreter* interp, Value* fn, int argc, bool withThis);

Value* _ArrayEach(Interpreter* interpreter, int argc, Value** arguments) {
	// Reserve 1 arg for thisArg"
	if (argc != 2) {
		return NewErrorValue(interpreter, "Array.each expects 1 argument");
	}

	Value* thisArg	= arguments[0];
	Value* callback = arguments[1];

	if (!ValueIsArray(thisArg)) {
		return NewErrorValue(interpreter,
							 "First argument to Array.each must be an array");
	}

	if (!ValueIsCallable(callback)) {
		return NewErrorValue(
			interpreter,
			"Second argument to Array.each must be a function");
	}

	int argNeeded = ValueIsNativeFunction(callback)
						? CoerceToNativeFunction(callback)->Argc
						: CoerceToUserFunction(callback)->Argc;

	if (argNeeded != 2 && argNeeded != VARARG) {
		return NewErrorValue(interpreter,
							 "Callback function for Array.each must take "
							 "exactly 2 arguments (item, index)");
	}

	Array* array = CoerceToArray(thisArg);

	Value* arrayVal = NewArrayValue(interpreter);
	Array* newArray = CoerceToArray(arrayVal);

	for (size_t i = 0; i < ArrayLength(array); i++) {
		Value* item	 = ArrayGet(array, i);
		Value* index = NewNumValue(interpreter, (int) i);
		Push(interpreter, index);
		Push(interpreter, item);
		DoCall(interpreter, callback, argc, false);
		ArrayPush(newArray, Popp(interpreter));
	}

	return arrayVal;
}

Value* _ArrayKeep(Interpreter* interpreter, int argc, Value** arguments) {
	// Reserve 1 arg for thisArg"
	if (argc != 2) {
		return NewErrorValue(interpreter, "Array.keep expects 1 argument");
	}

	Value* thisArg	= arguments[0];
	Value* callback = arguments[1];

	if (!ValueIsArray(thisArg)) {
		return NewErrorValue(interpreter,
							 "First argument to Array.keep must be an array");
	}

	if (!ValueIsCallable(callback)) {
		return NewErrorValue(
			interpreter,
			"Second argument to Array.keep must be a function");
	}

	int argNeeded = ValueIsNativeFunction(callback)
						? CoerceToNativeFunction(callback)->Argc
						: CoerceToUserFunction(callback)->Argc;

	if (argNeeded != 2 && argNeeded != VARARG) {
		return NewErrorValue(interpreter,
							 "Callback function for Array.keep must take "
							 "exactly 2 arguments (item, index)");
	}

	Array* array = CoerceToArray(thisArg);

	Value* arrayVal = NewArrayValue(interpreter);
	Array* newArray = CoerceToArray(arrayVal);

	for (size_t i = 0; i < ArrayLength(array); i++) {
		Value* item	 = ArrayGet(array, i);
		Value* index = NewNumValue(interpreter, (int) i);
		Push(interpreter, index);
		Push(interpreter, item);
		DoCall(interpreter, callback, argc, false);
		if (CoerceToBool(Popp(interpreter))) {
			ArrayPush(newArray, item);
		}
	}

	return arrayVal;
}

Value* _ArrayPush(Interpreter* interpreter, int argc, Value** arguments) {
	// Reserve 1 arg for thisArg"
	if (argc != 2) {
		return NewErrorValue(interpreter, "Array.push expects 1 argument");
	}

	Value* thisArg	  = arguments[0];
	Value* itemToPush = arguments[1];

	if (!ValueIsArray(thisArg)) {
		return NewErrorValue(interpreter,
							 "First argument to Array.push must be an array");
	}

	Array* array = CoerceToArray(thisArg);
	ArrayPush(array, itemToPush);

	return interpreter->Null;
}

Value* _ArrayPop(Interpreter* interpreter, int argc, Value** arguments) {
	// Reserve 1 arg for thisArg"
	if (argc != 1) {
		return NewErrorValue(interpreter, "Array.pop expects no arguments");
	}

	Value* thisArg = arguments[0];

	if (!ValueIsArray(thisArg)) {
		return NewErrorValue(interpreter,
							 "First argument to Array.pop must be an array");
	}

	Array* array = CoerceToArray(thisArg);
	if (ArrayLength(array) == 0) {
		return NewErrorValue(interpreter, "Cannot pop from an empty array");
	}

	void* poppedItem = ArrayGet(array, ArrayLength(array) - 1);
	array->Count--;	 // Reduce count to effectively pop the item

	return poppedItem;
}

static Value*
_ArrayLength(Interpreter* interpreter, int argc, Value** arguments) {
	// Reserve 1 arg for thisArg"
	if (argc != 1) {
		return NewErrorValue(interpreter, "Array.length expects no arguments");
	}

	Value* thisArg = arguments[0];

	if (!ValueIsArray(thisArg)) {
		return NewErrorValue(interpreter,
							 "First argument to Array.length must be an array");
	}

	Array* array = CoerceToArray(thisArg);
	return NewIntValue(interpreter, (int) ArrayLength(array));
}

static ModuleFunction _ArrayClassMethods[] = {
	// Array class
	{ .Name		 = "each",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _ArrayEach,
	  .Value	 = NULL },
	{ .Name		 = "keep",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _ArrayKeep,
	  .Value	 = NULL },
	{ .Name		 = "push",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _ArrayPush,
	  .Value	 = NULL },
	{ .Name		 = "pop",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _ArrayPop,
	  .Value	 = NULL },
	{ .Name		 = "length",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _ArrayLength,
	  .Value	 = NULL },
	// end of module functions
	{ .Name = NULL }
};

Value* CreateArrayClass(Interpreter* interpreter) {
	Value* arrayClass =
		NewClassValue(interpreter, CreateUserClass("Array", NULL));
	Class* cls = CoerceToUserClass(arrayClass);

	// Define Array methods here (e.g., push, pop, length, etc.)
	for (int i = 0; _ArrayClassMethods[i].Name != NULL; i++) {
		ModuleFunction func = _ArrayClassMethods[i];
		String		   hKey = func.Name;

		if (func.CFunction != NULL) {
			ClassDefineMemberByString(
				cls,
				hKey,
				NewNativeFunctionValue(
					interpreter,
					CreateNativeFunctionMeta((const String) hKey,
											 func.Argc,
											 func.CFunction)),
				false);
		}
	}

	return arrayClass;
}

Value* LoadCoreArray(Interpreter* interpreter) {
	Value* val = (interpreter->Array != NULL) ? interpreter->Array
											  : CreateArrayClass(interpreter);

	Value*	 module = NewObjectValue(interpreter);
	HashMap* map	= CoerceToHashMap(module);

	HashMapSet(map, "Array", val);
	return module;
}

#undef PUSH