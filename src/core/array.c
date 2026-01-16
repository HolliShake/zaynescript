#include "./array.h"

Value* _ArrayPush(Interpreter* interpreter, int argc, Value** arguments) {
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

static ModuleFunction _IoModuleFunctions[] = {
    // Array class
    { .Name = "push", .Argc = 2, .CFunction = (NativeFunction) _ArrayPush, .Value = NULL },
    // end of module functions
    { .Name = NULL }
};

Value* CreateArrayClass(Interpreter* interpreter) {
    Value* arrayClass = NewClassValue(interpreter, CreateUserClass("Array", NULL));
    UserClass* cls = CoerceToUserClass(arrayClass);

    // Define Array methods here (e.g., push, pop, length, etc.)
    for (int i = 0; _IoModuleFunctions[i].Name != NULL; i++) {
        ModuleFunction func = _IoModuleFunctions[i];
        String name = AllocateString(func.Name);
        
        if (func.CFunction != NULL) {
            ClassDefineMemberByString(
                cls, 
                name, 
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