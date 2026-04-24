#include "./os.h"


// --- Platform Compatibility Layer ---
#ifdef _WIN32
#	include <direct.h>	  // for _getcwd
#	include <process.h>  // for _getpid
#	include <windows.h>  // for GetUserNameA
#	define getpid _getpid
#	define getcwd _getcwd
#else
#	include <pwd.h>	 // for backup user detection
#	include <unistd.h>	 // for getpid, getcwd
#endif

#if defined(__ANDROID__) || defined(TERMUX)
static inline int getlogin_r(char* name, size_t namesize) {
	const char* uname = "termux";
	size_t		len	  = strlen(uname) + 1;
	if (namesize < len)
		return -1;
	memcpy(name, uname, len);
	return 0;  // Return 0 for success
}
#endif

static Value* _OsGetCwd(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 0) {
		return NewErrorFValue(interpreter,
							  "%s: getCwd() expects exactly 0 arguments",
							  ARGUMENT_ERROR);
	}

	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		return NewStrValue(interpreter, cwd);
	}
	return NewErrorFValue(interpreter,
						  "%s: failed to get current working directory",
						  RUNTIME_ERROR);
}

static Value* _OsGetPid(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 0) {
		return NewErrorFValue(interpreter,
							  "%s: getPid() expects exactly 0 arguments",
							  ARGUMENT_ERROR);
	}

	return NewIntValue(interpreter, (int) getpid());
}

static Value*
_OsGetUser(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 0) {
		return NewErrorFValue(interpreter,
							  "%s: getUser() expects exactly 0 arguments",
							  ARGUMENT_ERROR);
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
	if (login)
		return NewStrValue(interpreter, login);
#endif

	return NewErrorFValue(interpreter,
						  "%s: failed to get username",
						  RUNTIME_ERROR);
}

static Value* _OsSystem(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: system() expects exactly 1 argument",
							  ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: system() expects a string as its argument",
							  TYPE_ERROR);
	}

	String cmd	  = ValueToString(arguments[0]);
	int	   status = system(cmd);
	// Note: ensure free(cmd) matches how your interpreter
	// allocates strings
	free(cmd);

	return NewIntValue(interpreter, status);
}

// _OsGetType remains the same as your original (it was correctly
// using #ifdefs)
static Value*
_OsGetType(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 0) {
		return NewErrorFValue(interpreter,
							  "%s: getType() expects exactly 0 arguments",
							  ARGUMENT_ERROR);
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

static Value* _OsArgs(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 0) {
		return NewErrorFValue(interpreter,
							  "%s: args() expects exactly 0 arguments",
							  ARGUMENT_ERROR);
	}

	Value* array = NewArrayValue(interpreter);
	Array* arr	 = CoerceToArray(array);

	String argStr = interpreter->ArgString;
	if (argStr == NULL)
		return array;

	while (*argStr) {
		while (*argStr == ' ')
			argStr++;
		if (*argStr == '\0')
			break;

		const char* start = argStr;
		while (*argStr != '\0' && *argStr != ' ')
			argStr++;

		size_t len = (size_t) (argStr - start);
		String tok = (String) Allocate(len + 1);
		memcpy(tok, start, len);
		tok[len] = '\0';
		ArrayPush(arr, NewStrValue(interpreter, tok));
		free(tok);
	}

	return array;
}

// ... Rest of your Module Loading logic remains the same ...

static ModuleFunction _OsModuleFunctions[] = {
	// getCwd
	{ .Name		 = "getCwd",
	  .Argc		 = 0,
	  .CFunction = (NativeFunctionCallback) (_OsGetCwd),
	  .Value	 = NULL },
	// getPid
	{ .Name		 = "getPid",
	  .Argc		 = 0,
	  .CFunction = (NativeFunctionCallback) (_OsGetPid),
	  .Value	 = NULL },
	// getUser
	{ .Name		 = "getUser",
	  .Argc		 = 0,
	  .CFunction = (NativeFunctionCallback) (_OsGetUser),
	  .Value	 = NULL },
	// system
	{ .Name		 = "system",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) (_OsSystem),
	  .Value	 = NULL },
	// getType
	{ .Name		 = "getType",
	  .Argc		 = 0,
	  .CFunction = (NativeFunctionCallback) (_OsGetType),
	  .Value	 = NULL },
	// Arg
	{
		.Name	   = "args",
		.Argc	   = 0,
		.CFunction = (NativeFunctionCallback) (_OsArgs),
		.Value	   = NULL,
	},
	// end of module functions
	{ .Name = NULL }
};

Value* LoadCoreOs(Interpreter* interpreter) {
	Value*	 osModule = NewObjectValue(interpreter);
	HashMap* osMap	  = CoerceToHashMap(osModule);

	for (int i = 0; _OsModuleFunctions[i].Name != NULL; i++) {
		ModuleFunction func = _OsModuleFunctions[i];
		String		   hKey = func.Name;

		if (func.Value != NULL) {
			HashMapSet(osMap, hKey, _OsModuleFunctions[i].Value);
		} else {
			HashMapSet(osMap,
					   hKey,
					   NewNativeFunctionValue(
						   interpreter,
						   CreateNativeFunctionMeta((const String) hKey,
													func.Argc,
													func.CFunction)));
		}
	}

	return osModule;
}
