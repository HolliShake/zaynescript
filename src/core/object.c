#include "./object.h"

#include "../error.h"

static Value* _ObjectInit(Interpreter* interp, int argc, Value** args) {
	if (argc != 1) {
		return NewErrorFValue(interp,
							  "%s: Object constructor expects 0 arguments",
							  ARGUMENT_ERROR);
	}
	return interp->Null;
}

static Value* _ObjectDestroy(Interpreter* interp, int argc, Value** args) {
	if (argc != 0) {
		return NewErrorFValue(interp,
							  "%s: Object destructor expects 0 arguments",
							  ARGUMENT_ERROR);
	}
	// Do nothing!!
	return interp->Null;
}

static Value* _ObjectKeys(Interpreter* interp, int argc, Value** args) {
	if (argc != 1 || !ValueIsObject(args[0]))
		return NewErrorFValue(interp,
							  "%s: Object.keys expects 1 object argument",
							  TYPE_ERROR);

	Value*	 obj	 = args[0];
	HashMap* map	 = CoerceToHashMap(obj);
	Value*	 keysArr = NewArrayValue(interp);
	Array*	 arr	 = (Array*) keysArr->Value.Opaque;

	HashNode* buckets = map->Buckets;
	for (size_t i = 0; i < map->Size; i++) {
		HashNode* node = &buckets[i];
		while (node != NULL && node->Key != NULL) {
			HashNode* next = node->Next;
			ArrayPush(arr, NewStrValue(interp, node->Key));
			node = next;
		}
	}

	return keysArr;
}

static Value* _ObjectValues(Interpreter* interp, int argc, Value** args) {
	if (argc != 1 || !ValueIsObject(args[0]))
		return NewErrorFValue(interp,
							  "%s: Object.values expects 1 object argument",
							  TYPE_ERROR);

	Value*	 obj	   = args[0];
	HashMap* map	   = CoerceToHashMap(obj);
	Value*	 valuesArr = NewArrayValue(interp);
	Array*	 arr	   = (Array*) valuesArr->Value.Opaque;

	HashNode* buckets = map->Buckets;
	for (size_t i = 0; i < map->Size; i++) {
		HashNode* node = &buckets[i];
		while (node != NULL && node->Key != NULL) {
			HashNode* next = node->Next;
			ArrayPush(arr, (Value*) node->Val);
			node = next;
		}
	}

	return valuesArr;
}

static Value* _ObjectFreeze(Interpreter* interp, int argc, Value** args) {
	if (argc != 1 || !ValueIsObject(args[0]))
		return NewErrorFValue(interp,
							  "%s: Object.freeze expects 1 object argument",
							  TYPE_ERROR);

	Value*	 obj	   = args[0];
	HashMap* map	   = HashMapClone(CoerceToHashMap(obj), true);
	Value*	 frozenObj = NewObjectValue(interp);
	FreeHashMap(CoerceToHashMap(frozenObj));
	frozenObj->Value.Opaque = map;
	return frozenObj;
}

static ModuleFunction _ObjectClassMethods[] = {
	{ .Name		 = CONSTRUCTOR_NAME,
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _ObjectInit,
	  .Value	 = NULL },
	{ .Name		 = DESTRUCTOR_NAME,
	  .Argc		 = 0,
	  .CFunction = (NativeFunctionCallback) _ObjectDestroy,
	  .Value	 = NULL },
	{ .Name = NULL }
};

static ModuleFunction _ObjectClassStatic[] = {
	// Members
	{
		.Name	   = "constructor",
		.Argc	   = 0,
		.CFunction = (NativeFunctionCallback) _ObjectInit,
		.Value	   = NULL,
	},
	// Static
	{
		.Name	   = "keys",
		.Argc	   = 1,
		.CFunction = (NativeFunctionCallback) _ObjectKeys,
		.Value	   = NULL,
	},
	{
		.Name	   = "values",
		.Argc	   = 1,
		.CFunction = (NativeFunctionCallback) _ObjectValues,
		.Value	   = NULL,
	},
	{
		.Name	   = "freeze",
		.Argc	   = 1,
		.CFunction = (NativeFunctionCallback) _ObjectFreeze,
		.Value	   = NULL,
	},
	{ .Name = NULL }
};

Value* CreateObjectClass(Interpreter* interpreter) {
	Value* objectClass =
		NewClassValue(interpreter, CreateUserClass("Object", NULL));
	Class* cls = CoerceToUserClass(objectClass);

	// Define Object methods here (e.g., init, etc.)
	for (int i = 0; _ObjectClassMethods[i].Name != NULL; i++) {
		ModuleFunction func = _ObjectClassMethods[i];
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

	// Define static methods
	for (int i = 0; _ObjectClassStatic[i].Name != NULL; i++) {
		ModuleFunction func = _ObjectClassStatic[i];
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
				true);
		}
	}

	return objectClass;
}

Value* LoadCoreObject(Interpreter* interpreter) {
	Value* val = (interpreter->Object != NULL)
					 ? interpreter->Object
					 : (interpreter->Object = CreateObjectClass(interpreter));

	Value*	 module = NewObjectValue(interpreter);
	HashMap* map	= CoerceToHashMap(module);

	HashMapSet(map, "Object", val);
	return module;
}
