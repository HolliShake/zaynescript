#include "./utf8.h"

size_t Utf8Core_RuneLen(const Rune* runes) {
	size_t n = 0;
	if (runes == NULL) {
		return 0;
	}
	while (runes[n] != 0) {
		++n;
	}
	return n;
}

bool Utf8Core_RuneValid(Rune r) {
	return utf8proc_codepoint_valid((utf8proc_int32_t) r) != 0;
}

int Utf8Core_RuneCharWidth(Rune r) {
	return utf8proc_charwidth((utf8proc_int32_t) r);
}

int Utf8Core_RuneCategory(Rune r) {
	return (int) utf8proc_category((utf8proc_int32_t) r);
}

Rune Utf8Core_RuneToLower(Rune r) {
	return (Rune) utf8proc_tolower((utf8proc_int32_t) r);
}

Rune Utf8Core_RuneToUpper(Rune r) {
	return (Rune) utf8proc_toupper((utf8proc_int32_t) r);
}

Rune Utf8Core_RuneToTitle(Rune r) {
	return (Rune) utf8proc_totitle((utf8proc_int32_t) r);
}

bool Utf8Core_GraphemeBreak(Rune prev, Rune curr) {
	return utf8proc_grapheme_break((utf8proc_int32_t) prev,
								   (utf8proc_int32_t) curr)
		   != 0;
}

String Utf8Core_RunesToUtf8(const Rune* runes) {
	if (runes == NULL) {
		return NULL;
	}

	size_t n   = Utf8Core_RuneLen(runes);
	size_t cap = 0;
	for (size_t i = 0; i < n; i++) {
		if (!Utf8Core_RuneValid(runes[i])) {
			return NULL;
		}
		utf8proc_uint8_t tmp[4];
		utf8proc_ssize_t w =
			utf8proc_encode_char((utf8proc_int32_t) runes[i], tmp);
		if (w <= 0) {
			return NULL;
		}
		cap += (size_t) w;
	}

	String out = Allocate(cap + 1);
	size_t off = 0;
	for (size_t i = 0; i < n; i++) {
		utf8proc_ssize_t w =
			utf8proc_encode_char((utf8proc_int32_t) runes[i],
								 (utf8proc_uint8_t*) (out + off));
		if (w <= 0) {
			free(out);
			return NULL;
		}
		off += (size_t) w;
	}
	out[off] = '\0';
	return out;
}

Rune* Utf8Core_Utf8ToRunes(const char* utf8) {
	if (utf8 == NULL) {
		return NULL;
	}

	const utf8proc_uint8_t* p	  = (const utf8proc_uint8_t*) utf8;
	utf8proc_ssize_t		blen  = (utf8proc_ssize_t) strlen(utf8);
	size_t					count = 0;
	utf8proc_ssize_t		pos	  = 0;

	while (pos < blen) {
		utf8proc_int32_t cp;
		utf8proc_ssize_t d = utf8proc_iterate(p + pos, blen - pos, &cp);
		if (d <= 0) {
			return NULL;
		}
		pos += d;
		++count;
	}

	Rune* runes = Allocate(sizeof(Rune) * (count + 1));
	pos			= 0;
	size_t o	= 0;
	while (pos < blen) {
		utf8proc_int32_t cp;
		utf8proc_ssize_t d = utf8proc_iterate(p + pos, blen - pos, &cp);
		if (d <= 0) {
			free(runes);
			return NULL;
		}
		pos		   += d;
		runes[o++]	= (Rune) cp;
	}
	runes[o] = 0;
	return runes;
}

static Rune*
_Utf8Core_normalize_runes(const Rune* runes,
						  utf8proc_uint8_t* (*norm)(const utf8proc_uint8_t*) ) {
	if (runes == NULL || norm == NULL) {
		return NULL;
	}

	String u8 = Utf8Core_RunesToUtf8(runes);
	if (u8 == NULL) {
		return NULL;
	}

	utf8proc_uint8_t* out = norm((const utf8proc_uint8_t*) u8);
	free(u8);
	if (out == NULL) {
		return NULL;
	}

	Rune* r = Utf8Core_Utf8ToRunes((const char*) out);
	free(out);
	return r;
}

Rune* Utf8Core_NFD_Runes(const Rune* runes) {
	return _Utf8Core_normalize_runes(runes, utf8proc_NFD);
}

Rune* Utf8Core_NFC_Runes(const Rune* runes) {
	return _Utf8Core_normalize_runes(runes, utf8proc_NFC);
}

Rune* Utf8Core_NFKD_Runes(const Rune* runes) {
	return _Utf8Core_normalize_runes(runes, utf8proc_NFKD);
}

Rune* Utf8Core_NFKC_Runes(const Rune* runes) {
	return _Utf8Core_normalize_runes(runes, utf8proc_NFKC);
}

Rune* Utf8Core_CasefoldRunes(const Rune* runes) {
	if (runes == NULL) {
		return NULL;
	}

	String u8 = Utf8Core_RunesToUtf8(runes);
	if (u8 == NULL) {
		return NULL;
	}

	utf8proc_uint8_t* mapped = NULL;
	utf8proc_ssize_t  len	 = utf8proc_map(
		(const utf8proc_uint8_t*) u8,
		(utf8proc_ssize_t) strlen(u8),
		&mapped,
		(utf8proc_option_t) (UTF8PROC_NULLTERM | UTF8PROC_CASEFOLD));
	free(u8);

	if (len < 0 || mapped == NULL) {
		if (mapped != NULL) {
			free(mapped);
		}
		return NULL;
	}

	Rune* r = Utf8Core_Utf8ToRunes((const char*) mapped);
	free(mapped);
	return r;
}

static Rune* _Utf8RunesFromValue(Value* v) {
	if (!ValueIsStr(v)) {
		return NULL;
	}
	return (Rune*) v->Value.Opaque;
}

static Value*
_Utf8StrFromRunes(Interpreter* interpreter, Rune* runes, const char* what) {
	if (runes == NULL) {
		return NewErrorFValue(interpreter,
							  "utf8: %s failed (invalid text or out of memory)",
							  what);
	}
	String utf8 = RunesStrToString(runes);
	free(runes);
	if (utf8 == NULL) {
		return NewErrorValue(interpreter, "utf8: UTF-8 encode failed");
	}
	Value* out = NewStrValue(interpreter, utf8);
	free(utf8);
	return out;
}

static Value* _Utf8Nfc(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorValue(interpreter, "nfc() expects 1 string");
	}
	return _Utf8StrFromRunes(
		interpreter,
		Utf8Core_NFC_Runes(_Utf8RunesFromValue(arguments[0])),
		"nfc");
}

static Value* _Utf8Nfd(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorValue(interpreter, "nfd() expects 1 string");
	}
	return _Utf8StrFromRunes(
		interpreter,
		Utf8Core_NFD_Runes(_Utf8RunesFromValue(arguments[0])),
		"nfd");
}

static Value* _Utf8Nfkc(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorValue(interpreter, "nfkc() expects 1 string");
	}
	return _Utf8StrFromRunes(
		interpreter,
		Utf8Core_NFKC_Runes(_Utf8RunesFromValue(arguments[0])),
		"nfkc");
}

static Value* _Utf8Nfkd(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorValue(interpreter, "nfkd() expects 1 string");
	}
	return _Utf8StrFromRunes(
		interpreter,
		Utf8Core_NFKD_Runes(_Utf8RunesFromValue(arguments[0])),
		"nfkd");
}

static Value*
_Utf8Casefold(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorValue(interpreter, "casefold() expects 1 string");
	}
	return _Utf8StrFromRunes(
		interpreter,
		Utf8Core_CasefoldRunes(_Utf8RunesFromValue(arguments[0])),
		"casefold");
}

static Value*
_Utf8ToLower(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorValue(interpreter, "toLower() expects 1 string");
	}
	Rune*  runes = _Utf8RunesFromValue(arguments[0]);
	size_t n	 = Utf8Core_RuneLen(runes);
	Rune*  out	 = Allocate(sizeof(Rune) * (n + 1));
	for (size_t i = 0; i < n; i++) {
		out[i] = Utf8Core_RuneToLower(runes[i]);
	}
	out[n] = 0;
	return _Utf8StrFromRunes(interpreter, out, "toLower");
}

static Value*
_Utf8ToUpper(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorValue(interpreter, "toUpper() expects 1 string");
	}
	Rune*  runes = _Utf8RunesFromValue(arguments[0]);
	size_t n	 = Utf8Core_RuneLen(runes);
	Rune*  out	 = Allocate(sizeof(Rune) * (n + 1));
	for (size_t i = 0; i < n; i++) {
		out[i] = Utf8Core_RuneToUpper(runes[i]);
	}
	out[n] = 0;
	return _Utf8StrFromRunes(interpreter, out, "toUpper");
}

static Value*
_Utf8ToTitle(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorValue(interpreter, "toTitle() expects 1 string");
	}
	Rune*  runes = _Utf8RunesFromValue(arguments[0]);
	size_t n	 = Utf8Core_RuneLen(runes);
	Rune*  out	 = Allocate(sizeof(Rune) * (n + 1));
	for (size_t i = 0; i < n; i++) {
		out[i] = Utf8Core_RuneToTitle(runes[i]);
	}
	out[n] = 0;
	return _Utf8StrFromRunes(interpreter, out, "toTitle");
}

static Value* _Utf8Len(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorValue(interpreter, "len() expects 1 string");
	}
	return NewIntValue(
		interpreter,
		(int) Utf8Core_RuneLen(_Utf8RunesFromValue(arguments[0])));
}

static Value*
_Utf8CharWidth(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorValue(interpreter, "charWidth() expects 1 number");
	}
	if (!ValueIsNum(arguments[0]) && !ValueIsInt(arguments[0])) {
		return NewErrorValue(interpreter,
							 "charWidth() expects a number (codepoint)");
	}
	int cp = CoerceToI32(arguments[0]);
	return NewIntValue(interpreter, Utf8Core_RuneCharWidth((Rune) cp));
}

static Value*
_Utf8CategoryString(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorValue(interpreter, "categoryString() expects 1 number");
	}
	if (!ValueIsNum(arguments[0]) && !ValueIsInt(arguments[0])) {
		return NewErrorValue(interpreter,
							 "categoryString() expects a number (codepoint)");
	}
	int			cp	= CoerceToI32(arguments[0]);
	const char* cat = utf8proc_category_string((utf8proc_int32_t) cp);
	String		s	= AllocateString((String) cat);
	Value*		out = NewStrValue(interpreter, s);
	free(s);
	return out;
}

static Value*
_Utf8ValidCodepoint(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorValue(interpreter, "validCodepoint() expects 1 number");
	}
	if (!ValueIsNum(arguments[0]) && !ValueIsInt(arguments[0])) {
		return NewErrorValue(interpreter,
							 "validCodepoint() expects a number (codepoint)");
	}
	int cp = CoerceToI32(arguments[0]);
	return NewBoolValue(interpreter, Utf8Core_RuneValid((Rune) cp) ? 1 : 0);
}

static Value*
_Utf8GraphemeBreak(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorValue(interpreter,
							 "graphemeBreak() expects 2 numbers (codepoints)");
	}
	if ((!ValueIsNum(arguments[0]) && !ValueIsInt(arguments[0]))
		|| (!ValueIsNum(arguments[1]) && !ValueIsInt(arguments[1]))) {
		return NewErrorValue(interpreter,
							 "graphemeBreak() expects numbers (codepoints)");
	}
	Rune a = (Rune) CoerceToI32(arguments[0]);
	Rune b = (Rune) CoerceToI32(arguments[1]);
	return NewBoolValue(interpreter, Utf8Core_GraphemeBreak(a, b) ? 1 : 0);
}

static Value*
_Utf8Version(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 0) {
		return NewErrorValue(interpreter, "version() expects 0 arguments");
	}
	String ver = AllocateString((String) utf8proc_version());
	Value* out = NewStrValue(interpreter, ver);
	free(ver);
	return out;
}

static ModuleFunction _Utf8ModuleFunctions[] = {
	{ .Name		 = "nfc",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8Nfc,
	  .Value	 = NULL },
	{ .Name		 = "nfd",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8Nfd,
	  .Value	 = NULL },
	{ .Name		 = "nfkc",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8Nfkc,
	  .Value	 = NULL },
	{ .Name		 = "nfkd",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8Nfkd,
	  .Value	 = NULL },
	{ .Name		 = "casefold",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8Casefold,
	  .Value	 = NULL },
	{ .Name		 = "toLower",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8ToLower,
	  .Value	 = NULL },
	{ .Name		 = "toUpper",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8ToUpper,
	  .Value	 = NULL },
	{ .Name		 = "toTitle",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8ToTitle,
	  .Value	 = NULL },
	{ .Name		 = "len",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8Len,
	  .Value	 = NULL },
	{ .Name		 = "charWidth",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8CharWidth,
	  .Value	 = NULL },
	{ .Name		 = "categoryString",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8CategoryString,
	  .Value	 = NULL },
	{ .Name		 = "validCodepoint",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _Utf8ValidCodepoint,
	  .Value	 = NULL },
	{ .Name		 = "graphemeBreak",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _Utf8GraphemeBreak,
	  .Value	 = NULL },
	{ .Name		 = "version",
	  .Argc		 = 0,
	  .CFunction = (NativeFunctionCallback) _Utf8Version,
	  .Value	 = NULL },
	{ .Name = NULL }
};

Value* LoadCoreUtf8(Interpreter* interpreter) {
	Value*	 mod	= NewObjectValue(interpreter);
	HashMap* modMap = CoerceToHashMap(mod);

	for (int i = 0; _Utf8ModuleFunctions[i].Name != NULL; i++) {
		ModuleFunction func = _Utf8ModuleFunctions[i];
		String		   hKey = (String) func.Name;

		if (func.Value != NULL) {
			HashMapSet(modMap, hKey, func.Value);
		} else {
			HashMapSet(modMap,
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
