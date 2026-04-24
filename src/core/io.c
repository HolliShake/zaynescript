#include "./io.h"

static Value* _IoGenericPrint(Interpreter* interpreter,
							  int		   argc,
							  Value**	   arguments,
							  bool		   newline) {
	if (argc == 0) {
		puts(newline ? "" : "");  // or just:
		if (newline)
			putchar('\n');
		return interpreter->Null;
	}

	// ----------------------------------------------------------------
	// PASS 1: resolve all strings + measure total length
	// ----------------------------------------------------------------
	String* parts = Allocate(argc * sizeof(String));
	if (!parts)
		return interpreter->Null;
	size_t* lens = Allocate(argc * sizeof(size_t));
	if (!lens) {
		free(parts);
		return interpreter->Null;
	}

	// total = sum of lengths + (argc-1) spaces + '\0'
	size_t total = (argc - 1) + 1;
	for (int i = 0; i < argc; i++) {
		parts[i]  = ValueToString(arguments[i]);
		lens[i]	  = parts[i] ? strlen(parts[i]) : 0;
		total	 += lens[i];
	}

	// ----------------------------------------------------------------
	// PASS 2: single alloc, memcpy in
	// ----------------------------------------------------------------
	String buffer = Allocate(total);
	if (!buffer)
		goto cleanup;

	String p = buffer;
	for (int i = 0; i < argc; i++) {
		if (lens[i] > 0) {
			memcpy(p, parts[i], lens[i]);
			p += lens[i];
		}
		if (i < argc - 1)
			*p++ = ' ';
	}
	*p = '\0';

	// single write syscall — faster than printf format
	// processing
	fwrite(buffer, 1, total - 1,
		   stdout);	 // total-1 excludes '\0'
	if (newline)
		putchar('\n');

	fflush(stdout);

	free(buffer);

cleanup:
	for (int i = 0; i < argc; i++)
		free(parts[i]);
	free(parts);
	free(lens);

	return interpreter->Null;
}

static Value* _IoPrint(Interpreter* interpreter, int argc, Value** arguments) {
	return _IoGenericPrint(interpreter, argc, arguments, false);
}

static Value*
_IoPrintln(Interpreter* interpreter, int argc, Value** arguments) {
	return _IoGenericPrint(interpreter, argc, arguments, true);
}

static Value* _IoScan(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc > 1) {
		return NewErrorFValue(interpreter,
							  "%s: scan() expects 0 or 1 argument",
							  ARGUMENT_ERROR);
	}

	if (argc == 1 && !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: scan() expects a string as its argument",
							  TYPE_ERROR);
	}

	// Print the prompt message if provided
	if (argc == 1) {
		String prompt = ValueToString(arguments[0]);
		printf("%s", prompt);
		fflush(stdout);
		free(prompt);
	}

	size_t bufferSize = 1024;
	size_t totalRead  = 0;
	String buffer	  = Allocate(bufferSize);

	// Read input dynamically, expanding buffer as needed
	while (1) {
		if (fgets(buffer + totalRead, bufferSize - totalRead, stdin) == NULL) {
			if (totalRead == 0) {
				free(buffer);
				return interpreter->Null;
			}
			break;
		}

		size_t justRead	 = strlen(buffer + totalRead);
		totalRead		+= justRead;

		// Check if we hit a newline (end of input)
		if (totalRead > 0 && buffer[totalRead - 1] == '\n') {
			buffer[totalRead - 1] = '\0';
			break;
		}

		// If buffer is full and no newline, expand it
		if (totalRead >= bufferSize - 1) {
			bufferSize *= 2;
			buffer		= Reallocate(buffer, bufferSize);
		} else {
			// fgets returned but didn't fill buffer, we're done
			break;
		}
	}

	Value* result = NewStrValue(interpreter, buffer);
	free(buffer);
	return result;
}

static Value*
_IoParseNum(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: parseNum() expects exactly 1 argument",
							  ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: parseNum() expects a string as its argument",
							  TYPE_ERROR);
	}
	String str = ValueToString(arguments[0]);
	String endptr;
	double num = strtod(str, &endptr);
	free(str);

	// Check if the number can be represented as an integer
	if (num == (int) num) {
		return NewIntValue(interpreter, (int) num);
	}
	return NewNumValue(interpreter, num);
}

static Value* _IoFormat(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 1) {
		return NewErrorFValue(interpreter,
							  "%s: format() expects at least 1 argument",
							  ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(
			interpreter,
			"%s: format() expects the first argument to be a string",
			TYPE_ERROR);
	}

	String formatStr = ValueToString(arguments[0]);
	size_t formatLen = strlen(formatStr);

	// Estimate initial buffer size
	size_t bufferSize = formatLen + argc * 32;
	String buffer	  = Allocate(bufferSize);
	size_t bufferUsed = 0;

	int argIndex = 1;
	for (size_t i = 0; i < formatLen;) {
		if (formatStr[i] == '{' && formatStr[i + 1] == '}' && argIndex < argc) {
			// Insert argument string
			String argStr = ValueToString(arguments[argIndex]);
			size_t argLen = strlen(argStr);

			// Ensure buffer is large enough
			while (bufferUsed + argLen + 1 >= bufferSize) {
				bufferSize *= 2;
				buffer		= Reallocate(buffer, bufferSize);
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
				buffer		= Reallocate(buffer, bufferSize);
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

static Value*
_IoClearScreen(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 0) {
		return NewErrorFValue(interpreter,
							  "%s: clearScreen() expects 0 arguments",
							  ARGUMENT_ERROR);
	}
	printf("\x1B[2J\x1B[H");
	fflush(stdout);
	return interpreter->Null;
}

static Value*
_IoSetColor(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc > 2) {
		return NewErrorFValue(
			interpreter,
			"%s: setColor() expects 0, 1, or 2 arguments (fg, bg)",
			ARGUMENT_ERROR);
	}

	if (argc == 0) {
		printf("\x1B[0m");	// reset
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

// ----------------------------------------------------------------
// Decompiler
// ----------------------------------------------------------------
/**
 * @brief Renders a text disassembly of @a uf: prints name/argc/locals, then
 *        one line per instruction with offset and opcode operands (string
 *        operands via @c ReadString, immediates via @c ReadInt32; @c
 *        OP_LOAD_CONST also shows @c ValueToString of @a
 *        interpreter->Constants when the offset is in range).
 * @param interpreter Constant pool and formatting context; may be NULL, in
 *        which case @c OP_LOAD_CONST lines omit the parenthesized value.
 * @param uf Bytecode image to walk from IP 0 to @a uf->CodeC.
 * @return Newly allocated multiline C string, or NULL if an append failed;
 *        caller must @c free() on success.
 * @origin src/decompiler.c
 */
extern String DecompileFunction(Interpreter* interpreter, UserFunction* uf);

static Value*
_IoDecompile(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: decompile() expects exactly 1 argument",
							  ARGUMENT_ERROR);
	}
	if (!ValueIsUserFunction(arguments[0])) {
		return NewErrorFValue(
			interpreter,
			"%s: decompile() expects a function as its argument",
			TYPE_ERROR);
	}

	UserFunction* uf	 = CoerceToUserFunction(arguments[0]);
	String		  code	 = DecompileFunction(interpreter, uf);
	Value*		  result = NewStrValue(interpreter, code);
	free(code);
	return result;
}

// ----------------------------------------------------------------
// File class (Python-like io)
// ----------------------------------------------------------------

#define PROP_FILE_PTR	   "__file"
#define PROP_FILE_READABLE "__readable"
#define PROP_FILE_WRITABLE "__writable"
#define PROP_FILE_BINARY   "__binary"

static void _FileModeFlags(const char* m, bool* outRead, bool* outWrite) {
	*outRead  = false;
	*outWrite = false;
	if (!m || !*m) {
		return;
	}
	if (strchr(m, '+')) {
		*outRead  = true;
		*outWrite = true;
		return;
	}
	switch (m[0]) {
		case 'r':
			*outRead = true;
			break;
		case 'w':
		case 'a':
		case 'x':
			*outWrite = true;
			break;
		default:
			break;
	}
}

static inline FILE* _GetFilePtr(ClassInstance* inst) {
	Value* v = (Value*) HashMapGet(inst->Members, PROP_FILE_PTR);
	if (v && ValueIsOpaquePtr(v))
		return (FILE*) v->Value.Opaque;
	return NULL;
}

/* File class constructor (`init`); invoked for `new File(...)`, not as an
 * instance method. */
static Value* _FileInit(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 2) {
		return NewErrorFValue(
			interpreter,
			"%s: File init expects at least 1 argument: (path, [mode])",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[1])) {
		return NewErrorFValue(interpreter,
							  "%s: File path must be a string",
							  TYPE_ERROR);
	}
	ClassInstance* inst	 = CoerceToClassInstance(arguments[0]);
	Rune*		   runes = (Rune*) arguments[1]->Value.Opaque;
	String		   path	 = RunesStrToString(runes);

	const char* mode	= "r";
	String		modeBuf = NULL;
	if (argc >= 3) {
		if (!ValueIsStr(arguments[2])) {
			free(path);
			return NewErrorFValue(interpreter,
								  "%s: File mode must be a string",
								  TYPE_ERROR);
		}
		runes	= (Rune*) arguments[2]->Value.Opaque;
		modeBuf = RunesStrToString(runes);
		mode	= modeBuf;
	}

	FILE* fp = fopen(path, mode);
	free(path);

	if (!fp) {
		if (modeBuf)
			free(modeBuf);
		return NewErrorFValue(interpreter,
							  "%s: fopen: %s",
							  IO_ERROR,
							  strerror(errno));
	}

	bool rd, wr;
	bool isBin = (mode && strchr(mode, 'b') != NULL);
	_FileModeFlags(mode, &rd, &wr);
	if (modeBuf)
		free(modeBuf);

	HashMapSet(inst->Members, PROP_FILE_PTR, NewOpquePtrValue(interpreter, fp));
	HashMapSet(inst->Members,
			   PROP_FILE_READABLE,
			   rd ? interpreter->True : interpreter->False);
	HashMapSet(inst->Members,
			   PROP_FILE_WRITABLE,
			   wr ? interpreter->True : interpreter->False);
	HashMapSet(inst->Members,
			   PROP_FILE_BINARY,
			   isBin ? interpreter->True : interpreter->False);
	return interpreter->Null;
}

static bool _FileIsBinary(ClassInstance* inst) {
	Value* v = (Value*) HashMapGet(inst->Members, PROP_FILE_BINARY);
	return v && ValueIsBool(v) && v->Value.I32;
}

static Value* _FileRead(Interpreter* interpreter, int argc, Value** arguments) {
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	FILE*		   fp	= _GetFilePtr(inst);
	if (!fp) {
		return NewErrorFValue(interpreter,
							  "%s: I/O operation on closed file",
							  IO_ERROR);
	}

	bool	  binary = _FileIsBinary(inst);
	long long toRead = -1;
	if (argc >= 2) {
		toRead = (long long) CoerceToNum(arguments[1]);
	}

	if (toRead == 0) {
		if (binary) {
			return NewBlobValue(interpreter, (uint8_t*) "", 0, "");
		}
		return NewStrValue(interpreter, "");
	}

	// Fixed-size read
	if (toRead > 0) {
		uint8_t* buf = Allocate((size_t) toRead);
		size_t	 got = fread(buf, 1, (size_t) toRead, fp);
		if (binary) {
			Value* v = NewBlobValue(interpreter, buf, got, "");
			free(buf);
			return v;
		}
		char* sbuf = (char*) Reallocate(buf, got + 1);
		sbuf[got]  = '\0';
		Value* v   = NewStrValue(interpreter, sbuf);
		free(sbuf);
		return v;
	}

	// Read until EOF
	size_t	 cap   = 4096;
	size_t	 total = 0;
	uint8_t* buf   = Allocate(cap);
	for (;;) {
		if (total + 4096 > cap) {
			cap = (total + 4096) * 2;
			buf = Reallocate(buf, cap);
		}
		size_t n  = fread(buf + total, 1, 4096, fp);
		total	 += n;
		if (n < 4096)
			break;
	}

	if (binary) {
		Value* v = NewBlobValue(interpreter, buf, total, "");
		free(buf);
		return v;
	}
	if (total == 0) {
		free(buf);
		return NewStrValue(interpreter, "");
	}
	char* str  = Reallocate((char*) buf, total + 1);
	str[total] = '\0';
	Value* v   = NewStrValue(interpreter, str);
	free(str);
	return v;
}

static Value*
_FileReadline(Interpreter* interpreter, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	FILE*		   fp	= _GetFilePtr(inst);
	if (!fp) {
		return NewErrorFValue(interpreter,
							  "%s: I/O operation on closed file",
							  IO_ERROR);
	}

	if (_FileIsBinary(inst)) {
		return NewErrorFValue(interpreter,
							  "%s: readline() is not available in binary mode",
							  IO_ERROR);
	}

	size_t cap = 128;
	char*  buf = Allocate(cap);
	size_t len = 0;
	for (;;) {
		if (len + 1 >= cap) {
			cap *= 2;
			buf	 = Reallocate(buf, cap);
		}
		int c = fgetc(fp);
		if (c == EOF) {
			if (len == 0) {
				free(buf);
				return NewStrValue(interpreter, "");
			}
			break;
		}
		buf[len++] = (char) c;
		if (c == (int) '\n')
			break;
	}
	buf[len]	  = '\0';
	Value* result = NewStrValue(interpreter, buf);
	free(buf);
	return result;
}

static Value*
_FileReadlines(Interpreter* interpreter, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	FILE*		   fp	= _GetFilePtr(inst);
	if (!fp) {
		return NewErrorFValue(interpreter,
							  "%s: I/O operation on closed file",
							  IO_ERROR);
	}

	if (_FileIsBinary(inst)) {
		return NewErrorFValue(interpreter,
							  "%s: readlines() is not available in binary mode",
							  IO_ERROR);
	}

	Value* arrayVal = NewArrayValue(interpreter);
	Array* arr		= CoerceToArray(arrayVal);

	for (;;) {
		size_t cap	  = 128;
		char*  buf	  = Allocate(cap);
		size_t len	  = 0;
		int	   gotEof = 0;
		for (;;) {
			if (len + 1 >= cap) {
				cap *= 2;
				buf	 = Reallocate(buf, cap);
			}
			int c = fgetc(fp);
			if (c == EOF) {
				gotEof = 1;
				break;
			}
			buf[len++] = (char) c;
			if (c == (int) '\n') {
				break;
			}
		}
		if (len == 0 && gotEof) {
			free(buf);
			break;
		}
		buf[len]	= '\0';
		Value* line = NewStrValue(interpreter, buf);
		free(buf);
		ArrayPush(arr, line);
		if (gotEof) {
			break;
		}
	}

	return arrayVal;
}

/**
 * @brief Writes one value as raw bytes: Blob as its payload, anything else
 * via ValueToString (UTF-8 for strings).
 * @return null on success, error Value on failure.
 */
/** @return NULL on success, error Value on failure. */
static Value* _FileWriteOne(Interpreter* interpreter, FILE* fp, Value* v) {
	if (ValueIsBlob(v)) {
		Blob*		b = CoerceToBlob(v);
		const void* p = b->Size > 0 ? (const void*) b->Data : "";
		if (fwrite(p, 1, b->Size, fp) != b->Size) {
			return NewErrorFValue(interpreter,
								  "%s: fwrite: %s",
								  IO_ERROR,
								  strerror(errno));
		}
		return NULL;
	}

	String s = ValueToString(v);
	size_t n = strlen(s);
	size_t w = fwrite(s, 1, n, fp);
	free(s);
	if (w != n) {
		return NewErrorFValue(interpreter,
							  "%s: fwrite: %s",
							  IO_ERROR,
							  strerror(errno));
	}
	return NULL;
}

static Value*
_FileWrite(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 2) {
		return NewErrorFValue(interpreter,
							  "%s: File.write expects 1 argument",
							  ARGUMENT_ERROR);
	}
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	FILE*		   fp	= _GetFilePtr(inst);
	if (!fp) {
		return NewErrorFValue(interpreter,
							  "%s: I/O operation on closed file",
							  IO_ERROR);
	}
	Value* wv = (Value*) HashMapGet(inst->Members, PROP_FILE_WRITABLE);
	if (!wv || !ValueIsBool(wv) || !wv->Value.I32) {
		return NewErrorFValue(interpreter,
							  "%s: File is not writable",
							  IO_ERROR);
	}

	Value* v = arguments[1];
	if (_FileIsBinary(inst) && !ValueIsStr(v) && !ValueIsBlob(v)) {
		return NewErrorFValue(
			interpreter,
			"%s: write() in binary mode expects a string or blob",
			TYPE_ERROR);
	}

	Value* err = _FileWriteOne(interpreter, fp, v);
	return err != NULL ? err : interpreter->Null;
}

static Value*
_FileWritelines(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 2) {
		return NewErrorFValue(
			interpreter,
			"%s: File.writelines expects 1 argument (array of lines)",
			ARGUMENT_ERROR);
	}
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	FILE*		   fp	= _GetFilePtr(inst);
	if (!fp) {
		return NewErrorFValue(interpreter,
							  "%s: I/O operation on closed file",
							  IO_ERROR);
	}
	Value* wv = (Value*) HashMapGet(inst->Members, PROP_FILE_WRITABLE);
	if (!wv || !ValueIsBool(wv) || !wv->Value.I32) {
		return NewErrorFValue(interpreter,
							  "%s: File is not writable",
							  IO_ERROR);
	}
	if (!ValueIsArray(arguments[1])) {
		return NewErrorFValue(interpreter,
							  "%s: writelines() expects an array",
							  TYPE_ERROR);
	}
	Array* arr = CoerceToArray(arguments[1]);
	for (size_t i = 0; i < ArrayLength(arr); i++) {
		Value* line = ArrayGet(arr, i);
		if (_FileIsBinary(inst) && !ValueIsStr(line) && !ValueIsBlob(line)) {
			return NewErrorFValue(interpreter,
								  "%s: writelines() in binary mode: each "
								  "element must be a string "
								  "or blob",
								  TYPE_ERROR);
		}
		Value* err = _FileWriteOne(interpreter, fp, line);
		if (err != NULL) {
			return err;
		}
	}
	return interpreter->Null;
}

static Value*
_FileFlush(Interpreter* interpreter, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	FILE*		   fp	= _GetFilePtr(inst);
	if (!fp) {
		return NewErrorFValue(interpreter,
							  "%s: I/O operation on closed file",
							  IO_ERROR);
	}
	if (fflush(fp) != 0) {
		return NewErrorFValue(interpreter,
							  "%s: fflush: %s",
							  IO_ERROR,
							  strerror(errno));
	}
	return interpreter->Null;
}

static Value*
_FileClose(Interpreter* interpreter, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	FILE*		   fp	= _GetFilePtr(inst);
	if (fp) {
		if (fclose(fp) == EOF) {
			return NewErrorFValue(interpreter,
								  "%s: fclose: %s",
								  IO_ERROR,
								  strerror(errno));
		}
		HashMapSet(inst->Members,
				   PROP_FILE_PTR,
				   NewOpquePtrValue(interpreter, NULL));
	}
	return interpreter->Null;
}

static Value* _FileTell(Interpreter* interpreter, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	FILE*		   fp	= _GetFilePtr(inst);
	if (!fp) {
		return NewErrorFValue(interpreter,
							  "%s: I/O operation on closed file",
							  IO_ERROR);
	}
	long pos = ftell(fp);
	if (pos < 0) {
		return NewErrorFValue(interpreter,
							  "%s: ftell: %s",
							  IO_ERROR,
							  strerror(errno));
	}
	return NewNumValue(interpreter, (double) pos);
}

static Value* _FileSeek(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 2) {
		return NewErrorFValue(
			interpreter,
			"%s: File.seek expects at least 1 argument (offset, [whence])",
			ARGUMENT_ERROR);
	}
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	FILE*		   fp	= _GetFilePtr(inst);
	if (!fp) {
		return NewErrorFValue(interpreter,
							  "%s: I/O operation on closed file",
							  IO_ERROR);
	}
	long offset = (long) CoerceToNum(arguments[1]);
	int	 whence = SEEK_SET;
	if (argc >= 3) {
		int w = (int) CoerceToNum(arguments[2]);
		if (w == 0)
			whence = SEEK_SET;
		else if (w == 1)
			whence = SEEK_CUR;
		else if (w == 2)
			whence = SEEK_END;
		else {
			return NewErrorFValue(
				interpreter,
				"%s: seek: whence must be 0 (SEEK_SET), 1 (SEEK_CUR), or 2 "
				"(SEEK_END)",
				ARGUMENT_ERROR);
		}
	}
	if (fseek(fp, offset, whence) != 0) {
		return NewErrorFValue(interpreter,
							  "%s: fseek: %s",
							  IO_ERROR,
							  strerror(errno));
	}
	return interpreter->Null;
}

static Value*
_FileReadable(Interpreter* interpreter, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	Value*		   v = (Value*) HashMapGet(inst->Members, PROP_FILE_READABLE);
	if (!v) {
		return interpreter->False;
	}
	return v;
}

static Value*
_FileWritable(Interpreter* interpreter, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* inst = CoerceToClassInstance(arguments[0]);
	Value*		   v = (Value*) HashMapGet(inst->Members, PROP_FILE_WRITABLE);
	if (!v) {
		return interpreter->False;
	}
	return v;
}

static Value*
_FileIsClosed(Interpreter* interpreter, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* inst	  = CoerceToClassInstance(arguments[0]);
	bool		   closed = _GetFilePtr(inst) == NULL;
	return NewBoolValue(interpreter, closed ? 1 : 0);
}

static ModuleFunction _IoModuleFunctions[] = {
	// print
	{ .Name		 = "print",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) (_IoPrint),
	  .Value	 = NULL },
	// println
	{ .Name		 = "println",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) (_IoPrintln),
	  .Value	 = NULL },
	// scan
	{ .Name		 = "scan",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) (_IoScan),
	  .Value	 = NULL },
	// parse num
	{ .Name		 = "parseNum",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) (_IoParseNum),
	  .Value	 = NULL },
	// format
	{ .Name		 = "format",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) (_IoFormat),
	  .Value	 = NULL },
	// clearScreen
	{ .Name		 = "clearScreen",
	  .Argc		 = 0,
	  .CFunction = (NativeFunctionCallback) (_IoClearScreen),
	  .Value	 = NULL },
	// setColor
	{ .Name		 = "setColor",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) (_IoSetColor),
	  .Value	 = NULL },
	// decompile
	{
		.Name	   = "decompile",
		.Argc	   = 1,
		.CFunction = (NativeFunctionCallback) (_IoDecompile),
	},
	// end of module functions
	{ .Name = NULL }
};

static ModuleFunction _IoFileClassFunctions[] = {
	{ .Name		 = CONSTRUCTOR_NAME,
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _FileInit,
	  .Value	 = NULL },
	{ .Name		 = "read",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _FileRead,
	  .Value	 = NULL },
	{ .Name		 = "readline",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _FileReadline,
	  .Value	 = NULL },
	{ .Name		 = "readlines",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _FileReadlines,
	  .Value	 = NULL },
	{ .Name		 = "write",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _FileWrite,
	  .Value	 = NULL },
	{ .Name		 = "writelines",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _FileWritelines,
	  .Value	 = NULL },
	{ .Name		 = "flush",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _FileFlush,
	  .Value	 = NULL },
	{ .Name		 = "close",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _FileClose,
	  .Value	 = NULL },
	{ .Name		 = "tell",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _FileTell,
	  .Value	 = NULL },
	{ .Name		 = "seek",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _FileSeek,
	  .Value	 = NULL },
	{ .Name		 = "readable",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _FileReadable,
	  .Value	 = NULL },
	{ .Name		 = "writable",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _FileWritable,
	  .Value	 = NULL },
	{ .Name		 = "isClosed",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _FileIsClosed,
	  .Value	 = NULL },
	{ .Name = NULL }
};

static Value* CreateFileClass(Interpreter* interpreter) {
	Value* fileClass =
		NewClassValue(interpreter,
					  CreateUserClass("File", interpreter->Object));
	Class* cls = CoerceToUserClass(fileClass);

	for (int i = 0; _IoFileClassFunctions[i].Name != NULL; i++) {
		ModuleFunction func = _IoFileClassFunctions[i];
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

	return fileClass;
}

Value* LoadCoreIo(Interpreter* interpreter) {
	Value*	 ioModule = NewObjectValue(interpreter);
	HashMap* ioMap	  = CoerceToHashMap(ioModule);

	for (int i = 0; _IoModuleFunctions[i].Name != NULL; i++) {
		ModuleFunction func = _IoModuleFunctions[i];
		String		   hKey = func.Name;

		if (func.Value != NULL) {
			HashMapSet(ioMap, hKey, _IoModuleFunctions[i].Value);
		} else {
			HashMapSet(ioMap,
					   hKey,
					   NewNativeFunctionValue(
						   interpreter,
						   CreateNativeFunctionMeta((const String) hKey,
													func.Argc,
													func.CFunction)));
		}
	}

	HashMapSet(ioMap, "File", CreateFileClass(interpreter));

	return ioModule;
}