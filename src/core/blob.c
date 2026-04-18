#include "./blob.h"

/**
 * @brief Growable byte buffer used while constructing Blob values.
 */
typedef struct {
	uint8_t* data; /**< Allocated storage; may be NULL when empty. */
	size_t	 len;  /**< Number of bytes currently stored. */
	size_t	 cap;  /**< Allocated capacity in bytes. */
} _BlobBuf;

static Value* _BlobBufAppend(_BlobBuf*		buf,
							 Interpreter*	interpreter,
							 const uint8_t* bytes,
							 size_t			n) {
	if (n == 0)
		return NULL;
	size_t need = buf->len + n;
	if (need > buf->cap) {
		size_t nc = buf->cap ? buf->cap : 64;
		while (nc < need)
			nc *= 2;
		uint8_t* nd = Reallocate(buf->data, nc);
		if (nd == NULL)
			return NewErrorFValue(interpreter, "Blob: out of memory");
		buf->data = nd;
		buf->cap  = nc;
	}
	memcpy(buf->data + buf->len, bytes, n);
	buf->len = need;
	return NULL;
}

static Value*
_BlobBufAppendUtf8(_BlobBuf* buf, Interpreter* interpreter, const String utf8) {
	if (utf8 == NULL)
		return NULL;
	return _BlobBufAppend(buf,
						  interpreter,
						  (const uint8_t*) utf8,
						  strlen(utf8));
}

/**
 * @brief Formats one element for Blob string join (commas; arrays recurse).
 */
static String _BlobJoinElementString(Value* el, Interpreter* interpreter);

static String _BlobArrayJoinString(Array* arr, Interpreter* interpreter) {
	size_t n = ArrayLength(arr);
	if (n == 0)
		return AllocateString("");

	String* parts = Allocate(sizeof(String) * n);
	if (parts == NULL)
		return NULL;
	size_t total = n > 0 ? n - 1 : 0;
	for (size_t i = 0; i < n; i++) {
		parts[i] =
			_BlobJoinElementString((Value*) ArrayGet(arr, i), interpreter);
		if (parts[i] == NULL) {
			for (size_t j = 0; j < i; j++)
				free(parts[j]);
			free(parts);
			return NULL;
		}
		total += strlen(parts[i]);
	}

	String out = Allocate(total + 1);
	if (out == NULL) {
		for (size_t i = 0; i < n; i++)
			free(parts[i]);
		free(parts);
		return NULL;
	}

	size_t pos = 0;
	for (size_t i = 0; i < n; i++) {
		if (i > 0)
			out[pos++] = ',';
		size_t L = strlen(parts[i]);
		memcpy(out + pos, parts[i], L);
		pos += L;
		free(parts[i]);
	}
	free(parts);
	out[pos] = '\0';
	return out;
}

static String _BlobJoinElementString(Value* el, Interpreter* interpreter) {
	if (el == NULL || ValueIsNull(el))
		return AllocateString("");
	if (ValueIsArray(el))
		return _BlobArrayJoinString(CoerceToArray(el), interpreter);
	if (ValueIsStr(el))
		return RunesStrToString((Rune*) el->Value.Opaque);
	return ValueToString(el);
}

static Value*
_BlobAppendPart(_BlobBuf* buf, Interpreter* interpreter, Value* part) {
	if (part == NULL || ValueIsNull(part))
		return NULL;

	if (ValueIsBlob(part)) {
		Blob* b = CoerceToBlob(part);
		if (b->Size > 0 && b->Data != NULL)
			return _BlobBufAppend(buf, interpreter, b->Data, b->Size);
		return NULL;
	}

	if (ValueIsArray(part)) {
		String s = _BlobArrayJoinString(CoerceToArray(part), interpreter);
		if (s == NULL)
			return NewErrorFValue(interpreter, "Blob: out of memory");
		Value* err = _BlobBufAppendUtf8(buf, interpreter, s);
		free(s);
		return err;
	}

	if (ValueIsStr(part)) {
		String s = RunesStrToString((Rune*) part->Value.Opaque);
		if (s == NULL)
			return NewErrorFValue(interpreter, "Blob: out of memory");
		Value* err = _BlobBufAppendUtf8(buf, interpreter, s);
		free(s);
		return err;
	}

	String s = ValueToString(part);
	if (s == NULL)
		return NewErrorFValue(interpreter, "Blob: out of memory");
	Value* err = _BlobBufAppendUtf8(buf, interpreter, s);
	free(s);
	return err;
}

static String _BlobReadTypeOption(HashMap* options) {
	if (options == NULL)
		return AllocateString("");
	Value* t = (Value*) HashMapGet(options, "type");
	if (t == NULL || ValueIsNull(t))
		return AllocateString("");
	if (ValueIsStr(t))
		return RunesStrToString((Rune*) t->Value.Opaque);
	return ValueToString(t);
}

static Value* _BlobInit(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc < 1) {
		return NewErrorFValue(interpreter,
							  "Blob constructor requires at least 1 argument");
	}

	Value* thisArg = arguments[0];
	if (!ValueIsBlob(thisArg)) {
		return NewErrorFValue(
			interpreter,
			"Blob constructor requires a Blob argument as the first argument");
	}

	Value* data = argc >= 2 ? arguments[1] : NULL;
	if (data != NULL && !ValueIsNull(data) && !ValueIsStr(data)
		&& !ValueIsArray(data) && !ValueIsBlob(data)) {
		return NewErrorFValue(interpreter,
							  "Blob constructor: blobParts must be a string, "
							  "Array, Blob, or null");
	}

	Value* optionsObj = argc >= 3 ? arguments[2] : NULL;
	if (optionsObj != NULL && !ValueIsNull(optionsObj)
		&& !ValueIsObject(optionsObj)) {
		return NewErrorFValue(
			interpreter,
			"Blob constructor: options must be an object or null");
	}

	HashMap* options = (optionsObj != NULL && !ValueIsNull(optionsObj))
						   ? CoerceToHashMap(optionsObj)
						   : NULL;

	Blob* blob = CoerceToBlob(thisArg);

	_BlobBuf buf = { NULL, 0, 0 };

	if (data != NULL && !ValueIsNull(data)) {
		if (ValueIsArray(data)) {
			Array* a = CoerceToArray(data);
			for (size_t i = 0; i < ArrayLength(a); i++) {
				Value* err =
					_BlobAppendPart(&buf, interpreter, (Value*) ArrayGet(a, i));
				if (err != NULL) {
					free(buf.data);
					return err;
				}
			}
		} else {
			Value* err = _BlobAppendPart(&buf, interpreter, data);
			if (err != NULL) {
				free(buf.data);
				return err;
			}
		}
	}

	String mime = _BlobReadTypeOption(options);
	if (mime == NULL) {
		free(buf.data);
		return NewErrorFValue(interpreter, "Blob: out of memory");
	}

	if (blob->Data != NULL) {
		free(blob->Data);
		blob->Data = NULL;
	}
	if (blob->MimeType != NULL) {
		free(blob->MimeType);
		blob->MimeType = NULL;
	}

	blob->Data	   = buf.data;
	blob->Size	   = buf.len;
	blob->MimeType = AllocateString(mime);
	free(mime);

	return interpreter->Null;
}

static ModuleFunction _BlobClassMethods[] = {
	{ .Name		 = CONSTRUCTOR_NAME,
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _BlobInit,
	  .Value	 = NULL },
	{ .Name = NULL }
};

Value* CreateBlobClass(Interpreter* interpreter) {
	Value* blobClass =
		NewClassValue(interpreter,
					  CreateUserClass("Blob", interpreter->Object));
	Class* cls = CoerceToUserClass(blobClass);

	for (int i = 0; _BlobClassMethods[i].Name != NULL; i++) {
		ModuleFunction func = _BlobClassMethods[i];
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

	return blobClass;
}

Value* LoadCoreBlob(Interpreter* interpreter) {
	Value*	 module = NewObjectValue(interpreter);
	HashMap* map	= CoerceToHashMap(module);
	HashMapSet(map, "Blob", CreateBlobClass(interpreter));
	return module;
}
