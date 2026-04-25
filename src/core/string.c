/**
 * @file string.c
 * @brief Implements @c core:string: utf8proc-backed helpers for script strings
 *        (stored as NUL-terminated rune arrays inside @c VLT_STR values).
 */

#include "./string.h"

static size_t _RuneLen(const Rune* runes) {
	size_t n = 0;
	if (runes == NULL) {
		return 0;
	}
	while (runes[n] != 0) {
		++n;
	}
	return n;
}

static int _RuneCategory(Rune r) {
	return (int) utf8proc_category((utf8proc_int32_t) r);
}

static bool _IsSpaceRune(Rune r) {
	int c = _RuneCategory(r);
	return c == UTF8PROC_CATEGORY_ZS || c == UTF8PROC_CATEGORY_ZL
		   || c == UTF8PROC_CATEGORY_ZP;
}

static bool _IsAlphaCat(int cat) {
	return cat == UTF8PROC_CATEGORY_LU || cat == UTF8PROC_CATEGORY_LL
		   || cat == UTF8PROC_CATEGORY_LT || cat == UTF8PROC_CATEGORY_LM
		   || cat == UTF8PROC_CATEGORY_LO;
}

static bool _IsAlnumCat(int cat) {
	return _IsAlphaCat(cat) || cat == UTF8PROC_CATEGORY_ND
		   || cat == UTF8PROC_CATEGORY_NL || cat == UTF8PROC_CATEGORY_NO;
}

static Value*
_StrFromRunesOwned(Interpreter* interpreter, Rune* runes, const char* ctx) {
	if (runes == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: string: %s failed (invalid text)",
							  RUNTIME_ERROR,
							  ctx);
	}
	String utf8 = RunesStrToString(runes);
	free(runes);
	if (utf8 == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: string: UTF-8 encode failed in %s",
							  RUNTIME_ERROR,
							  ctx);
	}
	Value* out = NewStrValue(interpreter, utf8);
	free(utf8);
	return out;
}

static Rune* _RunesFromStrValue(Value* v) {
	if (!ValueIsStr(v)) {
		return NULL;
	}
	return (Rune*) v->Value.Opaque;
}

static Value*
_StringToUpper(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: toUpper() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes = _RunesFromStrValue(arguments[0]);
	size_t n	 = _RuneLen(runes);
	Rune*  out	 = Allocate(sizeof(Rune) * (n + 1));
	for (size_t i = 0; i < n; i++) {
		out[i] = (Rune) utf8proc_toupper((utf8proc_int32_t) runes[i]);
	}
	out[n] = 0;
	return _StrFromRunesOwned(interpreter, out, "toUpper");
}

static Value*
_StringToLower(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: toLower() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes = _RunesFromStrValue(arguments[0]);
	size_t n	 = _RuneLen(runes);
	Rune*  out	 = Allocate(sizeof(Rune) * (n + 1));
	for (size_t i = 0; i < n; i++) {
		out[i] = (Rune) utf8proc_tolower((utf8proc_int32_t) runes[i]);
	}
	out[n] = 0;
	return _StrFromRunesOwned(interpreter, out, "toLower");
}

static Value*
_StringCapitalize(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: capitalize() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes = _RunesFromStrValue(arguments[0]);
	size_t n	 = _RuneLen(runes);
	Rune*  out	 = Allocate(sizeof(Rune) * (n + 1));
	if (n == 0) {
		out[0] = 0;
		return _StrFromRunesOwned(interpreter, out, "capitalize");
	}
	out[0] = (Rune) utf8proc_totitle((utf8proc_int32_t) runes[0]);
	for (size_t i = 1; i < n; i++) {
		out[i] = (Rune) utf8proc_tolower((utf8proc_int32_t) runes[i]);
	}
	out[n] = 0;
	return _StrFromRunesOwned(interpreter, out, "capitalize");
}

static Value*
_StringIsIdentifier(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: isIdentifier() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes = _RunesFromStrValue(arguments[0]);
	size_t n	 = _RuneLen(runes);
	if (n == 0) {
		return interpreter->False;
	}
	int c0 = _RuneCategory(runes[0]);
	if (!(runes[0] == (Rune) '_' || _IsAlphaCat(c0))) {
		return interpreter->False;
	}
	for (size_t i = 1; i < n; i++) {
		int c = _RuneCategory(runes[i]);
		if (!(runes[i] == (Rune) '_' || _IsAlnumCat(c))) {
			return interpreter->False;
		}
	}
	return interpreter->True;
}

static Value*
_StringIsAlpha(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: isAlpha() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes = _RunesFromStrValue(arguments[0]);
	size_t n	 = _RuneLen(runes);
	if (n == 0) {
		return interpreter->False;
	}
	for (size_t i = 0; i < n; i++) {
		if (!_IsAlphaCat(_RuneCategory(runes[i]))) {
			return interpreter->False;
		}
	}
	return interpreter->True;
}

static Value*
_StringIsDigit(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: isDigit() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes = _RunesFromStrValue(arguments[0]);
	size_t n	 = _RuneLen(runes);
	if (n == 0) {
		return interpreter->False;
	}
	for (size_t i = 0; i < n; i++) {
		if (_RuneCategory(runes[i]) != UTF8PROC_CATEGORY_ND) {
			return interpreter->False;
		}
	}
	return interpreter->True;
}

static Value*
_StringIsAlnum(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: isAlnum() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes = _RunesFromStrValue(arguments[0]);
	size_t n	 = _RuneLen(runes);
	if (n == 0) {
		return interpreter->False;
	}
	for (size_t i = 0; i < n; i++) {
		if (!_IsAlnumCat(_RuneCategory(runes[i]))) {
			return interpreter->False;
		}
	}
	return interpreter->True;
}

static Value*
_StringSplit(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 1 || argc > 2) {
		return NewErrorFValue(
			interpreter,
			"%s: split() expects 1 or 2 arguments (string, optional delimiter)",
			ARGUMENT_ERROR);
	}
	if (!ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: split() first argument must be a string",
							  TYPE_ERROR);
	}
	if (argc == 2 && !ValueIsStr(arguments[1])) {
		return NewErrorFValue(interpreter,
							  "%s: split() delimiter must be a string",
							  TYPE_ERROR);
	}

	Value* arrVal = NewArrayValue(interpreter);
	Array* arr	  = CoerceToArray(arrVal);
	Rune*  runes  = _RunesFromStrValue(arguments[0]);
	size_t n	  = _RuneLen(runes);

	if (argc == 1) {
		size_t i = 0;
		while (i < n && _IsSpaceRune(runes[i])) {
			++i;
		}
		while (i < n) {
			size_t start = i;
			while (i < n && !_IsSpaceRune(runes[i])) {
				++i;
			}
			Rune* piece = Allocate(sizeof(Rune) * (i - start + 1));
			for (size_t j = 0; j < i - start; j++) {
				piece[j] = runes[start + j];
			}
			piece[i - start] = 0;
			Value* part		 = _StrFromRunesOwned(interpreter, piece, "split");
			if (ValueIsError(part)) {
				return part;
			}
			ArrayPush(arr, part);
			while (i < n && _IsSpaceRune(runes[i])) {
				++i;
			}
		}
		return arrVal;
	}
	String hay = ValueToString(arguments[0]);
	String del = ValueToString(arguments[1]);
	if (hay == NULL || del == NULL) {
		free(hay);
		free(del);
		return NewErrorFValue(interpreter,
							  "%s: split() out of memory",
							  MEMORY_ERROR);
	}
	size_t dlen = strlen(del);
	if (dlen == 0) {
		free(hay);
		free(del);
		return NewErrorFValue(
			interpreter,
			"%s: split() delimiter must be a non-empty string",
			ARGUMENT_ERROR);
	}

	char* cur = hay;
	while (*cur != '\0') {
		char* hit = strstr(cur, del);
		if (hit == NULL) {
			Value* part = NewStrValue(interpreter, cur);
			ArrayPush(arr, part);
			break;
		}
		*hit		= '\0';
		Value* part = NewStrValue(interpreter, cur);
		ArrayPush(arr, part);
		cur = hit + dlen;
	}
	free(hay);
	free(del);
	return arrVal;
}

static Value*
_StringOrd(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: ord() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes = _RunesFromStrValue(arguments[0]);
	size_t n	 = _RuneLen(runes);
	if (n != 1) {
		return NewErrorFValue(
			interpreter,
			"%s: ord() expects a string containing exactly one codepoint",
			ARGUMENT_ERROR);
	}
	return NewIntValue(interpreter, (int) runes[0]);
}

static Value*
_StringChr(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: chr() expects 1 integer (codepoint)",
							  ARGUMENT_ERROR);
	}
	if (!ValueIsInt(arguments[0]) && !ValueIsNum(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: chr() expects a number",
							  TYPE_ERROR);
	}
	int cp = CoerceToI32(arguments[0]);
	if (!utf8proc_codepoint_valid((utf8proc_int32_t) cp)) {
		return NewErrorFValue(interpreter,
							  "%s: chr() invalid Unicode codepoint",
							  ARGUMENT_ERROR);
	}
	Rune   buf[2] = { (Rune) cp, 0 };
	String utf8	  = RunesStrToString(buf);
	if (utf8 == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: chr() encode failed",
							  RUNTIME_ERROR);
	}
	Value* out = NewStrValue(interpreter, utf8);
	free(utf8);
	return out;
}

static Value*
_StringBytes(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: bytes() expects 1 string",
							  TYPE_ERROR);
	}
	String u8 = ValueToString(arguments[0]);
	if (u8 == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: bytes() out of memory",
							  MEMORY_ERROR);
	}
	Value* arrVal = NewArrayValue(interpreter);
	Array* arr	  = CoerceToArray(arrVal);
	for (const unsigned char* p = (const unsigned char*) u8; *p != '\0'; ++p) {
		ArrayPush(arr, NewIntValue(interpreter, (int) *p));
	}
	free(u8);
	return arrVal;
}

static Value*
_StringCodepoints(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: codepoints() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes  = _RunesFromStrValue(arguments[0]);
	size_t n	  = _RuneLen(runes);
	Value* arrVal = NewArrayValue(interpreter);
	Array* arr	  = CoerceToArray(arrVal);
	for (size_t i = 0; i < n; i++) {
		ArrayPush(arr, NewIntValue(interpreter, (int) runes[i]));
	}
	return arrVal;
}

static Value*
_StringStrip(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: strip() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes = _RunesFromStrValue(arguments[0]);
	size_t n	 = _RuneLen(runes);
	size_t lo	 = 0;
	while (lo < n && _IsSpaceRune(runes[lo])) {
		++lo;
	}
	size_t hi = n;
	while (hi > lo && _IsSpaceRune(runes[hi - 1])) {
		--hi;
	}
	Rune* out = Allocate(sizeof(Rune) * (hi - lo + 1));
	for (size_t i = lo; i < hi; i++) {
		out[i - lo] = runes[i];
	}
	out[hi - lo] = 0;
	return _StrFromRunesOwned(interpreter, out, "strip");
}

static Value*
_StringJoin(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(
			interpreter,
			"%s: join() expects 2 arguments (array, separator)",
			ARGUMENT_ERROR);
	}
	if (!ValueIsArray(arguments[0]) || !ValueIsStr(arguments[1])) {
		return NewErrorFValue(interpreter,
							  "%s: join() expects (array, string)",
							  TYPE_ERROR);
	}
	Array* arr = CoerceToArray(arguments[0]);
	String sep = ValueToString(arguments[1]);
	if (sep == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: join() out of memory",
							  MEMORY_ERROR);
	}
	size_t n = ArrayLength(arr);
	if (n == 0) {
		free(sep);
		return NewStrValue(interpreter, "");
	}

	size_t	sepLen = strlen(sep);
	size_t	total  = 0;
	String* parts  = Allocate(sizeof(String) * n);
	for (size_t i = 0; i < n; i++) {
		Value* el = (Value*) ArrayGet(arr, i);
		if (!ValueIsStr(el)) {
			for (size_t j = 0; j < i; j++) {
				free(parts[j]);
			}
			free(parts);
			free(sep);
			return NewErrorFValue(interpreter,
								  "%s: join() array must contain only strings",
								  TYPE_ERROR);
		}
		parts[i] = ValueToString(el);
		if (parts[i] == NULL) {
			for (size_t j = 0; j < i; j++) {
				free(parts[j]);
			}
			free(parts);
			free(sep);
			return NewErrorFValue(interpreter,
								  "%s: join() out of memory",
								  MEMORY_ERROR);
		}
		total += strlen(parts[i]);
		if (i + 1 < n) {
			total += sepLen;
		}
	}

	String out = Allocate(total + 1);
	if (out == NULL) {
		for (size_t i = 0; i < n; i++) {
			free(parts[i]);
		}
		free(parts);
		free(sep);
		return NewErrorFValue(interpreter,
							  "%s: join() out of memory",
							  MEMORY_ERROR);
	}
	size_t pos = 0;
	for (size_t i = 0; i < n; i++) {
		if (i > 0 && sepLen > 0) {
			memcpy(out + pos, sep, sepLen);
			pos += sepLen;
		}
		size_t L = strlen(parts[i]);
		memcpy(out + pos, parts[i], L);
		pos += L;
		free(parts[i]);
	}
	free(parts);
	free(sep);
	out[pos]   = '\0';
	Value* ret = NewStrValue(interpreter, out);
	free(out);
	return ret;
}

static Value*
_StringRepeat(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2 || !ValueIsStr(arguments[0])
		|| (!ValueIsInt(arguments[1]) && !ValueIsNum(arguments[1]))) {
		return NewErrorFValue(interpreter,
							  "%s: repeat() expects (string, count)",
							  TYPE_ERROR);
	}
	int count = CoerceToI32(arguments[1]);
	if (count < 0) {
		return NewErrorFValue(interpreter,
							  "%s: repeat() count must be non-negative",
							  ARGUMENT_ERROR);
	}
	String s = ValueToString(arguments[0]);
	if (s == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: repeat() out of memory",
							  MEMORY_ERROR);
	}
	size_t slen = strlen(s);
	if (count == 0 || slen == 0) {
		free(s);
		return NewStrValue(interpreter, "");
	}
	if (slen > SIZE_MAX / (size_t) count) {
		free(s);
		return NewErrorFValue(interpreter,
							  "%s: repeat() result too large",
							  ARGUMENT_ERROR);
	}
	size_t total = slen * (size_t) count;
	String out	 = Allocate(total + 1);
	if (out == NULL) {
		free(s);
		return NewErrorFValue(interpreter,
							  "%s: repeat() out of memory",
							  MEMORY_ERROR);
	}
	for (int i = 0; i < count; i++) {
		memcpy(out + (size_t) i * slen, s, slen);
	}
	out[total] = '\0';
	free(s);
	Value* ret = NewStrValue(interpreter, out);
	free(out);
	return ret;
}

static Value*
_StringReverse(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: reverse() expects 1 string",
							  TYPE_ERROR);
	}
	Rune*  runes = _RunesFromStrValue(arguments[0]);
	size_t n	 = _RuneLen(runes);
	Rune*  out	 = Allocate(sizeof(Rune) * (n + 1));
	for (size_t i = 0; i < n; i++) {
		out[i] = runes[n - 1 - i];
	}
	out[n] = 0;
	return _StrFromRunesOwned(interpreter, out, "reverse");
}

static Value*
_StringContains(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2 || !ValueIsStr(arguments[0]) || !ValueIsStr(arguments[1])) {
		return NewErrorFValue(interpreter,
							  "%s: contains() expects 2 strings",
							  TYPE_ERROR);
	}
	String a = ValueToString(arguments[0]);
	String b = ValueToString(arguments[1]);
	if (a == NULL || b == NULL) {
		free(a);
		free(b);
		return NewErrorFValue(interpreter,
							  "%s: contains() out of memory",
							  MEMORY_ERROR);
	}
	bool ok = strstr(a, b) != NULL;
	free(a);
	free(b);
	return ok ? interpreter->True : interpreter->False;
}

static Value*
_StringStartsWith(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2 || !ValueIsStr(arguments[0]) || !ValueIsStr(arguments[1])) {
		return NewErrorFValue(interpreter,
							  "%s: startsWith() expects 2 strings",
							  TYPE_ERROR);
	}
	String a = ValueToString(arguments[0]);
	String p = ValueToString(arguments[1]);
	if (a == NULL || p == NULL) {
		free(a);
		free(p);
		return NewErrorFValue(interpreter,
							  "%s: startsWith() out of memory",
							  MEMORY_ERROR);
	}
	bool ok = StringStartsWith(a, p);
	free(a);
	free(p);
	return ok ? interpreter->True : interpreter->False;
}

static Value*
_StringEndsWith(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2 || !ValueIsStr(arguments[0]) || !ValueIsStr(arguments[1])) {
		return NewErrorFValue(interpreter,
							  "%s: endsWith() expects 2 strings",
							  TYPE_ERROR);
	}
	String a = ValueToString(arguments[0]);
	String s = ValueToString(arguments[1]);
	if (a == NULL || s == NULL) {
		free(a);
		free(s);
		return NewErrorFValue(interpreter,
							  "%s: endsWith() out of memory",
							  MEMORY_ERROR);
	}
	size_t la = strlen(a);
	size_t ls = strlen(s);
	bool   ok = la >= ls && memcmp(a + la - ls, s, ls) == 0;
	free(a);
	free(s);
	return ok ? interpreter->True : interpreter->False;
}

static Value*
_StringReplace(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 3 || !ValueIsStr(arguments[0]) || !ValueIsStr(arguments[1])
		|| !ValueIsStr(arguments[2])) {
		return NewErrorFValue(
			interpreter,
			"%s: replace() expects 3 strings (haystack, from, to)",
			TYPE_ERROR);
	}
	String src = ValueToString(arguments[0]);
	String old = ValueToString(arguments[1]);
	String nw  = ValueToString(arguments[2]);
	if (src == NULL || old == NULL || nw == NULL) {
		free(src);
		free(old);
		free(nw);
		return NewErrorFValue(interpreter,
							  "%s: replace() out of memory",
							  MEMORY_ERROR);
	}
	size_t olen = strlen(old);
	if (olen == 0) {
		free(src);
		free(old);
		free(nw);
		return NewErrorFValue(interpreter,
							  "%s: replace() second argument must be non-empty",
							  ARGUMENT_ERROR);
	}
	size_t nlen = strlen(nw);
	size_t cap	= strlen(src) + 64;
	char*  out	= Allocate(cap);
	if (out == NULL) {
		free(src);
		free(old);
		free(nw);
		return NewErrorFValue(interpreter,
							  "%s: replace() out of memory",
							  MEMORY_ERROR);
	}
	size_t o = 0;
	char*  r = src;
	while (*r != '\0') {
		char* hit = strstr(r, old);
		if (hit == NULL) {
			size_t tail = strlen(r);
			if (o + tail + 1 > cap) {
				cap		   = (o + tail + 1) * 2;
				char* nout = Reallocate(out, cap);
				if (nout == NULL) {
					free(out);
					free(src);
					free(old);
					free(nw);
					return NewErrorFValue(interpreter,
										  "%s: replace() out of memory",
										  MEMORY_ERROR);
				}
				out = nout;
			}
			memcpy(out + o, r, tail);
			o += tail;
			break;
		}
		size_t chunk = (size_t) (hit - r);
		if (o + chunk + nlen + 1 > cap) {
			while (o + chunk + nlen + 1 > cap) {
				cap *= 2;
			}
			char* nout = Reallocate(out, cap);
			if (nout == NULL) {
				free(out);
				free(src);
				free(old);
				free(nw);
				return NewErrorFValue(interpreter,
									  "%s: replace() out of memory",
									  MEMORY_ERROR);
			}
			out = nout;
		}
		memcpy(out + o, r, chunk);
		o += chunk;
		memcpy(out + o, nw, nlen);
		o += nlen;
		r  = hit + olen;
	}
	out[o] = '\0';
	free(src);
	free(old);
	free(nw);
	Value* ret = NewStrValue(interpreter, out);
	free(out);
	return ret;
}

static Value*
_StringByteLength(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: byteLength() expects 1 string",
							  TYPE_ERROR);
	}
	String u8 = ValueToString(arguments[0]);
	if (u8 == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: byteLength() out of memory",
							  MEMORY_ERROR);
	}
	int len = (int) strlen(u8);
	free(u8);
	return NewIntValue(interpreter, len);
}

static const char _StringB64Table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int _StringB64FromChar(int c) {
	if (c >= 'A' && c <= 'Z') {
		return c - 'A';
	}
	if (c >= 'a' && c <= 'z') {
		return c - 'a' + 26;
	}
	if (c >= '0' && c <= '9') {
		return c - '0' + 52;
	}
	if (c == '+') {
		return 62;
	}
	if (c == '/') {
		return 63;
	}
	return -1;
}

static bool _StringUtf8WellFormed(const uint8_t* p, size_t len) {
	size_t i = 0;
	while (i < len) {
		utf8proc_int32_t cp;
		utf8proc_ssize_t d =
			utf8proc_iterate(p + i, (utf8proc_ssize_t) (len - i), &cp);
		if (d <= 0 || cp < 0) {
			return false;
		}
		i += (size_t) d;
	}
	return true;
}

static Value*
_StringEncode(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(
			interpreter,
			"%s: encode() expects 1 argument (string or array)",
			ARGUMENT_ERROR);
	}

	uint8_t* raw = NULL;
	size_t	 n	 = 0;

	if (ValueIsStr(arguments[0])) {
		String u8 = ValueToString(arguments[0]);
		if (u8 == NULL) {
			return NewErrorFValue(interpreter,
								  "%s: encode() out of memory",
								  MEMORY_ERROR);
		}
		n	= strlen(u8);
		raw = (uint8_t*) u8;
	} else if (ValueIsArray(arguments[0])) {
		Array* arr = CoerceToArray(arguments[0]);
		n		   = ArrayLength(arr);
		raw		   = Allocate(n == 0 ? 1 : n);
		if (raw == NULL) {
			return NewErrorFValue(interpreter,
								  "%s: encode() out of memory",
								  MEMORY_ERROR);
		}
		for (size_t i = 0; i < n; i++) {
			Value* el = (Value*) ArrayGet(arr, i);
			if (el == NULL || (!ValueIsInt(el) && !ValueIsNum(el))) {
				free(raw);
				return NewErrorFValue(interpreter,
									  "%s: encode() array must contain numbers",
									  TYPE_ERROR);
			}
			int v = CoerceToI32(el);
			if (v < 0 || v > 255) {
				free(raw);
				return NewErrorFValue(
					interpreter,
					"%s: encode() each byte must be in range 0..255",
					ARGUMENT_ERROR);
			}
			raw[i] = (uint8_t) v;
		}
	} else {
		return NewErrorFValue(interpreter,
							  "%s: encode() expects a string or array of bytes",
							  TYPE_ERROR);
	}

	size_t outCap = 4 * ((n + 2) / 3) + 1;
	String out	  = Allocate(outCap);
	if (out == NULL) {
		if (ValueIsStr(arguments[0])) {
			free((String) raw);
		} else {
			free(raw);
		}
		return NewErrorFValue(interpreter,
							  "%s: encode() out of memory",
							  MEMORY_ERROR);
	}

	size_t j = 0;
	for (size_t i = 0; i < n; i += 3) {
		uint32_t v = (uint32_t) raw[i] << 16;
		if (i + 1 < n) {
			v |= (uint32_t) raw[i + 1] << 8;
		}
		if (i + 2 < n) {
			v |= (uint32_t) raw[i + 2];
		}
		out[j++] = _StringB64Table[(v >> 18) & 63];
		out[j++] = _StringB64Table[(v >> 12) & 63];
		out[j++] = (i + 1 < n) ? _StringB64Table[(v >> 6) & 63] : (char) '=';
		out[j++] = (i + 2 < n) ? _StringB64Table[v & 63] : (char) '=';
	}
	out[j] = '\0';

	if (ValueIsStr(arguments[0])) {
		free((String) raw);
	} else {
		free(raw);
	}

	Value* ret = NewStrValue(interpreter, out);
	free(out);
	return ret;
}

static Value*
_StringDecode(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: decode() expects 1 string (Base64)",
							  TYPE_ERROR);
	}

	String s = ValueToString(arguments[0]);
	if (s == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: decode() out of memory",
							  MEMORY_ERROR);
	}

	size_t slen = strlen(s);
	size_t clen = 0;
	for (size_t i = 0; i < slen; i++) {
		unsigned char c = (unsigned char) s[i];
		if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
			continue;
		}
		clen++;
	}

	if (clen % 4 != 0) {
		free(s);
		return NewErrorFValue(
			interpreter,
			"%s: decode() invalid Base64 length (after ignoring whitespace)",
			ARGUMENT_ERROR);
	}

	char* clean = Allocate(clen + 1);
	if (clean == NULL) {
		free(s);
		return NewErrorFValue(interpreter,
							  "%s: decode() out of memory",
							  MEMORY_ERROR);
	}
	size_t w = 0;
	for (size_t i = 0; i < slen; i++) {
		unsigned char c = (unsigned char) s[i];
		if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
			continue;
		}
		clean[w++] = (char) c;
	}
	clean[w] = '\0';
	free(s);

	size_t	 binCap = (clen / 4) * 3;
	uint8_t* bin	= Allocate(binCap == 0 ? 1 : binCap);
	if (bin == NULL) {
		free(clean);
		return NewErrorFValue(interpreter,
							  "%s: decode() out of memory",
							  MEMORY_ERROR);
	}

	size_t out = 0;
	for (size_t j = 0; j < clen; j += 4) {
		int n0 = _StringB64FromChar((unsigned char) clean[j]);
		int n1 = _StringB64FromChar((unsigned char) clean[j + 1]);
		if (n0 < 0 || n1 < 0) {
			free(clean);
			free(bin);
			return NewErrorFValue(interpreter,
								  "%s: decode() invalid Base64 character",
								  ARGUMENT_ERROR);
		}
		int n2 = -1;
		int n3 = -1;
		if (clean[j + 2] != '=') {
			n2 = _StringB64FromChar((unsigned char) clean[j + 2]);
			if (n2 < 0) {
				free(clean);
				free(bin);
				return NewErrorFValue(interpreter,
									  "%s: decode() invalid Base64 character",
									  ARGUMENT_ERROR);
			}
		}
		if (clean[j + 3] != '=') {
			n3 = _StringB64FromChar((unsigned char) clean[j + 3]);
			if (n3 < 0) {
				free(clean);
				free(bin);
				return NewErrorFValue(interpreter,
									  "%s: decode() invalid Base64 character",
									  ARGUMENT_ERROR);
			}
		}

		uint32_t pack = ((uint32_t) n0 << 18) | ((uint32_t) n1 << 12)
						| ((uint32_t) (n2 < 0 ? 0 : n2) << 6)
						| (uint32_t) (n3 < 0 ? 0 : n3);

		bin[out++] = (uint8_t) ((pack >> 16) & 255);
		if (clean[j + 2] != '=') {
			bin[out++] = (uint8_t) ((pack >> 8) & 255);
		}
		if (clean[j + 3] != '=') {
			bin[out++] = (uint8_t) (pack & 255);
		}
	}
	free(clean);

	if (out > 0 && !_StringUtf8WellFormed(bin, out)) {
		free(bin);
		return NewErrorFValue(interpreter,
							  "%s: decode() decoded bytes are not valid UTF-8",
							  ARGUMENT_ERROR);
	}

	String utf8 = Allocate(out + 1);
	if (utf8 == NULL) {
		free(bin);
		return NewErrorFValue(interpreter,
							  "%s: decode() out of memory",
							  MEMORY_ERROR);
	}
	memcpy(utf8, bin, out);
	utf8[out] = '\0';
	free(bin);

	Value* ret = NewStrValue(interpreter, utf8);
	free(utf8);
	return ret;
}

static ModuleFunction _StringModuleFunctions[] = {
	{ .Name		 = "toUpper",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringToUpper,
	  .Value	 = NULL },
	{ .Name		 = "toLower",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringToLower,
	  .Value	 = NULL },
	{ .Name		 = "capitalize",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringCapitalize,
	  .Value	 = NULL },
	{ .Name		 = "isIdentifier",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringIsIdentifier,
	  .Value	 = NULL },
	{ .Name		 = "isAlpha",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringIsAlpha,
	  .Value	 = NULL },
	{ .Name		 = "isDigit",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringIsDigit,
	  .Value	 = NULL },
	{ .Name		 = "isAlnum",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringIsAlnum,
	  .Value	 = NULL },
	{ .Name		 = "split",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _StringSplit,
	  .Value	 = NULL },
	{ .Name		 = "ord",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringOrd,
	  .Value	 = NULL },
	{ .Name		 = "chr",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringChr,
	  .Value	 = NULL },
	{ .Name		 = "bytes",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringBytes,
	  .Value	 = NULL },
	{ .Name		 = "codepoints",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringCodepoints,
	  .Value	 = NULL },
	{ .Name		 = "strip",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringStrip,
	  .Value	 = NULL },
	{ .Name		 = "join",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _StringJoin,
	  .Value	 = NULL },
	{ .Name		 = "repeat",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _StringRepeat,
	  .Value	 = NULL },
	{ .Name		 = "reverse",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringReverse,
	  .Value	 = NULL },
	{ .Name		 = "contains",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _StringContains,
	  .Value	 = NULL },
	{ .Name		 = "startsWith",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _StringStartsWith,
	  .Value	 = NULL },
	{ .Name		 = "endsWith",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _StringEndsWith,
	  .Value	 = NULL },
	{ .Name		 = "replace",
	  .Argc		 = 3,
	  .CFunction = (NativeFunctionCallback) _StringReplace,
	  .Value	 = NULL },
	{ .Name		 = "byteLength",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringByteLength,
	  .Value	 = NULL },
	{ .Name		 = "encode",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringEncode,
	  .Value	 = NULL },
	{ .Name		 = "decode",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StringDecode,
	  .Value	 = NULL },
	{ .Name = NULL }
};

Value* LoadCoreString(Interpreter* interpreter) {
	Value*	 module = NewObjectValue(interpreter);
	HashMap* map	= CoerceToHashMap(module);

	for (int i = 0; _StringModuleFunctions[i].Name != NULL; i++) {
		ModuleFunction func = _StringModuleFunctions[i];
		String		   hKey = (String) func.Name;

		if (func.Value != NULL) {
			HashMapSet(map, hKey, func.Value);
		} else {
			HashMapSet(map,
					   hKey,
					   NewNativeFunctionValue(
						   interpreter,
						   CreateNativeFunctionMeta((const String) hKey,
													func.Argc,
													func.CFunction)));
		}
	}

	return module;
}
