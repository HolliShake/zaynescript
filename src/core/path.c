#include "./path.h"

#include "../error.h"

#ifdef _WIN32
#	include <io.h>
#	define PATH_SEPARATOR "\\"
#else
#	define PATH_SEPARATOR "/"
#endif

static Value*
_PathNormalize(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(
			interpreter,
			"%s: normalize() expects exactly 1 argument",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(
			interpreter,
			"%s: normalize() expects a string argument",
			TYPE_ERROR);
	}
	String in  = ValueToString(arguments[0]);
	String out = NormalizePath(in);
	free(in);
	Value* ret = NewStrValue(interpreter, out);
	free(out);
	return ret;
}

static Value*
_PathBasename(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(
			interpreter,
			"%s: basename() expects exactly 1 argument",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(
			interpreter,
			"%s: basename() expects a string argument",
			TYPE_ERROR);
	}
	String in  = ValueToString(arguments[0]);
	String out = Basename(in);
	free(in);
	Value* ret = NewStrValue(interpreter, out);
	free(out);
	return ret;
}

static Value*
_PathDirname(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(
			interpreter,
			"%s: dirname() expects exactly 1 argument",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(
			interpreter,
			"%s: dirname() expects a string argument",
			TYPE_ERROR);
	}
	String in  = ValueToString(arguments[0]);
	String out = Dirname(in);
	free(in);
	Value* ret = NewStrValue(interpreter, out);
	free(out);
	return ret;
}

static Value*
_PathAbsolute(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(
			interpreter,
			"%s: absolutePath() expects exactly 1 argument",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(
			interpreter,
			"%s: absolutePath() expects a string argument",
			TYPE_ERROR);
	}
	String in  = ValueToString(arguments[0]);
	String out = AbsolutePath(in);
	free(in);
	if (!out) {
		return NewErrorFValue(
			interpreter,
			"%s: absolutePath() could not resolve the path",
			RUNTIME_ERROR);
	}
	Value* ret = NewStrValue(interpreter, out);
	free(out);
	return ret;
}

static Value*
_PathAbsoluteFromBase(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(
			interpreter,
			"%s: absolutePathFromBase() expects exactly 2 arguments",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0]) || !ValueIsStr(arguments[1])) {
		return NewErrorFValue(
			interpreter,
			"%s: absolutePathFromBase() expects two string arguments",
			TYPE_ERROR);
	}
	String base = ValueToString(arguments[0]);
	String rel	= ValueToString(arguments[1]);
	String out	= AbsolutePathFromBase(base, rel);
	free(base);
	free(rel);
	if (!out) {
		return NewErrorFValue(
			interpreter,
			"%s: absolutePathFromBase() could not resolve the path",
			RUNTIME_ERROR);
	}
	Value* ret = NewStrValue(interpreter, out);
	free(out);
	return ret;
}

static Value* _PathJoin(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: join() expects exactly 2 arguments",
							  ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0]) || !ValueIsStr(arguments[1])) {
		return NewErrorFValue(interpreter,
							  "%s: join() expects two string arguments",
							  TYPE_ERROR);
	}
	String a   = ValueToString(arguments[0]);
	String b   = ValueToString(arguments[1]);
	String out = JoinPath(a, b);
	free(a);
	free(b);
	if (!out) {
		return NewErrorFValue(interpreter,
							  "%s: join() failed",
							  RUNTIME_ERROR);
	}
	Value* ret = NewStrValue(interpreter, out);
	free(out);
	return ret;
}

static Value*
_PathExists(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(
			interpreter,
			"%s: exists() expects exactly 1 argument",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: exists() expects a string argument",
							  TYPE_ERROR);
	}
	String p = ValueToString(arguments[0]);
	bool   e = PathExists(p);
	free(p);
	return e ? interpreter->True : interpreter->False;
}

static Value*
_PathIsDirectory(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(
			interpreter,
			"%s: isDirectory() expects exactly 1 argument",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(
			interpreter,
			"%s: isDirectory() expects a string argument",
			TYPE_ERROR);
	}
	String p = ValueToString(arguments[0]);
	bool   v = PathIsDirectory(p);
	free(p);
	return v ? interpreter->True : interpreter->False;
}

static Value*
_PathIsFile(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(
			interpreter,
			"%s: isFile() expects exactly 1 argument",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: isFile() expects a string argument",
							  TYPE_ERROR);
	}
	String p = ValueToString(arguments[0]);
	bool   v = PathIsRegularFile(p);
	free(p);
	return v ? interpreter->True : interpreter->False;
}

static Value*
_PathIsAbsolute(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(
			interpreter,
			"%s: isAbsolute() expects exactly 1 argument",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(
			interpreter,
			"%s: isAbsolute() expects a string argument",
			TYPE_ERROR);
	}
	String p = ValueToString(arguments[0]);
	bool   v = IsAbsolutePath(p);
	free(p);
	return v ? interpreter->True : interpreter->False;
}

static Value*
_PathSeparator(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 0) {
		return NewErrorFValue(
			interpreter,
			"%s: separator() expects exactly 0 arguments",
			ARGUMENT_ERROR);
	}
	char sep[2] = { NativePathSeparator(), '\0' };
	return NewStrValue(interpreter, sep);
}

String JoinPath(String baseStr, String segmentStr) {
	if (!segmentStr || segmentStr[0] == '\0') {
		if (!baseStr || baseStr[0] == '\0')
			return AllocateString(".");
		String c = AllocateString(baseStr);
		String n = NormalizePath(c);
		free(c);
		return n;
	}
	if (!baseStr || baseStr[0] == '\0') {
		String c = AllocateString(segmentStr);
		String n = NormalizePath(c);
		free(c);
		return n;
	}

	size_t bl = strlen(baseStr);
	while (bl > 0 && (baseStr[bl - 1] == '/' || baseStr[bl - 1] == '\\')) {
		bl--;
	}

	const char* seg = segmentStr;
	while (seg[0] == '/' || seg[0] == '\\') {
		seg++;
	}

	size_t total = bl + strlen(PATH_SEPARATOR) + strlen(seg) + 1;
	String buf	 = Allocate(total);
	if (!buf)
		return NULL;

	memcpy(buf, baseStr, bl);
	buf[bl] = '\0';
	strcat(buf, PATH_SEPARATOR);
	strcat(buf, seg);

	String out = NormalizePath(buf);
	free(buf);
	return out;
}

bool PathExists(String path) {
	if (!path || path[0] == '\0')
		return false;
#ifdef _WIN32
	struct _stat st;
	return _stat(path, &st) == 0;
#else
	struct stat st;
	return stat(path, &st) == 0;
#endif
}

bool PathIsDirectory(String path) {
	if (!path || path[0] == '\0')
		return false;
#ifdef _WIN32
	struct _stat st;
	if (_stat(path, &st) != 0)
		return false;
	return (st.st_mode & _S_IFDIR) != 0;
#else
	struct stat st;
	if (stat(path, &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
#endif
}

bool PathIsRegularFile(String path) {
	if (!path || path[0] == '\0')
		return false;
#ifdef _WIN32
	struct _stat st;
	if (_stat(path, &st) != 0)
		return false;
	return (st.st_mode & _S_IFREG) != 0;
#else
	struct stat st;
	if (stat(path, &st) != 0)
		return false;
	return S_ISREG(st.st_mode);
#endif
}

char NativePathSeparator(void) {
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}

static ModuleFunction _PathModuleFunctions[] = {
	{ .Name		 = "normalize",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _PathNormalize,
	  .Value	 = NULL },
	{ .Name		 = "basename",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _PathBasename,
	  .Value	 = NULL },
	{ .Name		 = "dirname",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _PathDirname,
	  .Value	 = NULL },
	{ .Name		 = "absolutePath",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _PathAbsolute,
	  .Value	 = NULL },
	{ .Name		 = "absolutePathFromBase",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _PathAbsoluteFromBase,
	  .Value	 = NULL },
	{ .Name		 = "join",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _PathJoin,
	  .Value	 = NULL },
	{ .Name		 = "exists",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _PathExists,
	  .Value	 = NULL },
	{ .Name		 = "isDirectory",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _PathIsDirectory,
	  .Value	 = NULL },
	{ .Name		 = "isFile",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _PathIsFile,
	  .Value	 = NULL },
	{ .Name		 = "isAbsolute",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _PathIsAbsolute,
	  .Value	 = NULL },
	{ .Name		 = "separator",
	  .Argc		 = 0,
	  .CFunction = (NativeFunctionCallback) _PathSeparator,
	  .Value	 = NULL },
	{ .Name = NULL }
};

Value* LoadCorePath(Interpreter* interpreter) {
	Value*	 mod	 = NewObjectValue(interpreter);
	HashMap* pathMap = CoerceToHashMap(mod);

	for (int i = 0; _PathModuleFunctions[i].Name != NULL; i++) {
		ModuleFunction func = _PathModuleFunctions[i];
		String		   hKey = func.Name;

		if (func.Value != NULL) {
			HashMapSet(pathMap, hKey, _PathModuleFunctions[i].Value);
		} else {
			HashMapSet(pathMap,
					   hKey,
					   NewNativeFunctionValue(
						   interpreter,
						   CreateNativeFunctionMeta((const String) hKey,
													func.Argc,
													func.CFunction)));
		}
	}

	return mod;
}
