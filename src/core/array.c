#include "./array.h"

Value* _ArrayPush(Interpreter* interpreter, int argc, Value** arguments) {
    // Reserve 1 arg for thisArg"
    if (argc != 2) {
        return NewErrorValue(interpreter, "Array.push expects 1 argument");
    }

    Value* thisArg    = arguments[0];
    Value* itemToPush = arguments[1];

    if (!ValueIsArray(thisArg)) {
        return NewErrorValue(interpreter, "First argument to Array.push must be an array");
    }

    Array* array = CoerceToArray(thisArg);
    ArrayPush(array, itemToPush);

    return interpreter->Null;
}

static Value* _ArrayLength(Interpreter* interpreter, int argc, Value** arguments) {
    // Reserve 1 arg for thisArg"
    if (argc != 1) {
        return NewErrorValue(interpreter, "Array.length expects no arguments");
    }

    Value* thisArg = arguments[0];

    if (!ValueIsArray(thisArg)) {
        return NewErrorValue(interpreter, "First argument to Array.length must be an array");
    }

    Array* array = CoerceToArray(thisArg);
    return NewIntValue(interpreter, (int) ArrayLength(array));
}

static ModuleFunction _ArrayClassMethods[] = {
    // Array class
    { .Name = "push",   .Argc = 2, .CFunction = (NativeFunctionCallback) _ArrayPush  , .Value = NULL },
    { .Name = "length", .Argc = 1, .CFunction = (NativeFunctionCallback) _ArrayLength, .Value = NULL },
    // end of module functions
    { .Name = NULL }
};

Value* CreateArrayClass(Interpreter* interpreter) {
    Value* arrayClass = NewClassValue(interpreter, CreateUserClass("Array", NULL));
    Class* cls = CoerceToUserClass(arrayClass);

    // Define Array methods here (e.g., push, pop, length, etc.)
    for (int i = 0; _ArrayClassMethods[i].Name != NULL; i++) {
        ModuleFunction func = _ArrayClassMethods[i];
        String name = AllocateString(func.Name);
        String hKey = AllocateString(func.Name);
        
        if (func.CFunction != NULL) {
            ClassDefineMemberByString(
                cls, 
                hKey, 
                NewNativeFunctionValue(interpreter, 
                    CreateNativeFunctionMeta(
                        (const String) name,
                        func.Argc,
                        func.CFunction
                    )
                ), 
                false
            );
        }
    }

    return arrayClass;
}