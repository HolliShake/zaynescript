#include "./array.h"

/**
 * @brief Appends value at interpreter->Stacks[StckC++] so it becomes the new
 * operand-stack top.
 * @param interpreter VM whose StckC indexes the next free stack slot.
 * @param value Pointer stored on the stack; lifetime must cover the span it
 * remains reachable from the stack.
 * @origin src/interpreter.c
 */
extern void Push(Interpreter* interpreter, Value* value);

/**
 * @brief Returns interpreter->Stacks[--StckC], removing one slot from the
 * logical operand stack.
 * @param interpreter VM whose StckC must be > 0; otherwise behaviour is
 * undefined.
 * @return The Value* that was previously the stack top.
 * @origin src/interpreter.c
 */
extern Value* Popp(Interpreter* interpreter);

/**
 * @brief Reads interpreter->Stacks[StckC - 1] without changing StckC
 * (non-destructive top-of-stack).
 * @param interpreter VM whose StckC must be > 0; otherwise behaviour is
 * undefined.
 * @return Current top operand without popping it.
 * @origin src/interpreter.c
 */
extern Value* Peek(Interpreter* interpreter);

/**
 * @brief Dispatches fn as a callable: wires environments for user functions,
 * unwraps async targets into promises, validates native arity, consumes argc
 * stack operands, and leaves the callee result (or an Error value) per the
 * concrete callee kind.
 * @param interpreter Full VM state (stack, env stack, traces, active task).
 * @param fn User function, native function, promise continuation, or related
 * callable Value.
 * @param argc Operand count already present on the stack for this call (see
 * withThis for layout).
 * @param withThis When true, the lowest logical argument on the stack is bound
 * as the callee's this.
 * @return Callee result, interpreter->Null for async promise bootstrap paths,
 * or an Error Value on failure.
 * @origin src/operation.c
 */
extern Value*
DoCall(Interpreter* interpreter, Value* fn, int argc, bool withThis);

static Value*
_ArrayInit(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 1) {
		return NewErrorFValue(interpreter,
							  "Array constructor requires at least 1 argument");
	}

	Value* thisArg = arguments[0];
	if (!ValueIsArray(thisArg)) {
		return NewErrorFValue(interpreter,
							  "Array constructor requires an Array argument");
	}

	Array* array = CoerceToArray(thisArg);

	for (int i = 1; i < argc; i++) {
		ArrayPush(array, arguments[i]);
	}

	return interpreter->Null;
}

static Value*
_ArrayEach(Interpreter* interpreter, int argc, Value** arguments) {
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
	Push(interpreter, arrayVal);

	for (size_t i = 0; i < ArrayLength(array); i++) {
		Value* item	 = ArrayGet(array, i);
		Value* index = NewNumValue(interpreter, (int) i);
		Push(interpreter, index);
		Push(interpreter, item);
		DoCall(interpreter, callback, argc, false);
		ArrayPush(newArray, Popp(interpreter));
	}

	Popp(interpreter);
	return arrayVal;
}

static Value*
_ArrayKeep(Interpreter* interpreter, int argc, Value** arguments) {
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
	Push(interpreter, arrayVal);

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

	Popp(interpreter);
	return arrayVal;
}

static Value*
_ArrayPush(Interpreter* interpreter, int argc, Value** arguments) {
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

static Value* _ArrayPop(Interpreter* interpreter, int argc, Value** arguments) {
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
	{ .Name		 = CONSTRUCTOR_NAME,
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _ArrayInit,
	  .Value	 = NULL },
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
		NewClassValue(interpreter,
					  CreateUserClass("Array", interpreter->Object));
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
	Value* val = (interpreter->Array != NULL)
					 ? interpreter->Array
					 : (interpreter->Array = CreateArrayClass(interpreter));

	Value*	 module = NewObjectValue(interpreter);
	HashMap* map	= CoerceToHashMap(module);

	HashMapSet(map, "Array", val);
	return module;
}

#undef PUSH