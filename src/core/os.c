#include "./os.h"

// --- Platform Compatibility Layer ---
#ifdef _WIN32
    #include <process.h>  // for _getpid
    #include <direct.h>   // for _getcwd
    #include <windows.h>  // for GetUserNameA
    #define getpid _getpid
    #define getcwd _getcwd
#else
    #include <unistd.h>   // for getpid, getcwd
    #include <pwd.h>      // for backup user detection
#endif

static Value* _OsGetCwd(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc != 0) {
        return NewErrorValue(interpreter, "getcwd() expects exactly 0 arguments");
    }
    
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return NewStrValue(interpreter, cwd);
    }
    return NewErrorValue(interpreter, "Failed to get current working directory");
}

static Value* _OsGetPid(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc != 0) {
        return NewErrorValue(interpreter, "getpid() expects exactly 0 arguments");
    }
    
    return NewIntValue(interpreter, (int)getpid());
}

static Value* _OsGetUser(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc != 0) {
        return NewErrorValue(interpreter, "getuser() expects exactly 0 arguments");
    }
    
    char username[256];

#ifdef _WIN32
    DWORD size = sizeof(username);
    if (GetUserNameA(username, &size)) {
        return NewStrValue(interpreter, username);
    }
#else
    // POSIX getlogin_r
    if (getlogin_r(username, sizeof(username)) == 0) {
        return NewStrValue(interpreter, username);
    }
    // Fallback for some headless Linux environments
    String login = getenv("USER");
    if (login) return NewStrValue(interpreter, login);
#endif

    return NewErrorValue(interpreter, "Failed to get username");
}

static Value* _OsSystem(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc != 1) {
        return NewErrorValue(interpreter, "system() expects exactly 1 argument");
    }
    if (!ValueIsStr(arguments[0])) {
        return NewErrorValue(interpreter, "system() expects a string as its argument");
    }
    
    String cmd = ValueToString(arguments[0]);
    int status = system(cmd);
    // Note: ensure free(cmd) matches how your interpreter allocates strings
    free(cmd); 
    
    return NewIntValue(interpreter, status);
}

// _OsGetType remains the same as your original (it was correctly using #ifdefs)
static Value* _OsGetType(Interpreter* interpreter, int argc, Value** arguments) {
    if (argc != 0) {
        return NewErrorValue(interpreter, "type() expects exactly 0 arguments");
    }
#if defined(_WIN32)
    return NewStrValue(interpreter, "win32");
#elif defined(__APPLE__)
    return NewStrValue(interpreter, "mac");
#elif defined(__linux__)
    return NewStrValue(interpreter, "linux");
#else
    return NewStrValue(interpreter, "unknown");
#endif
}

// ... Rest of your Module Loading logic remains the same ...

static ModuleFunction _OsModuleFunctions[] = {
    // getcwd
    { .Name = "getcwd" , .Argc = 0, .CFunction = (NativeFunctionCallback) (_OsGetCwd) , .Value = NULL  },
    // getpid
    { .Name = "getpid" , .Argc = 0, .CFunction = (NativeFunctionCallback) (_OsGetPid) , .Value = NULL  },
    // getuser
    { .Name = "getuser", .Argc = 0, .CFunction = (NativeFunctionCallback) (_OsGetUser), .Value = NULL  },
    // system
    { .Name = "system" , .Argc = 1, .CFunction = (NativeFunctionCallback) (_OsSystem) , .Value = NULL  },
    // type
    { .Name = "type"   , .Argc = 0, .CFunction = (NativeFunctionCallback) (_OsGetType), .Value = NULL  },
    // end of module functions
    { .Name = NULL }
};

Value* LoadCoreOs(Interpreter* interpeter) {
    Value* osModule = NewObjectValue(interpeter);
    HashMap* osMap = CoerceToHashMap(osModule);
    
    for (int i = 0; _OsModuleFunctions[i].Name != NULL; i++) {
        ModuleFunction func = _OsModuleFunctions[i];
        String name = AllocateString(func.Name);
        String hKey = AllocateString(func.Name);
        
        if (func.Value != NULL) {
            HashMapSet(osMap, hKey, _OsModuleFunctions[i].Value);
        } else {
            HashMapSet(osMap, hKey, NewNativeFunctionValue(interpeter, 
                CreateNativeFunctionMeta(
                    (const String) name,
                    func.Argc,
                    func.CFunction
                )
            ));
        }
    }

    return osModule;
}

