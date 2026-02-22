#include "./io.h"


static Value* _IoGenericPrint(Interpreter* interpeter, int argc, Value** arguments, bool newline) {
    if (argc == 0) {
        printf("\n");
        return interpeter->Null;
    }
    
    size_t bufferSize = 256;
    size_t bufferUsed = 0;
    char* buffer = Allocate(bufferSize);
    buffer[0] = '\0';
    
    for (int i = 0; i < argc; i++) {
        String str = ValueToString(arguments[i]);
        size_t strLen = strlen(str);
        
        // Ensure buffer has enough space (including space and null terminator)
        size_t spaceNeeded = strLen + (i < argc - 1 ? 1 : 0); // +1 for space if not last
        while (bufferUsed + spaceNeeded + 1 >= bufferSize) {
            bufferSize *= 2;
            buffer = Reallocate(buffer, bufferSize);
        }
        
        strcpy(buffer + bufferUsed, str);
        bufferUsed += strLen;
        free(str);
        
        // Add space after each argument except the last one
        if (i < argc - 1) {
            buffer[bufferUsed] = ' ';
            bufferUsed++;
            buffer[bufferUsed] = '\0';
        }
    }
    
    printf("%s%s", buffer, newline ? "\n" : "");
    free(buffer);
    return interpeter->Null;
}

static Value* _IoPrint(Interpreter* interpeter, int argc, Value** arguments) {
    return _IoGenericPrint(interpeter, argc, arguments, false);
}

static Value* _IoPrintln(Interpreter* interpeter, int argc, Value** arguments) {
    return _IoGenericPrint(interpeter, argc, arguments, true);
}

static Value* _IoScan(Interpreter* interpreter, int argc, Value** arguments) {

    if (argc != 1) {
        return NewErrorValue(interpreter, "scan() expects exactly 1 argument");
    }

    if (!ValueIsStr(arguments[0])) {
        return NewErrorValue(interpreter, "scan() expects a string as its argument");
    }

    // Print the prompt message if provided
    if (argc > 0) {
        String prompt = ValueToString(arguments[0]);
        printf("%s", prompt);
        fflush(stdout);
        free(prompt);
    }
    
    size_t bufferSize = 1024;
    size_t totalRead = 0;
    char* buffer = Allocate(bufferSize);
    
    // Read input dynamically, expanding buffer as needed
    while (1) {
        if (fgets(buffer + totalRead, bufferSize - totalRead, stdin) == NULL) {
            if (totalRead == 0) {
                free(buffer);
                return interpreter->Null;
            }
            break;
        }
        
        size_t justRead = strlen(buffer + totalRead);
        totalRead += justRead;
        
        // Check if we hit a newline (end of input)
        if (totalRead > 0 && buffer[totalRead - 1] == '\n') {
            buffer[totalRead - 1] = '\0';
            break;
        }
        
        // If buffer is full and no newline, expand it
        if (totalRead >= bufferSize - 1) {
            bufferSize *= 2;
            buffer = Reallocate(buffer, bufferSize);
        } else {
            // fgets returned but didn't fill buffer, we're done
            break;
        }
    }
    
    //NOTE: memory leak (AllocateString creates a char* string that is passed to NewStrValue, but NewStrValue converts it to Runes and doesn't free the original char* string)
    Value* result = NewStrValue(interpreter, AllocateString(buffer));
    free(buffer);
    return result;
}

static Value* _IoParseNum(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc != 1) {
        return NewErrorValue(interpreter, "parseNum() expects exactly 1 argument");
    }
    if (!ValueIsStr(arguments[0])) {
        return NewErrorValue(interpreter, "parseNum() expects a string as its argument");
    }
    String str = ValueToString(arguments[0]);
    char* endptr;
    double num = strtod(str, &endptr);
    free(str);
    
    // Check if the number can be represented as an integer
    if (num == (int)num) {
        return NewIntValue(interpreter, (int)num);
    }
    return NewNumValue(interpreter, num);
}

static ModuleFunction _IoModuleFunctions[] = {
    // print
    { .Name = "print",   .Argc = VARARG, .CFunction = (NativeFunctionCallback) (_IoPrint),    .Value = NULL },
    // println
    { .Name = "println", .Argc = VARARG, .CFunction = (NativeFunctionCallback) (_IoPrintln),  .Value = NULL },
    // scan
    { .Name = "scan",    .Argc =      1, .CFunction = (NativeFunctionCallback) (_IoScan),     .Value = NULL },
    // parse num
    { .Name = "parseNum", .Argc =     1, .CFunction = (NativeFunctionCallback) (_IoParseNum), .Value = NULL },
    // end of module functions
    { .Name = NULL }
};

Value* LoadCoreIo(Interpreter* interpeter) {
    Value* ioModule = NewObjectValue(interpeter);
    HashMap* ioMap  = CoerceToHashMap(ioModule);
    
    for (int i = 0; _IoModuleFunctions[i].Name != NULL; i++) {
        ModuleFunction func = _IoModuleFunctions[i];
        String name = AllocateString(func.Name);
        String hKey = AllocateString(func.Name);
        
        if (func.Value != NULL) {
            HashMapSet(ioMap, hKey, _IoModuleFunctions[i].Value);
        } else {
            HashMapSet(ioMap, hKey, NewNativeFunctionValue(interpeter, 
                CreateNativeFunctionMeta(
                    (const String) name,
                    func.Argc,
                    func.CFunction
                )
            ));
        }
    }

    return ioModule;
}