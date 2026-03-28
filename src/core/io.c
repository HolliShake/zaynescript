#include "./io.h"

static Value* _IoGenericPrint(Interpreter* interpreter, int argc, Value** arguments, bool newline) {
    if (argc == 0) {
        puts(newline ? "" : "");  // or just:
        if (newline) putchar('\n');
        return interpreter->Null;
    }

    // ----------------------------------------------------------------
    // PASS 1: resolve all strings + measure total length
    // ----------------------------------------------------------------
    String *parts = Allocate(argc * sizeof(String));
    if (!parts) return interpreter->Null;
    size_t *lens  = Allocate(argc * sizeof(size_t));
    if (!lens) { free(parts); return interpreter->Null; }

    // total = sum of lengths + (argc-1) spaces + '\0'
    size_t total = (argc - 1) + 1;
    for (int i = 0; i < argc; i++) {
        parts[i] = ValueToString(arguments[i]);
        lens[i]  = parts[i] ? strlen(parts[i]) : 0;
        total   += lens[i];
    }

    // ----------------------------------------------------------------
    // PASS 2: single alloc, memcpy in
    // ----------------------------------------------------------------
    String buffer = Allocate(total);
    if (!buffer) goto cleanup;

    String p = buffer;
    for (int i = 0; i < argc; i++) {
        if (lens[i] > 0) {
            memcpy(p, parts[i], lens[i]);
            p += lens[i];
        }
        if (i < argc - 1) *p++ = ' ';
    }
    *p = '\0';

    // single write syscall — faster than printf format processing
    fputs("\x1B[93m", stdout);
    fwrite(buffer, 1, total - 1, stdout);  // total-1 excludes '\0'
    fputs("\x1B[0m", stdout);
    if (newline) putchar('\n');

    free(buffer);

cleanup:
    for (int i = 0; i < argc; i++) free(parts[i]);
    free(parts);
    free(lens);

    return interpreter->Null;
}

static Value* _IoPrint(Interpreter*  interpreter, int argc, Value** arguments) {
    return _IoGenericPrint( interpreter, argc, arguments, false);
}

static Value* _IoPrintln(Interpreter*  interpreter, int argc, Value** arguments) {
    return _IoGenericPrint( interpreter, argc, arguments, true);
}

static Value* _IoScan(Interpreter* interpreter, int argc, Value** arguments) {

    if (argc > 1) {
        return NewErrorValue(interpreter, "scan() expects 0 or 1 argument");
    }

    if (argc == 1 && !ValueIsStr(arguments[0])) {
        return NewErrorValue(interpreter, "scan() expects a string as its argument");
    }

    // Print the prompt message if provided
    if (argc == 1) {
        String prompt = ValueToString(arguments[0]);
        printf("%s", prompt);
        fflush(stdout);
        free(prompt);
    }
    
    size_t bufferSize = 1024;
    size_t totalRead = 0;
    String buffer = Allocate(bufferSize);
    
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
    
    Value* result = NewStrValue(interpreter, buffer);
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
    String endptr;
    double num = strtod(str, &endptr);
    free(str);
    
    // Check if the number can be represented as an integer
    if (num == (int)num) {
        return NewIntValue(interpreter, (int)num);
    }
    return NewNumValue(interpreter, num);
}

static Value* _IoFormat(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc < 1) {
        return NewErrorValue(interpreter, "format() expects at least 1 argument");
    }
    if (!ValueIsStr(arguments[0])) {
        return NewErrorValue(interpreter, "format() expects the first argument to be a string");
    }

    String formatStr = ValueToString(arguments[0]);
    size_t formatLen = strlen(formatStr);

    // Estimate initial buffer size
    size_t bufferSize = formatLen + argc * 32;
    String buffer = Allocate(bufferSize);
    size_t bufferUsed = 0;

    int argIndex = 1;
    for (size_t i = 0; i < formatLen; ) {
        if (formatStr[i] == '{' && formatStr[i+1] == '}' && argIndex < argc) {
            // Insert argument string
            String argStr = ValueToString(arguments[argIndex]);
            size_t argLen = strlen(argStr);

            // Ensure buffer is large enough
            while (bufferUsed + argLen + 1 >= bufferSize) {
                bufferSize *= 2;
                buffer = Reallocate(buffer, bufferSize);
            }
            strcpy(buffer + bufferUsed, argStr);
            bufferUsed += argLen;
            free(argStr);

            i += 2;
            argIndex++;
        } else {
            // Copy character
            if (bufferUsed + 2 >= bufferSize) {
                bufferSize *= 2;
                buffer = Reallocate(buffer, bufferSize);
            }
            buffer[bufferUsed++] = formatStr[i++];
        }
    }
    buffer[bufferUsed] = '\0';
    free(formatStr);

    Value* result = NewStrValue(interpreter, buffer);
    free(buffer);
    return result;
}

static Value* _IoClearScreen(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc != 0) {
        return NewErrorValue(interpreter, "clearScreen() expects 0 arguments");
    }
    printf("\x1B[2J\x1B[H");
    fflush(stdout);
    return interpreter->Null;
}

static Value* _IoSetColor(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc > 2) {
        return NewErrorValue(interpreter, "setColor() expects 0, 1, or 2 arguments (fg, bg)");
    }
    
    if (argc == 0) {
        printf("\x1B[0m"); // reset
        fflush(stdout);
        return interpreter->Null;
    }

    int fg = (int) CoerceToNum(arguments[0]);
    if (argc == 1) {
        printf("\x1B[%dm", fg);
    } else if (argc == 2) {
        int bg = (int) CoerceToNum(arguments[1]);
        printf("\x1B[%d;%dm", fg, bg);
    }
    
    fflush(stdout);
    return interpreter->Null;
}

static ModuleFunction _IoModuleFunctions[] = {
    // print
    { .Name = "print",    .Argc = VARARG, .CFunction = (NativeFunctionCallback) (_IoPrint),    .Value = NULL },
    // println
    { .Name = "println",  .Argc = VARARG, .CFunction = (NativeFunctionCallback) (_IoPrintln),  .Value = NULL },
    // scan
    { .Name = "scan",     .Argc = VARARG, .CFunction = (NativeFunctionCallback) (_IoScan),     .Value = NULL },
    // parse num
    { .Name = "parseNum", .Argc =      1, .CFunction = (NativeFunctionCallback) (_IoParseNum), .Value = NULL },
    // format
    { .Name = "format",   .Argc = VARARG, .CFunction = (NativeFunctionCallback) (_IoFormat),   .Value = NULL },
    // clearScreen
    { .Name = "clearScreen", .Argc =   0, .CFunction = (NativeFunctionCallback) (_IoClearScreen), .Value = NULL },
    // setColor
    { .Name = "setColor",    .Argc = VARARG, .CFunction = (NativeFunctionCallback) (_IoSetColor), .Value = NULL },
    // end of module functions
    { .Name = NULL }
};

Value* LoadCoreIo(Interpreter*  interpreter) {
    Value* ioModule = NewObjectValue( interpreter);
    HashMap* ioMap  = CoerceToHashMap(ioModule);
    
    for (int i = 0; _IoModuleFunctions[i].Name != NULL; i++) {
        ModuleFunction func = _IoModuleFunctions[i];
        String hKey = func.Name;
        
        if (func.Value != NULL) {
            HashMapSet(ioMap, hKey, _IoModuleFunctions[i].Value);
        } else {
            HashMapSet(ioMap, hKey, NewNativeFunctionValue(interpreter, 
                CreateNativeFunctionMeta(
                    (const String) hKey,
                    func.Argc,
                    func.CFunction
                )
            ));
        }
    }

    return ioModule;
}