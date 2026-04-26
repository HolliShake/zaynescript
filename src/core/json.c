#include "./json.h"

typedef struct {
	char*  Data;
	size_t Len;
	size_t Cap;
} JsonStringBuilder;

typedef struct {
	const char* Cur;
} JsonParser;

typedef struct {
	void** Items;
	size_t Count;
	size_t Cap;
} JsonSeenSet;

/* ── string builder ─────────────────────────────────────────────────────── */

static void _JsonSbEnsure(JsonStringBuilder* sb, size_t extra) {
	size_t need = sb->Len + extra + 1;
	if (need <= sb->Cap)
		return;
	/* one multiply instead of a loop */
	size_t cap = sb->Cap ? sb->Cap : 32;
	while (cap < need)
		cap <<= 1;
	sb->Cap	 = cap;
	sb->Data = Reallocate(sb->Data, cap);
}

static inline void _JsonSbAppendChar(JsonStringBuilder* sb, char c) {
	_JsonSbEnsure(sb, 1);
	sb->Data[sb->Len++] = c;
	sb->Data[sb->Len]	= '\0';
}

static inline void
_JsonSbAppendRaw(JsonStringBuilder* sb, const char* s, size_t n) {
	_JsonSbEnsure(sb, n);
	memcpy(sb->Data + sb->Len, s, n);
	sb->Len			  += n;
	sb->Data[sb->Len]  = '\0';
}

static inline void _JsonSbAppendCStr(JsonStringBuilder* sb, const char* s) {
	_JsonSbAppendRaw(sb, s, strlen(s));
}

static void _JsonSbAppendHex4(JsonStringBuilder* sb, unsigned int cp) {
	static const char hex[]	 = "0123456789abcdef";
	char			  buf[6] = { '\\',
								 'u',
								 hex[(cp >> 12) & 0xF],
								 hex[(cp >> 8) & 0xF],
								 hex[(cp >> 4) & 0xF],
								 hex[cp & 0xF] };
	_JsonSbAppendRaw(sb, buf, 6);
}

/* ── cycle detection ────────────────────────────────────────────────────── */

static bool _JsonSeenHas(JsonSeenSet* seen, void* p) {
	for (size_t i = 0; i < seen->Count; i++)
		if (seen->Items[i] == p)
			return true;
	return false;
}

static void _JsonSeenPush(JsonSeenSet* seen, void* p) {
	if (seen->Count == seen->Cap) {
		seen->Cap	= seen->Cap ? seen->Cap * 2 : 16;
		seen->Items = Reallocate(seen->Items, seen->Cap * sizeof(void*));
	}
	seen->Items[seen->Count++] = p;
}

static inline void _JsonSeenPop(JsonSeenSet* seen) {
	if (seen->Count > 0)
		seen->Count--;
}

/* ── parser helpers ─────────────────────────────────────────────────────── */

static inline void _JsonSkipWs(JsonParser* p) {
	while (isspace((unsigned char) *p->Cur))
		p->Cur++;
}

static inline bool _JsonMatch(JsonParser* p, const char* kw) {
	size_t n = strlen(kw);
	if (strncmp(p->Cur, kw, n) != 0)
		return false;
	p->Cur += n;
	return true;
}

static inline int _JsonHexDigit(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');
	if (c >= 'A' && c <= 'F')
		return 10 + (c - 'A');
	return -1;
}

static bool _JsonParseHex4(const char* s, unsigned int* out) {
	int d0 = _JsonHexDigit(s[0]), d1 = _JsonHexDigit(s[1]),
		d2 = _JsonHexDigit(s[2]), d3 = _JsonHexDigit(s[3]);
	if ((d0 | d1 | d2 | d3) < 0)
		return false;
	*out = ((unsigned) d0 << 12) | ((unsigned) d1 << 8) | ((unsigned) d2 << 4)
		   | (unsigned) d3;
	return true;
}

static bool _JsonAppendCodepointUtf8(JsonStringBuilder* out, unsigned int cp) {
	if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
		return false;
	unsigned char utf8[5] = { 0 };
	int			  n		  = utf_encode_char((int) cp, utf8);
	if (n <= 0 || n > 4)
		return false;
	_JsonSbAppendRaw(out, (const char*) utf8, (size_t) n);
	return true;
}

/* ── string parsing ─────────────────────────────────────────────────────── */

static bool _JsonParseStringRaw(JsonParser* p, JsonStringBuilder* out) {
	if (*p->Cur != '"')
		return false;
	p->Cur++;

	while (*p->Cur != '\0') {
		/* fast path: bulk-copy a run of plain ASCII bytes */
		const char* run = p->Cur;
		while ((unsigned char) *p->Cur >= 0x20 && *p->Cur != '"'
			   && *p->Cur != '\\') {
			p->Cur++;
		}
		if (p->Cur > run)
			_JsonSbAppendRaw(out, run, (size_t) (p->Cur - run));

		if (*p->Cur == '"') {
			p->Cur++;
			return true;
		}
		if (*p->Cur == '\0')
			break;

		/* escape sequence */
		p->Cur++; /* skip '\' */
		char esc = *p->Cur++;
		switch (esc) {
			case '"':
			case '\\':
			case '/':
				_JsonSbAppendChar(out, esc);
				break;
			case 'b':
				_JsonSbAppendChar(out, '\b');
				break;
			case 'f':
				_JsonSbAppendChar(out, '\f');
				break;
			case 'n':
				_JsonSbAppendChar(out, '\n');
				break;
			case 'r':
				_JsonSbAppendChar(out, '\r');
				break;
			case 't':
				_JsonSbAppendChar(out, '\t');
				break;
			case 'u':
				{
					unsigned int cp = 0;
					if (!_JsonParseHex4(p->Cur, &cp))
						return false;
					p->Cur += 4;
					if (cp >= 0xD800 && cp <= 0xDBFF) {
						if (p->Cur[0] != '\\' || p->Cur[1] != 'u')
							return false;
						p->Cur			 += 2;
						unsigned int low  = 0;
						if (!_JsonParseHex4(p->Cur, &low))
							return false;
						p->Cur += 4;
						if (low < 0xDC00 || low > 0xDFFF)
							return false;
						cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
					} else if (cp >= 0xDC00 && cp <= 0xDFFF) {
						return false;
					}
					if (!_JsonAppendCodepointUtf8(out, cp))
						return false;
					break;
				}
			default:
				return false;
		}
	}
	return false;
}

/* ── value parsers ──────────────────────────────────────────────────────── */

static Value* _JsonParseValue(Interpreter* interp, JsonParser* p, bool* ok);

static Value* _JsonParseArray(Interpreter* interp, JsonParser* p, bool* ok) {
	p->Cur++; /* skip '[' */
	_JsonSkipWs(p);
	Value* arrVal = NewArrayValue(interp);
	Array* arr	  = CoerceToArray(arrVal);
	if (*p->Cur == ']') {
		p->Cur++;
		return arrVal;
	}
	while (true) {
		Value* v = _JsonParseValue(interp, p, ok);
		if (!*ok)
			return NULL;
		ArrayPush(arr, v);
		_JsonSkipWs(p);
		if (*p->Cur == ',') {
			p->Cur++;
			_JsonSkipWs(p);
			continue;
		}
		if (*p->Cur == ']') {
			p->Cur++;
			return arrVal;
		}
		*ok = false;
		return NULL;
	}
}

static Value* _JsonParseObject(Interpreter* interp, JsonParser* p, bool* ok) {
	p->Cur++; /* skip '{' */
	_JsonSkipWs(p);
	Value*	 objVal = NewObjectValue(interp);
	HashMap* map	= CoerceToHashMap(objVal);
	if (*p->Cur == '}') {
		p->Cur++;
		return objVal;
	}

	/* reuse a single key builder for the lifetime of this object parse */
	JsonStringBuilder key = { .Data = Allocate(64), .Len = 0, .Cap = 64 };
	key.Data[0]			  = '\0';

	while (true) {
		/* reset key without freeing */
		key.Len		= 0;
		key.Data[0] = '\0';

		if (!_JsonParseStringRaw(p, &key))
			goto fail;
		_JsonSkipWs(p);
		if (*p->Cur != ':')
			goto fail;
		p->Cur++;
		_JsonSkipWs(p);

		Value* v = _JsonParseValue(interp, p, ok);
		if (!*ok)
			goto fail;

		HashMapSet(map, key.Data, v);
		_JsonSkipWs(p);
		if (*p->Cur == ',') {
			p->Cur++;
			_JsonSkipWs(p);
			continue;
		}
		if (*p->Cur == '}') {
			p->Cur++;
			free(key.Data);
			return objVal;
		}
	fail:
		free(key.Data);
		*ok = false;
		return NULL;
	}
}

/*
 * OPTIMIZATION: no temporary allocation.
 * strtod/strtoll parse directly from the source buffer and advance their own
 * endptr — we just verify it lands where our scanner left off.
 */
static Value* _JsonParseNumber(Interpreter* interp, JsonParser* p, bool* ok) {
	const char* start = p->Cur;
	if (*p->Cur == '-')
		p->Cur++;

	if (*p->Cur == '0') {
		p->Cur++;
	} else if (isdigit((unsigned char) *p->Cur)) {
		while (isdigit((unsigned char) *p->Cur))
			p->Cur++;
	} else {
		*ok = false;
		return NULL;
	}

	bool isFloat = false;
	if (*p->Cur == '.') {
		isFloat = true;
		p->Cur++;
		if (!isdigit((unsigned char) *p->Cur)) {
			*ok = false;
			return NULL;
		}
		while (isdigit((unsigned char) *p->Cur))
			p->Cur++;
	}
	if (*p->Cur == 'e' || *p->Cur == 'E') {
		isFloat = true;
		p->Cur++;
		if (*p->Cur == '+' || *p->Cur == '-')
			p->Cur++;
		if (!isdigit((unsigned char) *p->Cur)) {
			*ok = false;
			return NULL;
		}
		while (isdigit((unsigned char) *p->Cur))
			p->Cur++;
	}

	const char* scanned_end = p->Cur;

	/* parse integer path first to preserve int semantics */
	if (!isFloat) {
		char*	  endptr = NULL;
		long long ll	 = strtoll(start, &endptr, 10);
		if (endptr == scanned_end && ll >= INT_MIN && ll <= INT_MAX)
			return NewIntValue(interp, (int) ll);
	}

	char*  endptr = NULL;
	double d	  = strtod(start, &endptr);
	if (endptr != scanned_end || !isfinite(d)) {
		*ok = false;
		return NULL;
	}
	return NewNumValue(interp, d);
}

static Value* _JsonParseValue(Interpreter* interp, JsonParser* p, bool* ok) {
	_JsonSkipWs(p);
	switch (*p->Cur) {
		case '"':
			{
				JsonStringBuilder sb = { .Data = Allocate(32),
										 .Len  = 0,
										 .Cap  = 32 };
				sb.Data[0]			 = '\0';
				if (!_JsonParseStringRaw(p, &sb)) {
					free(sb.Data);
					*ok = false;
					return NULL;
				}
				Value* out = NewStrValue(interp, sb.Data);
				free(sb.Data);
				return out;
			}
		case '[':
			return _JsonParseArray(interp, p, ok);
		case '{':
			return _JsonParseObject(interp, p, ok);
		case 't':
			if (_JsonMatch(p, "true"))
				return interp->True;
			break;
		case 'f':
			if (_JsonMatch(p, "false"))
				return interp->False;
			break;
		case 'n':
			if (_JsonMatch(p, "null"))
				return interp->Null;
			break;
		default:
			if (*p->Cur == '-' || isdigit((unsigned char) *p->Cur))
				return _JsonParseNumber(interp, p, ok);
			break;
	}
	*ok = false;
	return NULL;
}

/* ── stringify ──────────────────────────────────────────────────────────── */

/*
 * OPTIMIZATION: bulk-copy safe byte runs, pay escape cost only for the
 * characters that actually need it.
 */
static void _JsonStringifyEscapedString(JsonStringBuilder* out, String s) {
	_JsonSbAppendChar(out, '"');
	const unsigned char* p = (const unsigned char*) s;
	while (*p) {
		/* collect a run of bytes that need no escaping */
		const unsigned char* run = p;
		while (*p >= 0x20 && *p != '"' && *p != '\\' && *p != 0x7F)
			p++;
		if (p > run)
			_JsonSbAppendRaw(out, (const char*) run, (size_t) (p - run));
		if (!*p)
			break;

		/* handle the byte that stopped the run */
		size_t n = utf_char_length((char*) p);
		if (n == 0)
			break;
		int cp = utf_to_codepoint(p[0],
								  n > 1 ? p[1] : 0,
								  n > 2 ? p[2] : 0,
								  n > 3 ? p[3] : 0);
		switch (cp) {
			case '"':
				_JsonSbAppendCStr(out, "\\\"");
				break;
			case '\\':
				_JsonSbAppendCStr(out, "\\\\");
				break;
			case '\b':
				_JsonSbAppendCStr(out, "\\b");
				break;
			case '\f':
				_JsonSbAppendCStr(out, "\\f");
				break;
			case '\n':
				_JsonSbAppendCStr(out, "\\n");
				break;
			case '\r':
				_JsonSbAppendCStr(out, "\\r");
				break;
			case '\t':
				_JsonSbAppendCStr(out, "\\t");
				break;
			default:
				if (cp >= 0 && cp < 0x20)
					_JsonSbAppendHex4(out, (unsigned int) cp);
				else
					_JsonSbAppendRaw(out, (const char*) p, n);
				break;
		}
		p += n;
	}
	_JsonSbAppendChar(out, '"');
}

static bool
_JsonStringifyValue(JsonStringBuilder* out, Value* v, JsonSeenSet* seen) {
	if (ValueIsNull(v)) {
		_JsonSbAppendCStr(out, "null");
		return true;
	}
	if (ValueIsBool(v)) {
		_JsonSbAppendCStr(out, v->Value.I32 ? "true" : "false");
		return true;
	}
	if (ValueIsInt(v) || ValueIsNum(v) || ValueIsBigInt(v)
		|| ValueIsBigNum(v)) {
		String n = ValueToString(v);
		_JsonSbAppendCStr(out, n);
		free(n);
		return true;
	}
	if (ValueIsStr(v)) {
		String s = ValueToString(v);
		_JsonStringifyEscapedString(out, s);
		free(s);
		return true;
	}
	if (ValueIsArray(v)) {
		Array* arr = CoerceToArray(v);
		if (_JsonSeenHas(seen, arr))
			return false;
		_JsonSeenPush(seen, arr);
		_JsonSbAppendChar(out, '[');
		size_t n = ArrayLength(arr);
		for (size_t i = 0; i < n; i++) {
			if (i > 0)
				_JsonSbAppendChar(out, ',');
			if (!_JsonStringifyValue(out, (Value*) ArrayGet(arr, i), seen)) {
				_JsonSeenPop(seen);
				return false;
			}
		}
		_JsonSbAppendChar(out, ']');
		_JsonSeenPop(seen);
		return true;
	}
	if (ValueIsObject(v)) {
		HashMap* map = CoerceToHashMap(v);
		if (_JsonSeenHas(seen, map))
			return false;
		_JsonSeenPush(seen, map);
		_JsonSbAppendChar(out, '{');
		bool first = true;
		for (size_t i = 0; i < map->Size; i++) {
			for (HashNode* node = &map->Buckets[i]; node && node->Key;
				 node			= node->Next) {
				if (!first)
					_JsonSbAppendChar(out, ',');
				first = false;
				_JsonStringifyEscapedString(out, node->Key);
				_JsonSbAppendChar(out, ':');
				if (!_JsonStringifyValue(out, (Value*) node->Val, seen)) {
					_JsonSeenPop(seen);
					return false;
				}
			}
		}
		_JsonSbAppendChar(out, '}');
		_JsonSeenPop(seen);
		return true;
	}
	return false;
}

/* ── public API ─────────────────────────────────────────────────────────── */

static Value* _JsonParse(Interpreter* interp, int argc, Value** arguments) {
	if (argc != 1)
		return NewErrorFValue(interp,
							  "%s: json.parse() expects exactly 1 argument",
							  ARGUMENT_ERROR);
	if (!ValueIsStr(arguments[0]))
		return NewErrorFValue(interp,
							  "%s: json.parse() expects a string argument",
							  TYPE_ERROR);

	String	   source = ValueToString(arguments[0]);
	JsonParser p	  = { .Cur = source };
	bool	   ok	  = true;
	Value*	   out	  = _JsonParseValue(interp, &p, &ok);
	_JsonSkipWs(&p);
	bool atEnd = (*p.Cur == '\0');
	free(source);

	if (!ok || !atEnd)
		return NewErrorFValue(interp, "%s: invalid JSON input", RUNTIME_ERROR);
	return out;
}

static Value* _JsonStringify(Interpreter* interp, int argc, Value** arguments) {
	if (argc != 1)
		return NewErrorFValue(interp,
							  "%s: json.stringify() expects exactly 1 argument",
							  ARGUMENT_ERROR);

	JsonStringBuilder out = { .Data = Allocate(128), .Len = 0, .Cap = 128 };
	out.Data[0]			  = '\0';
	JsonSeenSet seen	  = { .Items = NULL, .Count = 0, .Cap = 0 };
	bool		ok		  = _JsonStringifyValue(&out, arguments[0], &seen);
	free(seen.Items);
	if (!ok) {
		free(out.Data);
		return NewErrorFValue(
			interp,
			"%s: cannot stringify value (unsupported type or cyclic reference)",
			TYPE_ERROR);
	}
	Value* outVal = NewStrValue(interp, out.Data);
	free(out.Data);
	return outVal;
}

static ModuleFunction _JsonModuleFunctions[] = {
	{ .Name		 = "parse",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _JsonParse,
	  .Value	 = NULL },
	{ .Name		 = "stringify",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _JsonStringify,
	  .Value	 = NULL },
	{ .Name = NULL }
};

Value* LoadCoreJson(Interpreter* interp) {
	Value*	 module = NewObjectValue(interp);
	HashMap* map	= CoerceToHashMap(module);
	for (int i = 0; _JsonModuleFunctions[i].Name != NULL; i++) {
		ModuleFunction func = _JsonModuleFunctions[i];
		HashMapSet(map,
				   func.Name,
				   func.Value
					   ? func.Value
					   : NewNativeFunctionValue(
							 interp,
							 CreateNativeFunctionMeta((const String) func.Name,
													  func.Argc,
													  func.CFunction)));
	}
	return module;
}