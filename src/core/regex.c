#include "./regex.h"

#include "../../libregex/libregexp.h"
#include "../array.h"

#include <stdlib.h>
#include <string.h>

#define LRE_BLOB_KEY "__lre"

typedef enum {
	MODE_SEARCH	   = 0,
	MODE_MATCH	   = 1,
	MODE_FULLMATCH = 2,
} RegexMode;

/* ── libregexp embedding hooks ─────────────────────────────────────────── */

int lre_check_stack_overflow(void* opaque, size_t alloca_size) {
	(void) opaque;
	(void) alloca_size;
	return 0;
}

int lre_check_timeout(void* opaque) {
	(void) opaque;
	return 0;
}

void* lre_realloc(void* opaque, void* ptr, size_t size) {
	(void) opaque;
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	return realloc(ptr, size);
}

/* ── Internal helpers ──────────────────────────────────────────────────── */

/**
 * Advance one UTF-8 codepoint from *p, not past end.
 * Returns the new pointer, always strictly greater than the input.
 */
static const uint8_t* _Utf8Next(const uint8_t* p, const uint8_t* end) {
	if (p >= end)
		return p + 1; /* caller's guard handles this */
	/* skip continuation bytes (10xxxxxx) */
	++p;
	while (p < end && (*p & 0xC0) == 0x80)
		++p;
	return p;
}

static int _RegexParseFlags(Value* v, int* outFlags) {
	*outFlags = 0;
	if (v == NULL || ValueIsNull(v))
		return 1;
	if (ValueIsInt(v)) {
		*outFlags = CoerceToI32(v);
		return 1;
	}
	if (ValueIsNum(v)) {
		*outFlags = (int) CoerceToNum(v);
		return 1;
	}
	return 0;
}

static int _RegexGetBc(Value* rxVal, const uint8_t** out) {
	*out = NULL;
	if (!ValueIsClassInstance(rxVal))
		return 0;

	ClassInstance* inst = CoerceToClassInstance(rxVal);
	Value* bcVal = (Value*) HashMapGet(inst->Members, (String) LRE_BLOB_KEY);
	if (bcVal == NULL || !ValueIsBlob(bcVal))
		return 0;

	Blob* b = CoerceToBlob(bcVal);
	if (b == NULL || b->Data == NULL || b->Size == 0)
		return 0;

	*out = b->Data;
	return 1;
}

static Value* _RegexCompileInto(Interpreter* interpreter,
								Value*		 rxVal,
								Value*		 patternVal,
								int			 flags) {
	if (!ValueIsClassInstance(rxVal))
		return NewErrorValue(interpreter, "re: invalid regex object");
	if (!ValueIsStr(patternVal))
		return NewErrorValue(interpreter, "re: pattern must be a string");

	Rune*  patternRunes = (Rune*) patternVal->Value.Opaque;
	String pattern8		= RunesStrToString(patternRunes);
	if (pattern8 == NULL)
		return NewErrorValue(interpreter, "re: out of memory");

	char	 errorMsg[256] = { 0 };
	int		 bytecodeLen   = 0;
	uint8_t* bc			   = lre_compile(&bytecodeLen,
										 errorMsg,
										 (int) sizeof(errorMsg),
										 pattern8,
										 strlen(pattern8),
										 flags,
										 NULL);
	free(pattern8);

	if (bc == NULL)
		return NewErrorFValue(interpreter,
							  "re compile failed: %s",
							  errorMsg[0] ? errorMsg : "unknown");

	ClassInstance* inst	 = CoerceToClassInstance(rxVal);
	Value*		   blobV = NewBlobValue(interpreter,
										bc,
										(size_t) bytecodeLen,
										"application/x-lre-regexp");
	free(bc);
	HashMapSet(inst->Members, (String) LRE_BLOB_KEY, blobV);
	return rxVal;
}

static Value* _RegexSliceToStr(Interpreter*	  interpreter,
							   const uint8_t* start,
							   const uint8_t* end) {
	if (start == NULL || end == NULL || end < start)
		return interpreter->Null;

	size_t len = (size_t) (end - start);
	String tmp = Allocate(len + 1);
	if (tmp == NULL)
		return NewErrorValue(interpreter, "re: out of memory");

	memcpy(tmp, start, len);
	tmp[len]   = '\0';
	Value* out = NewStrValue(interpreter, tmp);
	free(tmp);
	return out;
}

/**
 * Run lre_exec once.  Returns 1 on success (check *outRet for match status),
 * 0 on allocation failure.  Caller owns *outCap on success.
 */
static int _RegexExecRaw(const uint8_t* bc,
						 const uint8_t* text,
						 int			textLen,
						 int			startIndex,
						 int			allocCount, /* pre-computed */
						 uint8_t***		outCap,
						 int*			outRet) {
	*outCap = NULL;
	*outRet = 0;

	uint8_t** cap = malloc(sizeof(*cap) * (size_t) allocCount);
	if (cap == NULL)
		return 0;

	memset(cap, 0, sizeof(*cap) * (size_t) allocCount);

	*outRet = lre_exec(cap, bc, text, startIndex, textLen, 0, NULL);
	*outCap = cap;
	return 1;
}

/**
 * Build a result array from a successful lre_exec cap array.
 * Returns an Error Value on OOM; otherwise a (possibly empty) Array Value.
 */
static Value*
_RegexBuildMatchArray(Interpreter* interpreter, uint8_t** cap, int capCount) {
	Value* out = NewArrayValue(interpreter);
	Array* arr = CoerceToArray(out);
	int	   n   = capCount / 2;

	for (int i = 0; i < n; i++) {
		Value* s = _RegexSliceToStr(interpreter, cap[2 * i], cap[2 * i + 1]);
		if (ValueIsError(s))
			return s; /* partial array is GC'd */
		ArrayPush(arr, s);
	}
	return out;
}

/* ── Core match logic ──────────────────────────────────────────────────── */

static Value* _RegexRunMode(Interpreter*   interpreter,
							const uint8_t* bc,
							Value*		   haystack,
							RegexMode	   mode) {
	if (!ValueIsStr(haystack))
		return NewErrorValue(interpreter, "re: haystack must be a string");

	Rune*  haystackRunes = (Rune*) haystack->Value.Opaque;
	String text			 = RunesStrToString(haystackRunes);
	if (text == NULL)
		return NewErrorValue(interpreter, "re: out of memory");

	int		  textLen  = (int) strlen(text);
	int		  allocCnt = lre_get_alloc_count(bc);
	uint8_t** cap	   = NULL;
	int		  ret	   = 0;

	if (allocCnt <= 0
		|| !_RegexExecRaw(bc,
						  (const uint8_t*) text,
						  textLen,
						  0,
						  allocCnt,
						  &cap,
						  &ret)) {
		free(text);
		return NewErrorValue(interpreter, "re: out of memory");
	}
	if (ret < 0) {
		free(cap);
		free(text);
		return NewErrorFValue(interpreter, "re exec failed: code %d", ret);
	}
	if (ret != 1) {
		free(cap);
		free(text);
		return interpreter->Null;
	}

	/* Mode guards */
	if (allocCnt >= 2) {
		const uint8_t* base = (const uint8_t*) text;
		uint8_t*	   p0	= cap[0];
		uint8_t*	   p1	= cap[1];

		if (mode == MODE_MATCH && p0 != base) {
			free(cap);
			free(text);
			return interpreter->Null;
		}
		if (mode == MODE_FULLMATCH && (p0 != base || p1 != base + textLen)) {
			free(cap);
			free(text);
			return interpreter->Null;
		}
	}

	Value* out = _RegexBuildMatchArray(interpreter, cap, allocCnt);
	free(cap);
	free(text);
	return out;
}

static Value*
_RegexFindAll(Interpreter* interpreter, const uint8_t* bc, Value* haystack) {
	if (!ValueIsStr(haystack))
		return NewErrorValue(interpreter, "re: haystack must be a string");

	Rune*  haystackRunes = (Rune*) haystack->Value.Opaque;
	String text			 = RunesStrToString(haystackRunes);
	if (text == NULL)
		return NewErrorValue(interpreter, "re: out of memory");

	int textLen = (int) strlen(text);

	/* Cache allocCount — it's constant for a given bytecode buffer. */
	int allocCnt = lre_get_alloc_count(bc);
	if (allocCnt < 2) {
		free(text);
		return NewArrayValue(interpreter);
	}
	int captureCount = allocCnt / 2;

	Value* out	  = NewArrayValue(interpreter);
	Array* outArr = CoerceToArray(out);
	int	   start  = 0;

	while (start <= textLen) {
		uint8_t** cap = NULL;
		int		  ret = 0;

		if (!_RegexExecRaw(bc,
						   (const uint8_t*) text,
						   textLen,
						   start,
						   allocCnt,
						   &cap,
						   &ret)) {
			free(text);
			return NewErrorValue(interpreter, "re: out of memory");
		}
		if (ret < 0) {
			free(cap);
			free(text);
			return NewErrorFValue(interpreter, "re exec failed: code %d", ret);
		}
		/* No match or full match degenerate — stop. */
		if (ret != 1 || cap[0] == NULL || cap[1] == NULL) {
			free(cap);
			break;
		}

		/* Build the item following Python re.findall semantics:
		 *   0 groups  → full-match string
		 *   1 group   → group-1 string  (may be Null for unmatched optional)
		 *   2+ groups → array of group strings                              */
		Value* item = NULL;
		if (captureCount <= 1) {
			item = _RegexSliceToStr(interpreter, cap[0], cap[1]);
		} else if (captureCount == 2) {
			/* cap[2]/cap[3] can be NULL for an unmatched optional group;
			   _RegexSliceToStr handles that by returning Null. */
			item = _RegexSliceToStr(interpreter, cap[2], cap[3]);
		} else {
			item		  = NewArrayValue(interpreter);
			Array* groups = CoerceToArray(item);
			for (int i = 1; i < captureCount; i++) {
				Value* gv =
					_RegexSliceToStr(interpreter, cap[2 * i], cap[2 * i + 1]);
				if (ValueIsError(gv)) {
					free(cap);
					free(text);
					return gv;
				}
				ArrayPush(groups, gv);
			}
		}
		if (ValueIsError(item)) {
			free(cap);
			free(text);
			return item;
		}
		ArrayPush(outArr, item);

		/* Advance: if the match was zero-length, step one UTF-8 codepoint
		 * forward to avoid an infinite loop and to stay on valid boundaries. */
		int next = (int) (cap[1] - (uint8_t*) text);
		free(cap);

		if (next <= start) {
			/* Zero-length match: advance past one full UTF-8 codepoint. */
			const uint8_t* base = (const uint8_t*) text;
			start = (int) (_Utf8Next(base + start, base + textLen) - base);
		} else {
			start = next;
		}
	}

	free(text);
	return out;
}

/* ── RegExp class methods ──────────────────────────────────────────────── */

static Value*
_RegexInit(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 2)
		return NewErrorValue(interpreter, "RegExp.init(pattern, flags=0)");
	int flags = 0;
	if (argc >= 3 && !_RegexParseFlags(arguments[2], &flags))
		return NewErrorValue(interpreter, "RegExp.init: flags must be numeric");
	return _RegexCompileInto(interpreter, arguments[0], arguments[1], flags);
}

static Value*
_RegexSearch(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2)
		return NewErrorValue(interpreter, "RegExp.search(text)");
	const uint8_t* bc = NULL;
	if (!_RegexGetBc(arguments[0], &bc))
		return NewErrorValue(interpreter, "RegExp object is not initialized");
	return _RegexRunMode(interpreter, bc, arguments[1], MODE_SEARCH);
}

static Value*
_RegexMatch(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2)
		return NewErrorValue(interpreter, "RegExp.match(text)");
	const uint8_t* bc = NULL;
	if (!_RegexGetBc(arguments[0], &bc))
		return NewErrorValue(interpreter, "RegExp object is not initialized");
	return _RegexRunMode(interpreter, bc, arguments[1], MODE_MATCH);
}

static Value*
_RegexFullMatch(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2)
		return NewErrorValue(interpreter, "RegExp.fullmatch(text)");
	const uint8_t* bc = NULL;
	if (!_RegexGetBc(arguments[0], &bc))
		return NewErrorValue(interpreter, "RegExp object is not initialized");
	return _RegexRunMode(interpreter, bc, arguments[1], MODE_FULLMATCH);
}

static Value*
_RegexFindAllMethod(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2)
		return NewErrorValue(interpreter, "RegExp.findall(text)");
	const uint8_t* bc = NULL;
	if (!_RegexGetBc(arguments[0], &bc))
		return NewErrorValue(interpreter, "RegExp object is not initialized");
	return _RegexFindAll(interpreter, bc, arguments[1]);
}

/* exec() is a JS-compat alias for search(). */
static Value*
_RegexExec(Interpreter* interpreter, int argc, Value** arguments) {
	return _RegexSearch(interpreter, argc, arguments);
}

/* test() avoids building the full match array — just checks for a match. */
static Value*
_RegexTest(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2)
		return NewErrorValue(interpreter, "RegExp.test(text)");
	const uint8_t* bc = NULL;
	if (!_RegexGetBc(arguments[0], &bc))
		return NewErrorValue(interpreter, "RegExp object is not initialized");
	if (!ValueIsStr(arguments[1]))
		return NewErrorValue(interpreter, "re: haystack must be a string");

	Rune*  runes = (Rune*) arguments[1]->Value.Opaque;
	String text	 = RunesStrToString(runes);
	if (text == NULL)
		return NewErrorValue(interpreter, "re: out of memory");

	int		  textLen  = (int) strlen(text);
	int		  allocCnt = lre_get_alloc_count(bc);
	uint8_t** cap	   = NULL;
	int		  ret	   = 0;
	int		  ok	   = allocCnt > 0
						 && _RegexExecRaw(bc,
										  (const uint8_t*) text,
										  textLen,
										  0,
										  allocCnt,
										  &cap,
										  &ret);
	free(text);
	if (!ok)
		return NewErrorValue(interpreter, "re: out of memory");
	if (ret < 0) {
		free(cap);
		return NewErrorFValue(interpreter, "re exec failed: code %d", ret);
	}
	int matched = (ret == 1);
	free(cap);
	return NewBoolValue(interpreter, matched);
}

/* ── Module-level (Python-style) functions ─────────────────────────────── */

static Value*
_ReCompile(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 1)
		return NewErrorValue(interpreter, "re.compile(pattern, flags=0)");
	int flags = 0;
	if (argc >= 2 && !_RegexParseFlags(arguments[1], &flags))
		return NewErrorValue(interpreter, "re.compile: flags must be numeric");

	Value*		   rxClass = CreateRegexClass(interpreter);
	ClassInstance* inst	   = CreateClassInstance(rxClass);
	Value*		   rxVal   = NewClassInstanceValue(interpreter, inst);
	return _RegexCompileInto(interpreter, rxVal, arguments[0], flags);
}

/**
 * Accept either a pre-compiled RegExp Value or a raw pattern string.
 * The compiled Value is returned so the caller can keep it rooted (preventing
 * GC from collecting the Blob before the bytecode pointer is used).
 */
static Value* _ReResolvePattern(Interpreter*	interpreter,
								Value*			patternOrRegex,
								Value*			flagsV,
								const uint8_t** outBc) {
	*outBc = NULL;
	if (_RegexGetBc(patternOrRegex, outBc))
		return patternOrRegex; /* already compiled */

	int flags = 0;
	if (!_RegexParseFlags(flagsV, &flags))
		return NewErrorValue(interpreter, "re: flags must be numeric");

	Value*		   rxClass = CreateRegexClass(interpreter);
	ClassInstance* inst	   = CreateClassInstance(rxClass);
	Value*		   rxVal   = NewClassInstanceValue(interpreter, inst);
	Value*		   compiled =
		_RegexCompileInto(interpreter, rxVal, patternOrRegex, flags);
	if (ValueIsError(compiled))
		return compiled;

	if (!_RegexGetBc(compiled, outBc))
		return NewErrorValue(interpreter, "re: failed to prepare regex");

	/* Return `compiled` so the caller keeps it on the stack (GC root). */
	return compiled;
}

static Value* _ReSearch(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 2)
		return NewErrorValue(interpreter, "re.search(pattern, text, flags=0)");
	const uint8_t* bc = NULL;
	Value*		   rx = _ReResolvePattern(interpreter,
										  arguments[0],
										  argc >= 3 ? arguments[2] : NULL,
										  &bc);
	if (ValueIsError(rx))
		return rx;
	return _RegexRunMode(interpreter, bc, arguments[1], MODE_SEARCH);
}

static Value* _ReMatch(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 2)
		return NewErrorValue(interpreter, "re.match(pattern, text, flags=0)");
	const uint8_t* bc = NULL;
	Value*		   rx = _ReResolvePattern(interpreter,
										  arguments[0],
										  argc >= 3 ? arguments[2] : NULL,
										  &bc);
	if (ValueIsError(rx))
		return rx;
	return _RegexRunMode(interpreter, bc, arguments[1], MODE_MATCH);
}

static Value*
_ReFullMatch(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 2)
		return NewErrorValue(interpreter,
							 "re.fullmatch(pattern, text, flags=0)");
	const uint8_t* bc = NULL;
	Value*		   rx = _ReResolvePattern(interpreter,
										  arguments[0],
										  argc >= 3 ? arguments[2] : NULL,
										  &bc);
	if (ValueIsError(rx))
		return rx;
	return _RegexRunMode(interpreter, bc, arguments[1], MODE_FULLMATCH);
}

static Value*
_ReFindAll(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 2)
		return NewErrorValue(interpreter, "re.findall(pattern, text, flags=0)");
	const uint8_t* bc = NULL;
	Value*		   rx = _ReResolvePattern(interpreter,
										  arguments[0],
										  argc >= 3 ? arguments[2] : NULL,
										  &bc);
	if (ValueIsError(rx))
		return rx;
	return _RegexFindAll(interpreter, bc, arguments[1]);
}

/* ── Flags object ──────────────────────────────────────────────────────── */

static Value* _LreFlagsObject(Interpreter* interpreter) {
	Value*	 obj = NewObjectValue(interpreter);
	HashMap* map = CoerceToHashMap(obj);

#define SET_FLAG(k, v) HashMapSet(map, k, NewIntValue(interpreter, v))
	SET_FLAG("GLOBAL", LRE_FLAG_GLOBAL);
	SET_FLAG("IGNORECASE", LRE_FLAG_IGNORECASE);
	SET_FLAG("MULTILINE", LRE_FLAG_MULTILINE);
	SET_FLAG("DOTALL", LRE_FLAG_DOTALL);
	SET_FLAG("UNICODE", LRE_FLAG_UNICODE);
	SET_FLAG("STICKY", LRE_FLAG_STICKY);
	SET_FLAG("INDICES", LRE_FLAG_INDICES);
	SET_FLAG("NAMED_GROUPS", LRE_FLAG_NAMED_GROUPS);
	SET_FLAG("UNICODE_SETS", LRE_FLAG_UNICODE_SETS);
	/* Python-style short aliases */
	SET_FLAG("I", LRE_FLAG_IGNORECASE);
	SET_FLAG("M", LRE_FLAG_MULTILINE);
	SET_FLAG("S", LRE_FLAG_DOTALL);
	SET_FLAG("U", LRE_FLAG_UNICODE);
#undef SET_FLAG

	return obj;
}

/* ── Class & module registration ───────────────────────────────────────── */

static ModuleFunction _RegexClassMethods[] = {
	{ CONSTRUCTOR_NAME, VARARG, (NativeFunctionCallback) _RegexInit, NULL },
	{ "search", 2, (NativeFunctionCallback) _RegexSearch, NULL },
	{ "match", 2, (NativeFunctionCallback) _RegexMatch, NULL },
	{ "fullmatch", 2, (NativeFunctionCallback) _RegexFullMatch, NULL },
	{ "findall", 2, (NativeFunctionCallback) _RegexFindAllMethod, NULL },
	{ "exec", 2, (NativeFunctionCallback) _RegexExec, NULL },
	{ "test", 2, (NativeFunctionCallback) _RegexTest, NULL },
	{ NULL }
};

Value* CreateRegexClass(Interpreter* interpreter) {
	Value* regexClass =
		NewClassValue(interpreter,
					  CreateUserClass("RegExp", interpreter->Object));
	Class* cls = CoerceToUserClass(regexClass);

	for (int i = 0; _RegexClassMethods[i].Name != NULL; i++) {
		ModuleFunction fn = _RegexClassMethods[i];
		ClassDefineMemberByString(
			cls,
			(String) fn.Name,
			NewNativeFunctionValue(
				interpreter,
				CreateNativeFunctionMeta((const String) fn.Name,
										 fn.Argc,
										 fn.CFunction)),
			false);
	}

	/* Attach a shared flags object to the class. */
	ClassDefineMemberByString(cls, "flags", _LreFlagsObject(interpreter), true);
	return regexClass;
}

Value* LoadCoreRegex(Interpreter* interpreter) {
	Value*	 module		= NewObjectValue(interpreter);
	HashMap* map		= CoerceToHashMap(module);
	Value*	 regexClass = CreateRegexClass(interpreter);
	Value*	 flags		= _LreFlagsObject(interpreter); /* one shared object */

#define REG_FN(name, fn)                                                       \
	HashMapSet(map,                                                            \
			   name,                                                           \
			   NewNativeFunctionValue(                                         \
				   interpreter,                                                \
				   CreateNativeFunctionMeta(name,                              \
											VARARG,                            \
											(NativeFunctionCallback) fn)))

	REG_FN("compile", _ReCompile);
	REG_FN("search", _ReSearch);
	REG_FN("match", _ReMatch);
	REG_FN("fullmatch", _ReFullMatch);
	REG_FN("findall", _ReFindAll);
#undef REG_FN

	HashMapSet(map, "RegExp", regexClass);
	HashMapSet(map, "Pattern", regexClass);
	HashMapSet(map, "flags", flags);

	/* Expose short flag aliases at module level. */
	HashMap* fm = CoerceToHashMap(flags);
	HashMapSet(map, "I", HashMapGet(fm, "I"));
	HashMapSet(map, "M", HashMapGet(fm, "M"));
	HashMapSet(map, "S", HashMapGet(fm, "S"));
	HashMapSet(map, "U", HashMapGet(fm, "U"));

	return module;
}