
#include "./mongoose.h"

/* -----------------------------------------------------------------------
 * MIME type lookup table — sorted by extension for readability.
 * _MimeFromExt() accepts the result of strrchr(fname, '.'), i.e. the
 * dot is included (or NULL when there is no extension).
 * ----------------------------------------------------------------------- */

typedef struct {
	const String ext;  /**< File extension (without leading dot, e.g. "html") */
	const String mime; /**< MIME type string (e.g. "text/html") */
} MimeEntry;

static const MimeEntry _MimeTable[] = {
	/* Text */
	{ "css", "text/css" },
	{ "csv", "text/csv" },
	{ "htm", "text/html" },
	{ "html", "text/html" },
	{ "ics", "text/calendar" },
	{ "js", "text/javascript" },
	{ "mjs", "text/javascript" },
	{ "md", "text/markdown" },
	{ "txt", "text/plain" },
	{ "ts", "text/x-typescript" },
	{ "vtt", "text/vtt" },
	{ "xml", "text/xml" },
	/* Images */
	{ "avif", "image/avif" },
	{ "bmp", "image/bmp" },
	{ "gif", "image/gif" },
	{ "ico", "image/x-icon" },
	{ "jpeg", "image/jpeg" },
	{ "jpg", "image/jpeg" },
	{ "png", "image/png" },
	{ "svg", "image/svg+xml" },
	{ "tif", "image/tiff" },
	{ "tiff", "image/tiff" },
	{ "webp", "image/webp" },
	/* Audio */
	{ "aac", "audio/aac" },
	{ "flac", "audio/flac" },
	{ "m4a", "audio/mp4" },
	{ "mp3", "audio/mpeg" },
	{ "oga", "audio/ogg" },
	{ "ogg", "audio/ogg" },
	{ "opus", "audio/opus" },
	{ "wav", "audio/wav" },
	{ "weba", "audio/webm" },
	/* Video */
	{ "avi", "video/x-msvideo" },
	{ "m4v", "video/mp4" },
	{ "mkv", "video/x-matroska" },
	{ "mov", "video/quicktime" },
	{ "mp4", "video/mp4" },
	{ "mpeg", "video/mpeg" },
	{ "mpg", "video/mpeg" },
	{ "ogv", "video/ogg" },
	{ "ts", "video/mp2t" }, /* also text/x-typescript — audio wins per IANA */
	{ "webm", "video/webm" },
	/* Application */
	{ "7z", "application/x-7z-compressed" },
	{ "doc", "application/msword" },
	{ "docx",
	  "application/"
	  "vnd.openxmlformats-officedocument.wordprocessingml.document" },
	{ "epub", "application/epub+zip" },
	{ "gz", "application/gzip" },
	{ "json", "application/json" },
	{ "jsonld", "application/ld+json" },
	{ "mpkg", "application/vnd.apple.installer+xml" },
	{ "pdf", "application/pdf" },
	{ "ppt", "application/vnd.ms-powerpoint" },
	{ "pptx",
	  "application/"
	  "vnd.openxmlformats-officedocument.presentationml.presentation" },
	{ "rar", "application/vnd.rar" },
	{ "rtf", "application/rtf" },
	{ "sh", "application/x-sh" },
	{ "tar", "application/x-tar" },
	{ "wasm", "application/wasm" },
	{ "xls", "application/vnd.ms-excel" },
	{ "xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
	{ "xhtml", "application/xhtml+xml" },
	{ "zip", "application/zip" },
	/* Fonts */
	{ "eot", "application/vnd.ms-fontobject" },
	{ "otf", "font/otf" },
	{ "ttf", "font/ttf" },
	{ "woff", "font/woff" },
	{ "woff2", "font/woff2" },
};

#define _MIME_TABLE_LEN (sizeof(_MimeTable) / sizeof(_MimeTable[0]))


/* -----------------------------------------------------------------------
 * Internal binding property keys
 * ----------------------------------------------------------------------- */

#define PROP_APP_PTR "__ptr"
#define PROP_MG_GC_ROOTS                                                              \
	"__mg_gc_roots" /**< Keeps route/middleware Values visible                        \
					   to the GC (see AppState). */
#define PROP_RES_CTX	"__ctx"
#define PROP_RES_STATUS "__status"
#define PROP_RES_HDRSTR "__hdrstr"
#define PROP_REQ_CLASS	"__RequestClass"
#define PROP_RES_CLASS	"__ResponseClass"

/* -----------------------------------------------------------------------
 * Forward declarations
 * --------------------------------------------------------------------- */

/**
 * @brief Pushes value onto frame->Operand and increments frame->OperandC.
 * @param interpreter VM context (currently unused by the implementation).
 * @param frame Call frame whose operand stack is mutated.
 * @param value Value placed at the new top slot.
 * @origin src/interpreter.c
 */
extern void FPush(Interpreter* interpreter, CallFrame* frame, Value* value);

/**
 * @brief Pops and returns the top operand from frame.
 * @param interpreter VM context (currently unused by the implementation).
 * @param frame Call frame whose operand stack is decremented.
 * @return Value previously stored at frame->Operand[frame->OperandC - 1].
 * @origin src/interpreter.c
 */
extern Value* FPopp(Interpreter* interpreter, CallFrame* frame);

/**
 * @brief Dispatches fn as a callable: wires environments for user functions,
 * unwraps async targets into promises, validates native arity, consumes argc
 * stack operands, and leaves the callee result (or an Error value) per the
 * concrete callee kind.
 * @param interpreter Full VM state (stack, env stack, traces, active task).
 * @param fn User function, native function, promise continuation, or related
 * callable Value.
 * @param argc Operand count already present on the stack for this call (see
 * withThis for layout).
 * @param withThis When true, the lowest logical argument on the stack is bound
 * as the callee's this.
 * @return Callee result, interpreter->Null for async promise bootstrap paths,
 * or an Error Value on failure.
 * @origin src/operation.c
 */
extern Value*
DoCall(Interpreter* interpreter, CallFrame* frame, Value* fn, int argc, bool withThis);

/**
 * @brief Appends task to the ring buffer TaskQueue[(head + count) % STACK_SIZE]
 * and increments TaskQueueC.
 * @param interpreter VM whose bounded queue triggers InterpreterPanic if
 * TaskQueueC already equals STACK_SIZE.
 * @param task Promise/task Value retained by the queue until the event loop
 * dequeues it.
 * @origin src/interpreter.c
 */
extern void EnqueueTask(Interpreter* interpreter, Value* task);

/* -----------------------------------------------------------------------
 * Lookup a MIME type string for a file extension, e.g. ".jpg" → "image/jpeg".
 * --------------------------------------------------------------------- */
static const String _MimeFromExt(const String dot) {
	if (!dot || dot[1] == '\0')
		return "application/octet-stream";

	const String ext = dot + 1; /* skip the leading dot */

	/* Lower-case the extension into a small stack buffer. */
	char   low[16];
	size_t i = 0;
	for (; ext[i] && i < sizeof(low) - 1; i++)
		low[i] = (char) tolower((unsigned char) ext[i]);
	low[i] = '\0';

	/* Linear scan — table is small enough that binary search buys nothing. */
	for (size_t j = 0; j < _MIME_TABLE_LEN; j++)
		if (strcmp(_MimeTable[j].ext, low) == 0)
			return _MimeTable[j].mime;

	return "application/octet-stream";
}

/* -----------------------------------------------------------------------
 * Status text helper
 * ----------------------------------------------------------------------- */

static const String _StatusText(int code) {
	switch (code) {
		case 100:
			return "continue";
		case 101:
			return "switching protocols";
		case 102:
			return "processing";
		case 103:
			return "early hints";
		case 200:
			return "ok";
		case 201:
			return "created";
		case 202:
			return "accepted";
		case 203:
			return "non-authoritative information";
		case 204:
			return "no content";
		case 205:
			return "reset content";
		case 206:
			return "partial content";
		case 300:
			return "multiple choices";
		case 301:
			return "moved permanently";
		case 302:
			return "found";
		case 303:
			return "see other";
		case 304:
			return "not modified";
		case 307:
			return "temporary redirect";
		case 308:
			return "permanent redirect";
		case 400:
			return "bad request";
		case 401:
			return "unauthorized";
		case 402:
			return "payment required";
		case 403:
			return "forbidden";
		case 404:
			return "not found";
		case 405:
			return "method not allowed";
		case 406:
			return "not acceptable";
		case 407:
			return "proxy authentication required";
		case 408:
			return "request timeout";
		case 409:
			return "conflict";
		case 410:
			return "gone";
		case 411:
			return "length required";
		case 412:
			return "precondition failed";
		case 413:
			return "payload too large";
		case 414:
			return "uri too long";
		case 415:
			return "unsupported media type";
		case 416:
			return "range not satisfiable";
		case 417:
			return "expectation failed";
		case 418:
			return "i'm a teapot";
		case 421:
			return "misdirected request";
		case 422:
			return "unprocessable entity";
		case 423:
			return "locked";
		case 424:
			return "failed dependency";
		case 425:
			return "too early";
		case 426:
			return "upgrade required";
		case 428:
			return "precondition required";
		case 429:
			return "too many requests";
		case 431:
			return "request header fields too large";
		case 451:
			return "unavailable for legal reasons";
		case 500:
			return "internal server error";
		case 501:
			return "not implemented";
		case 502:
			return "bad gateway";
		case 503:
			return "service unavailable";
		case 504:
			return "gateway timeout";
		case 505:
			return "http version not supported";
		case 507:
			return "insufficient storage";
		case 508:
			return "loop detected";
		case 511:
			return "network authentication required";
		default:
			return "unknown";
	}
}

/* Escape a C string for safe embedding inside a JSON string literal. */
static String _JsonEscape(const String src) {
	size_t len = 0;
	for (String p = src; *p; p++) {
		switch (*p) {
			case '"':
				len += 2;
				break;
			case '\\':
				len += 2;
				break;
			case '\n':
				len += 2;
				break;
			case '\r':
				len += 2;
				break;
			case '\t':
				len += 2;
				break;
			default:
				len += 1;
				break;
		}
	}
	String out = (String) Allocate(len + 1);
	String d   = out;
	for (String p = src; *p; p++) {
		switch (*p) {
			case '"':
				*d++ = '\\';
				*d++ = '"';
				break;
			case '\\':
				*d++ = '\\';
				*d++ = '\\';
				break;
			case '\n':
				*d++ = '\\';
				*d++ = 'n';
				break;
			case '\r':
				*d++ = '\\';
				*d++ = 'r';
				break;
			case '\t':
				*d++ = '\\';
				*d++ = 't';
				break;
			default:
				*d++ = *p;
				break;
		}
	}
	*d = '\0';
	return out;
}

/* -----------------------------------------------------------------------
 * Internal C structures
 * ----------------------------------------------------------------------- */

#define MAX_MIDDLEWARE 32
#define ROUTE_GROW	   16

typedef struct {
	String Method;	/**< HTTP method string (e.g. "GET", "POST"), or NULL for
					   wildcard */
	String Path;	/**< URL path pattern used by mg_match() */
	Value* Handler; /**< Callable Value invoked when the route matches */
} Route;

typedef struct {
	Route*		 Routes;   /**< Array of registered route handlers */
	size_t		 Count;	   /**< Number of routes registered */
	size_t		 Capacity; /**< Allocated capacity of the Routes array */
	Value*		 Middleware[MAX_MIDDLEWARE]; /**< Array of middleware callbacks */
	size_t		 MwCount; /**< Number of registered middleware callbacks */
	Interpreter* Interp;  /**< Interpreter instance owning this server */
	struct mg_connection*
		   Listener;	  /**< Mongoose listening connection on interp->MgMgr */
	bool   Running;		  /**< True while the server event loop is active */
	Value* ReqClass;	  /**< Built-in Request class Value */
	Value* ResClass;	  /**< Built-in Response class Value */
} AppState;

typedef struct {
	struct mg_connection*	Conn;	   /**< Active Mongoose connection */
	struct mg_http_message* Msg;	   /**< Parsed HTTP request message */
	bool					Responded; /**< True once a response has been sent */
} ReqResCtx;

static bool _HasNamedParams(String routePath) {
	return routePath != NULL && strchr(routePath, ':') != NULL;
}

static bool _MatchNamedRoute(Interpreter* interp,
							 String		  routePath,
							 String		  reqPath,
							 HashMap*	  paramsMap) {
	if (routePath == NULL || reqPath == NULL) {
		return false;
	}

	size_t i		= 0;
	size_t j		= 0;
	size_t routeLen = strlen(routePath);
	size_t reqLen	= strlen(reqPath);

	for (;;) {
		while (i < routeLen && routePath[i] == '/')
			i++;
		while (j < reqLen && reqPath[j] == '/')
			j++;

		if (i >= routeLen || j >= reqLen) {
			break;
		}

		size_t segStartRoute = i;
		size_t segStartReq	 = j;

		while (i < routeLen && routePath[i] != '/')
			i++;
		while (j < reqLen && reqPath[j] != '/')
			j++;

		size_t segLenRoute = i - segStartRoute;
		size_t segLenReq   = j - segStartReq;

		if (segLenRoute == 0 || segLenReq == 0) {
			return false;
		}

		if (routePath[segStartRoute] == ':' && segLenRoute > 1) {
			size_t keyLen = segLenRoute - 1;
			String key	  = Allocate(keyLen + 1);
			memcpy(key, &routePath[segStartRoute + 1], keyLen);
			key[keyLen] = '\0';

			String val = Allocate(segLenReq + 1);
			memcpy(val, &reqPath[segStartReq], segLenReq);
			val[segLenReq] = '\0';

			HashMapSet(paramsMap, key, NewStrValue(interp, val));
			free(key);
			free(val);
		} else {
			if (segLenRoute != segLenReq
				|| strncmp(&routePath[segStartRoute],
						   &reqPath[segStartReq],
						   segLenRoute)
					   != 0) {
				return false;
			}
		}
	}

	while (i < routeLen && routePath[i] == '/')
		i++;
	while (j < reqLen && reqPath[j] == '/')
		j++;

	return i == routeLen && j == reqLen;
}

/* -----------------------------------------------------------------------
 * AppState helpers  (Server ClassInstance)
 * ----------------------------------------------------------------------- */

static AppState* _GetApp(ClassInstance* cls) {
	Value* v = (Value*) HashMapGet(cls->Members, PROP_APP_PTR);
	if (v && ValueIsOpaquePtr(v))
		return (AppState*) v->Value.Opaque;
	return NULL;
}

static void _AppStateFree(AppState* app) {
	for (size_t i = 0; i < app->Count; i++) {
		free(app->Routes[i].Method); /* strdup'd, or NULL for wildcard */
		free(app->Routes[i].Path);
	}
	free(app->Routes);
	free(app);
}

static void _PushMgGcRoot(ClassInstance* cls, Value* handler) {
	Value* rootsVal = (Value*) HashMapGet(cls->Members, PROP_MG_GC_ROOTS);
	if (rootsVal != NULL && ValueIsArray(rootsVal))
		ArrayPush(CoerceToArray(rootsVal), handler);
}

static void _AppAddRoute(ClassInstance* cls,
						 AppState*		app,
						 const String	method,
						 const String	path,
						 Value*			handler) {
	if (app->Count >= app->Capacity) {
		app->Capacity += ROUTE_GROW;
		app->Routes	   = realloc(app->Routes, sizeof(Route) * app->Capacity);
	}
	app->Routes[app->Count].Method	= method ? strdup(method) : NULL;
	app->Routes[app->Count].Path	= strdup(path);
	app->Routes[app->Count].Handler = handler;
	app->Count++;
	_PushMgGcRoot(cls, handler);
}

/* -----------------------------------------------------------------------
 * ReqResCtx helpers  (Response ClassInstance)
 * ----------------------------------------------------------------------- */

static ReqResCtx* _GetCtx(ClassInstance* cls) {
	Value* v = (Value*) HashMapGet(cls->Members, PROP_RES_CTX);
	if (v && ValueIsOpaquePtr(v))
		return (ReqResCtx*) v->Value.Opaque;
	return NULL;
}

/* -----------------------------------------------------------------------
 * Response class methods
 * ----------------------------------------------------------------------- */

static Value* _ResInit(Interpreter* interp, int argc, Value** args) {
	(void) argc;
	(void) args;
	return NewErrorFValue(interp,
						  "%s: Response cannot be constructed directly",
						  RUNTIME_ERROR);
}

/* res.send(body) */
static Value* _ResSend(Interpreter* interp, int argc, Value** args) {
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	ReqResCtx*	   ctx = _GetCtx(cls);
	if (!ctx)
		return NewErrorFValue(interp,
							  "%s: res.send(): invalid context",
							  RUNTIME_ERROR);
	if (ctx->Responded)
		return interp->Null;

	Value* sv	  = (Value*) HashMapGet(cls->Members, PROP_RES_STATUS);
	int	   status = (sv && ValueIsInt(sv)) ? sv->Value.I32 : 200;
	Value* hv	  = (Value*) HashMapGet(cls->Members, PROP_RES_HDRSTR);
	String extra  = (hv && ValueIsStr(hv)) ? ValueToString(hv) : strdup("");
	String body	  = (argc >= 2) ? ValueToString(args[1]) : strdup("");

	String		 escaped = _JsonEscape(body);
	const String stxt	 = _StatusText(status);
	size_t		 wrapLen = strlen(escaped) + strlen(stxt) + 128;
	String		 wrapped = (String) Allocate(wrapLen);
	snprintf(wrapped,
			 wrapLen,
			 "{\"status\":%d,\"statusText\":\"%s\",\"data\":\"%s\"}",
			 status,
			 stxt,
			 escaped);

	size_t hlen	   = strlen(extra) + 64;
	String headers = (String) Allocate(hlen);
	snprintf(headers, hlen, "Content-Type: application/json\r\n%s", extra);

	mg_http_reply(ctx->Conn, status, headers, "%s", wrapped);
	ctx->Responded = true;
	free(body);
	free(escaped);
	free(wrapped);
	free(extra);
	free(headers);
	return interp->Null;
}

/* res.json(obj) */
static Value* _ResJson(Interpreter* interp, int argc, Value** args) {
	if (argc < 2)
		return NewErrorFValue(interp,
							  "%s: res.json() requires a body argument",
							  ARGUMENT_ERROR);

	ClassInstance* cls = CoerceToClassInstance(args[0]);
	ReqResCtx*	   ctx = _GetCtx(cls);
	if (!ctx)
		return NewErrorFValue(interp,
							  "%s: res.json(): invalid context",
							  RUNTIME_ERROR);
	if (ctx->Responded)
		return interp->Null;

	Value* sv	  = (Value*) HashMapGet(cls->Members, PROP_RES_STATUS);
	int	   status = (sv && ValueIsInt(sv)) ? sv->Value.I32 : 200;
	Value* hv	  = (Value*) HashMapGet(cls->Members, PROP_RES_HDRSTR);
	String extra  = (hv && ValueIsStr(hv)) ? ValueToString(hv) : strdup("");

	size_t hlen	   = strlen(extra) + 64;
	String headers = (String) Allocate(hlen);
	snprintf(headers, hlen, "Content-Type: application/json\r\n%s", extra);
	free(extra);

	String		 body	 = ValueToString(args[1]);
	const String stxt	 = _StatusText(status);
	size_t		 wrapLen = strlen(body) + strlen(stxt) + 128;
	String		 wrapped = (String) Allocate(wrapLen);
	snprintf(wrapped,
			 wrapLen,
			 "{\"status\":%d,\"statusText\":\"%s\",\"data\":%s}",
			 status,
			 stxt,
			 body);

	mg_http_reply(ctx->Conn, status, headers, "%s", wrapped);
	ctx->Responded = true;
	free(body);
	free(wrapped);
	free(headers);
	return interp->Null;
}

/* res.status(code) → this  (chainable) */
static Value* _ResStatus(Interpreter* interp, int argc, Value** args) {
	if (argc < 2 || !ValueIsAnyNum(args[1]))
		return NewErrorFValue(interp,
							  "%s: res.status() requires a numeric status code",
							  TYPE_ERROR);
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	HashMapSet(cls->Members,
			   PROP_RES_STATUS,
			   NewIntValue(interp, (int) CoerceToNum(args[1])));
	return args[0];
}

/* res.redirect(url) */
static Value* _ResRedirect(Interpreter* interp, int argc, Value** args) {
	if (argc < 2 || !ValueIsStr(args[1]))
		return NewErrorFValue(interp,
							  "%s: res.redirect() requires a URL string argument",
							  TYPE_ERROR);
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	ReqResCtx*	   ctx = _GetCtx(cls);
	if (!ctx)
		return NewErrorFValue(interp,
							  "%s: res.redirect(): invalid context",
							  RUNTIME_ERROR);
	if (ctx->Responded)
		return interp->Null;

	String location = ValueToString(args[1]);
	String escaped	= _JsonEscape(location);
	char   header[512];
	snprintf(header,
			 sizeof(header),
			 "Location: %s\r\nContent-Type: application/json\r\n",
			 location);
	char wrapped[1024];
	snprintf(wrapped,
			 sizeof(wrapped),
			 "{\"status\":302,\"statusText\":\"found\",\"data\":\"%s\"}",
			 escaped);
	mg_http_reply(ctx->Conn, 302, header, "%s", wrapped);
	ctx->Responded = true;
	free(location);
	free(escaped);
	return interp->Null;
}

/* res.setHeader(name, value) → this */
static Value* _ResSetHeader(Interpreter* interp, int argc, Value** args) {
	if (argc < 3 || !ValueIsStr(args[1]) || !ValueIsStr(args[2]))
		return NewErrorFValue(interp,
							  "%s: res.setHeader() requires (name, value) strings",
							  TYPE_ERROR);
	ClassInstance* cls	= CoerceToClassInstance(args[0]);
	String		   name = ValueToString(args[1]);
	String		   val	= ValueToString(args[2]);
	Value*		   curV = (Value*) HashMapGet(cls->Members, PROP_RES_HDRSTR);
	String		   cur = (curV && ValueIsStr(curV)) ? ValueToString(curV) : strdup("");
	char		   extra[512];
	snprintf(extra, sizeof(extra), "%s: %s\r\n", name, val);
	size_t newlen = strlen(cur) + strlen(extra) + 1;
	String merged = (String) Allocate(newlen);
	snprintf(merged, newlen, "%s%s", cur, extra);
	HashMapSet(cls->Members, PROP_RES_HDRSTR, NewStrValue(interp, merged));
	free(name);
	free(val);
	free(cur);
	free(merged);
	return args[0];
}

static ModuleFunction _ResClassMethods[] = {
	{ .Name		 = CONSTRUCTOR_NAME,
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _ResInit,
	  .Value	 = NULL },
	{ .Name		 = "send",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _ResSend,
	  .Value	 = NULL },
	{ .Name		 = "json",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _ResJson,
	  .Value	 = NULL },
	{ .Name		 = "status",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _ResStatus,
	  .Value	 = NULL },
	{ .Name		 = "redirect",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _ResRedirect,
	  .Value	 = NULL },
	{ .Name		 = "setHeader",
	  .Argc		 = 3,
	  .CFunction = (NativeFunctionCallback) _ResSetHeader,
	  .Value	 = NULL },
	{ .Name = NULL }
};

/* -----------------------------------------------------------------------
 * Request class methods
 * ----------------------------------------------------------------------- */

static Value* _ReqInit(Interpreter* interp, int argc, Value** args) {
	(void) argc;
	(void) args;
	return NewErrorFValue(interp,
						  "%s: Request cannot be constructed directly",
						  RUNTIME_ERROR);
}

static ModuleFunction _ReqClassMethods[] = { { .Name = CONSTRUCTOR_NAME,
											   .Argc = VARARG,
											   .CFunction =
												   (NativeFunctionCallback) _ReqInit,
											   .Value = NULL },
											 { .Name = NULL } };

/* -----------------------------------------------------------------------
 * JSON body → Value* recursive converter using mongoose's JSON API
 * ----------------------------------------------------------------------- */

static Value* _JsonToValue(Interpreter* interp, struct mg_str tok) {
	if (tok.len == 0)
		return interp->Null;

	char first = tok.buf[0];

	/* object */
	if (first == '{') {
		Value*		  obj = NewObjectValue(interp);
		HashMap*	  map = CoerceToHashMap(obj);
		size_t		  ofs = 0;
		struct mg_str key, val;
		while ((ofs = mg_json_next(tok, ofs, &key, &val)) > 0) {
			/* key arrives as a quoted JSON string — unescape it */
			size_t kbuf_sz = key.len + 1;
			String kbuf	   = (String) Allocate(kbuf_sz);
			if (key.len >= 2 && key.buf[0] == '"') {
				mg_json_unescape(mg_str_n(key.buf + 1, key.len - 2), kbuf, kbuf_sz);
			} else {
				memcpy(kbuf, key.buf, key.len);
				kbuf[key.len] = '\0';
			}
			HashMapSet(map, kbuf, _JsonToValue(interp, val));
			free(kbuf);
		}
		return obj;
	}

	/* array */
	if (first == '[') {
		Value*		  arr = NewArrayValue(interp);
		Array*		  a	  = CoerceToArray(arr);
		size_t		  ofs = 0;
		struct mg_str val;
		while ((ofs = mg_json_next(tok, ofs, NULL, &val)) > 0)
			ArrayPush(a, _JsonToValue(interp, val));
		return arr;
	}

	/* string */
	if (first == '"') {
		size_t bsz = tok.len + 1;
		String buf = (String) Allocate(bsz);
		mg_json_unescape(mg_str_n(tok.buf + 1, tok.len - 2), buf, bsz);
		Value* sv = NewStrValue(interp, buf);
		free(buf);
		return sv;
	}

	/* null */
	if (tok.len == 4 && memcmp(tok.buf, "null", 4) == 0)
		return interp->Null;

	/* boolean */
	if (tok.len == 4 && memcmp(tok.buf, "true", 4) == 0)
		return NewBoolValue(interp, 1);
	if (tok.len == 5 && memcmp(tok.buf, "false", 5) == 0)
		return NewBoolValue(interp, 0);

	/* number */
	double dv = 0.0;
	mg_json_get_num(tok, "$", &dv);
	if (dv == (int) dv)
		return NewIntValue(interp, (int) dv);
	return NewNumValue(interp, dv);
}

/* Case-insensitive substring match inside an mg_str header value */
static bool
_HeaderContains(struct mg_http_message* hm, const String hdr, const String needle) {
	struct mg_str* ct = mg_http_get_header(hm, hdr);
	if (!ct)
		return false;
	size_t nlen = strlen(needle);
	for (size_t j = 0; j + nlen <= ct->len; j++) {
		if (strncasecmp(ct->buf + j, needle, nlen) == 0)
			return true;
	}
	return false;
}

static bool _IsJsonContentType(struct mg_http_message* hm) {
	return _HeaderContains(hm, "Content-Type", "application/json");
}

static bool _IsFormContentType(struct mg_http_message* hm) {
	return _HeaderContains(hm, "Content-Type", "application/x-www-form-urlencoded");
}

static bool _IsMultipartContentType(struct mg_http_message* hm) {
	return _HeaderContains(hm, "Content-Type", "multipart/form-data");
}

/* Parse an application/x-www-form-urlencoded body into an object Value */
static Value* _FormToObject(Interpreter* interp, struct mg_str body) {
	Value*	 obj = NewObjectValue(interp);
	HashMap* map = CoerceToHashMap(obj);

	String p   = body.buf;
	String end = body.buf + body.len;

	while (p < end) {
		/* find end of this key=value pair */
		String amp = memchr(p, '&', (size_t) (end - p));
		if (!amp)
			amp = end;

		/* find '=' separator */
		const String eq = memchr(p, '=', (size_t) (amp - p));

		char keyBuf[512], valBuf[2048];

		if (eq) {
			mg_url_decode(p, (size_t) (eq - p), keyBuf, sizeof(keyBuf), 1);
			mg_url_decode(eq + 1, (size_t) (amp - eq - 1), valBuf, sizeof(valBuf), 1);
		} else {
			mg_url_decode(p, (size_t) (amp - p), keyBuf, sizeof(keyBuf), 1);
			valBuf[0] = '\0';
		}

		HashMapSet(map, keyBuf, NewStrValue(interp, valBuf));
		p = amp < end ? amp + 1 : end;
	}
	return obj;
}

/* Parse a multipart/form-data body into an object Value.
 * Regular text fields become string values.
 * File fields become objects: { filename, data, size }
 * where data is a Blob holding the raw binary content. */
static Value* _MultipartToObject(Interpreter* interp, struct mg_http_message* hm) {
	Value*				obj = NewObjectValue(interp);
	HashMap*			map = CoerceToHashMap(obj);
	struct mg_http_part part;
	size_t				ofs = 0;

	while ((ofs = mg_http_next_multipart(hm->body, ofs, &part)) > 0) {
		if (part.name.len == 0)
			continue;
		char   key[512];
		size_t klen =
			part.name.len < sizeof(key) - 1 ? part.name.len : sizeof(key) - 1;
		memcpy(key, part.name.buf, klen);
		key[klen] = '\0';

		if (part.filename.len > 0) {
			/* File upload — wrap in an object with Blob data */
			Value*	 fileObj = NewObjectValue(interp);
			HashMap* fileMap = CoerceToHashMap(fileObj);

			char   fname[512];
			size_t flen = part.filename.len < sizeof(fname) - 1 ? part.filename.len
																: sizeof(fname) - 1;
			memcpy(fname, part.filename.buf, flen);
			fname[flen] = '\0';
			HashMapSet(fileMap, "filename", NewStrValue(interp, fname));

			/* Guess MIME type from file extension */
			const String mime = _MimeFromExt(strrchr(fname, '.'));

			HashMapSet(fileMap,
					   "data",
					   NewBlobValue(interp,
									(uint8_t*) part.body.buf,
									part.body.len,
									(String) mime));
			HashMapSet(fileMap, "size", NewIntValue(interp, (int) part.body.len));

			HashMapSet(map, key, fileObj);
		} else {
			/* Regular text field */
			String val = (String) Allocate(part.body.len + 1);
			memcpy(val, part.body.buf, part.body.len);
			val[part.body.len] = '\0';

			HashMapSet(map, key, NewStrValue(interp, val));
			free(val);
		}
	}
	return obj;
}

/* -----------------------------------------------------------------------
 * Build req / res class instances per-request
 * ----------------------------------------------------------------------- */

static Value* _BuildResInstance(Interpreter* interp, Value* resClass, ReqResCtx* ctx) {
	ClassInstance* inst = CreateClassInstance(resClass);
	HashMapSet(inst->Members, PROP_RES_CTX, NewOpquePtrValue(interp, ctx));
	HashMapSet(inst->Members, PROP_RES_STATUS, NewIntValue(interp, 200));
	HashMapSet(inst->Members, PROP_RES_HDRSTR, NewStrValue(interp, ""));
	return NewClassInstanceValue(interp, inst);
}

static Value*
_BuildReqInstance(Interpreter* interp, Value* reqClass, struct mg_http_message* hm) {
	ClassInstance* inst = CreateClassInstance(reqClass);

	char method[16];
	snprintf(method, sizeof(method), "%.*s", (int) hm->method.len, hm->method.buf);
	HashMapSet(inst->Members, "method", NewStrValue(interp, method));

	char path[1024];
	snprintf(path, sizeof(path), "%.*s", (int) hm->uri.len, hm->uri.buf);
	HashMapSet(inst->Members, "url", NewStrValue(interp, path));
	HashMapSet(inst->Members, "path", NewStrValue(interp, path));

	char query[2048] = "";
	if (hm->query.len > 0)
		snprintf(query, sizeof(query), "%.*s", (int) hm->query.len, hm->query.buf);
	HashMapSet(inst->Members, "query", NewStrValue(interp, query));

	/* body: parse as JSON if application/json, as object if
	 * application/x-www-form-urlencoded or multipart/form-data,
	 * otherwise plain string */
	Value* bodyVal;
	if (hm->body.len > 0 && _IsJsonContentType(hm)) {
		bodyVal = _JsonToValue(interp, mg_str_n(hm->body.buf, hm->body.len));
	} else if (hm->body.len > 0 && _IsFormContentType(hm)) {
		bodyVal = _FormToObject(interp, mg_str_n(hm->body.buf, hm->body.len));
	} else if (hm->body.len > 0 && _IsMultipartContentType(hm)) {
		bodyVal = _MultipartToObject(interp, hm);
	} else if (hm->body.len > 0) {
		String body = (String) Allocate(hm->body.len + 1);
		memcpy(body, hm->body.buf, hm->body.len);
		body[hm->body.len] = '\0';
		bodyVal			   = NewStrValue(interp, body);
		free(body);
	} else {
		bodyVal = NewStrValue(interp, "");
	}
	HashMapSet(inst->Members, "body", bodyVal);

	Value*	 hdrsObj = NewObjectValue(interp);
	HashMap* hdrsMap = CoerceToHashMap(hdrsObj);
	for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
		if (hm->headers[i].name.len == 0)
			break;
		char hname[256], hval[1024];
		snprintf(hname,
				 sizeof(hname),
				 "%.*s",
				 (int) hm->headers[i].name.len,
				 hm->headers[i].name.buf);
		snprintf(hval,
				 sizeof(hval),
				 "%.*s",
				 (int) hm->headers[i].value.len,
				 hm->headers[i].value.buf);
		for (String p = hname; *p; p++)
			*p = (char) tolower((unsigned char) *p);
		HashMapSet(hdrsMap, hname, NewStrValue(interp, hval));
	}
	HashMapSet(inst->Members, "headers", hdrsObj);
	HashMapSet(inst->Members, "params", NewObjectValue(interp));

	return NewClassInstanceValue(interp, inst);
}

/* -----------------------------------------------------------------------
 * Mongoose event handler
 * ----------------------------------------------------------------------- */

static void _EvHandler(struct mg_connection* c, int ev, void* ev_data) {
	AppState* app = (AppState*) c->fn_data;

	if (ev != MG_EV_HTTP_MSG)
		return;

	struct mg_http_message* hm	   = (struct mg_http_message*) ev_data;
	Interpreter*			interp = app->Interp;

	ReqResCtx ctx = { .Conn = c, .Msg = hm, .Responded = false };

	Value* reqVal = _BuildReqInstance(interp, app->ReqClass, hm);
	Value* resVal = _BuildResInstance(interp, app->ResClass, &ctx);

	/* Middleware */
	for (size_t i = 0; i < app->MwCount && !ctx.Responded; i++) {
		Value* mw = app->Middleware[i];
		if (!ValueIsCallable(mw))
			continue;
		// if (interp->CurrentFrame == NULL)
		// 	continue;
		// FPush(interp, interp->CurrentFrame, reqVal);
		// FPush(interp, interp->CurrentFrame, resVal);
		// DoCall(interp, interp->CurrentFrame, mw, 2, false);
		// FPopp(interp, interp->CurrentFrame);
	}

	if (ctx.Responded)
		return;

	/* Route matching */
	char uri[1024];
	snprintf(uri, sizeof(uri), "%.*s", (int) hm->uri.len, hm->uri.buf);

	char meth[16];
	snprintf(meth, sizeof(meth), "%.*s", (int) hm->method.len, hm->method.buf);

	bool matched = false;
	for (size_t i = 0; i < app->Count && !matched; i++) {
		Route* r = &app->Routes[i];

		if (r->Method && strcasecmp(r->Method, meth) != 0)
			continue;

		Value*	 paramsObj = NewObjectValue(interp);
		HashMap* paramsMap = CoerceToHashMap(paramsObj);
		bool	 routeMatched;

		if (_HasNamedParams(r->Path)) {
			routeMatched = _MatchNamedRoute(interp, r->Path, uri, paramsMap);
		} else {
			struct mg_str caps[4];
			memset(caps, 0, sizeof(caps));
			routeMatched = mg_match(mg_str_n(uri, strlen(uri)), mg_str(r->Path), caps);
			if (routeMatched) {
				for (int k = 0; k < 4; k++) {
					if (caps[k].len == 0)
						break;
					char capKey[16], capBuf[512];
					snprintf(capKey, sizeof(capKey), "%d", k);
					snprintf(capBuf,
							 sizeof(capBuf),
							 "%.*s",
							 (int) caps[k].len,
							 caps[k].buf);
					HashMapSet(paramsMap, capKey, NewStrValue(interp, capBuf));
				}
			}
		}

		if (!routeMatched)
			continue;

		matched = true;

		/* Populate req.params with named/wildcard captures */
		ClassInstance* reqInst = CoerceToClassInstance(reqVal);
		HashMapSet(reqInst->Members, "params", paramsObj);

		// if (interp->CurrentFrame == NULL)
		// 	continue;
		// FPush(interp, interp->CurrentFrame, reqVal);
		// FPush(interp, interp->CurrentFrame, resVal);
		// DoCall(interp, interp->CurrentFrame, r->Handler, 2, false);
		// Value* result = FPopp(interp, interp->CurrentFrame);

		// if ((ValueIsPromise(result) && CoerceToPromise(result)->State == REJECTED))
		// { 	Promise* promise = CoerceToPromise(result); 	String	 errMsg	 =
		// ValueToString(promise->Result); 	String	 escaped = _JsonEscape(errMsg);
		// 	size_t	 wlen	 = strlen(escaped) + 128;
		// 	String	 wrapped = (String) Allocate(wlen);
		// 	snprintf(wrapped,
		// 			 wlen,
		// 			 "{\"status\":500,\"statusText\":\"internal server error\","
		// 			 "\"data\":\"%s\"}",
		// 			 escaped);
		// 	mg_http_reply(c, 500, "Content-Type: application/json\r\n", "%s", wrapped);
		// 	ctx.Responded = true;
		// 	free(errMsg);
		// 	free(escaped);
		// 	free(wrapped);
		// }
	}

	if (!matched && !ctx.Responded)
		mg_http_reply(c,
					  404,
					  "Content-Type: application/json\r\n",
					  "{\"status\":404,\"statusText\":\"not found\",\"data\":\"\"}");
}

/* -----------------------------------------------------------------------
 * Server class methods
 * ----------------------------------------------------------------------- */

static Value* _ServerInit(Interpreter* interp, int argc, Value** args) {
	(void) argc;
	ClassInstance* cls		 = CoerceToClassInstance(args[0]);
	Class*		   serverCls = CoerceToUserClass(cls->Proto);

	Value* reqClass = ClassGetMember(serverCls, PROP_REQ_CLASS, true);
	Value* resClass = ClassGetMember(serverCls, PROP_RES_CLASS, true);
	if (!reqClass || !resClass)
		return NewErrorFValue(interp,
							  "%s: internal: Request/Response class not found",
							  RUNTIME_ERROR);

	AppState* app = (AppState*) Callocate(1, sizeof(AppState));
	app->Interp	  = interp;
	app->Running  = false;
	app->ReqClass = reqClass;
	app->ResClass = resClass;

	HashMapSet(cls->Members, PROP_MG_GC_ROOTS, NewArrayValue(interp));
	HashMapSet(cls->Members, PROP_APP_PTR, NewOpquePtrValue(interp, app));
	return interp->Null;
}

#define DEFINE_ROUTE_METHOD(ZsName, HttpMethod)                                       \
	static Value* _App##ZsName(Interpreter* interp, int argc, Value** args) {         \
		if (argc != 3 || !ValueIsStr(args[1]) || !ValueIsCallable(args[2]))           \
			return NewErrorFValue(interp,                                             \
								  "%s: " #ZsName "() requires (path, handler): "      \
								  "path string and callable",                         \
								  ARGUMENT_ERROR);                                    \
		ClassInstance* cls = CoerceToClassInstance(args[0]);                          \
		AppState*	   app = _GetApp(cls);                                            \
		if (!app)                                                                     \
			return NewErrorFValue(interp,                                             \
								  "%s: " #ZsName "(): server not initialised",        \
								  RUNTIME_ERROR);                                     \
		String path = ValueToString(args[1]);                                         \
		_AppAddRoute(cls, app, HttpMethod, path, args[2]);                            \
		free(path);                                                                   \
		return args[0];                                                               \
	}

DEFINE_ROUTE_METHOD(Get, "GET")
DEFINE_ROUTE_METHOD(Post, "POST")
DEFINE_ROUTE_METHOD(Put, "PUT")
DEFINE_ROUTE_METHOD(Delete, "DELETE")
DEFINE_ROUTE_METHOD(Patch, "PATCH")
DEFINE_ROUTE_METHOD(All, NULL)

static Value* _AppUse(Interpreter* interp, int argc, Value** args) {
	if (argc < 2 || !ValueIsCallable(args[1]))
		return NewErrorFValue(interp,
							  "%s: use() requires a callback function",
							  TYPE_ERROR);
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	AppState*	   app = _GetApp(cls);
	if (!app)
		return NewErrorFValue(interp,
							  "%s: use(): server not initialised",
							  RUNTIME_ERROR);
	if (app->MwCount >= MAX_MIDDLEWARE)
		return NewErrorFValue(interp,
							  "%s: use(): maximum middleware count reached",
							  RUNTIME_ERROR);
	app->Middleware[app->MwCount++] = args[1];
	_PushMgGcRoot(cls, args[1]);
	return args[0];
}

static Value* _AppListen(Interpreter* interp, int argc, Value** args) {
	if (argc < 2 || !ValueIsAnyNum(args[1]))
		return NewErrorFValue(interp,
							  "%s: listen() requires a port number argument",
							  TYPE_ERROR);
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	AppState*	   app = _GetApp(cls);
	if (!app)
		return NewErrorFValue(interp,
							  "%s: listen(): server not initialised",
							  RUNTIME_ERROR);

	int	   port = (int) CoerceToNum(args[1]);
	Value* cb	= (argc >= 3 && ValueIsCallable(args[2])) ? args[2] : NULL;

	char url[64];
	snprintf(url, sizeof(url), "http://0.0.0.0:%d", port);

	// Register the listener on the interpreter's shared mongoose
	// manager. The interpreter event loop drives mg_mgr_poll().
	struct mg_connection* lc = mg_http_listen(&interp->MgMgr, url, _EvHandler, app);
	if (lc == NULL) {
		_AppStateFree(app);
		HashMapSet(cls->Members, PROP_APP_PTR, NewOpquePtrValue(interp, NULL));
		return NewErrorFValue(interp,
							  "%s: listen(): failed to bind on port %d",
							  RUNTIME_ERROR,
							  port);
	}

	app->Listener = lc;
	app->Running  = true;

	if (cb) {
		String msg	  = FormatString("Server listening on port %d", port);
		Value* msgVal = NewStrValue(interp, msg);
		free(msg);
		// if (interp->CurrentFrame != NULL) {
		// 	FPush(interp, interp->CurrentFrame, msgVal);
		// 	DoCall(interp, interp->CurrentFrame, cb, 1, false);
		// 	FPopp(interp, interp->CurrentFrame);
		// }
	}

	return interp->Null;
}

static Value* _AppClose(Interpreter* interp, int argc, Value** args) {
	(void) argc;
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	AppState*	   app = _GetApp(cls);
	if (app && app->Running) {
		app->Running = false;
		if (app->Listener) {
			app->Listener->is_closing = 1;
			app->Listener			  = NULL;
		}
		_AppStateFree(app);
		HashMapSet(cls->Members, PROP_APP_PTR, NewOpquePtrValue(interp, NULL));
	}
	return interp->Null;
}

static ModuleFunction _ServerClassMethods[] = {
	{ .Name		 = CONSTRUCTOR_NAME,
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _ServerInit,
	  .Value	 = NULL },
	{ .Name		 = "get",
	  .Argc		 = 3,
	  .CFunction = (NativeFunctionCallback) _AppGet,
	  .Value	 = NULL },
	{ .Name		 = "post",
	  .Argc		 = 3,
	  .CFunction = (NativeFunctionCallback) _AppPost,
	  .Value	 = NULL },
	{ .Name		 = "put",
	  .Argc		 = 3,
	  .CFunction = (NativeFunctionCallback) _AppPut,
	  .Value	 = NULL },
	{ .Name		 = "delete",
	  .Argc		 = 3,
	  .CFunction = (NativeFunctionCallback) _AppDelete,
	  .Value	 = NULL },
	{ .Name		 = "patch",
	  .Argc		 = 3,
	  .CFunction = (NativeFunctionCallback) _AppPatch,
	  .Value	 = NULL },
	{ .Name		 = "all",
	  .Argc		 = 3,
	  .CFunction = (NativeFunctionCallback) _AppAll,
	  .Value	 = NULL },
	{ .Name		 = "use",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _AppUse,
	  .Value	 = NULL },
	{ .Name		 = "listen",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _AppListen,
	  .Value	 = NULL },
	{ .Name		 = "close",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _AppClose,
	  .Value	 = NULL },
	{ .Name = NULL }
};

/* -----------------------------------------------------------------------
 * Class factory — identical to sqlite.c pattern
 * ----------------------------------------------------------------------- */

static Value*
_BuildClass(Interpreter* interp, const String name, ModuleFunction methods[]) {
	Value* classVal =
		NewClassValue(interp, CreateUserClass((String) name, interp->Object));
	Class* cls = CoerceToUserClass(classVal);

	for (int i = 0; methods[i].Name != NULL; i++) {
		ModuleFunction* func = &methods[i];
		if (func->CFunction) {
			ClassDefineMemberByString(
				cls,
				func->Name,
				NewNativeFunctionValue(interp,
									   CreateNativeFunctionMeta((String) func->Name,
																func->Argc,
																func->CFunction)),
				false);
		}
	}
	return classVal;
}

/* -----------------------------------------------------------------------
 * Internal structures
 *
 * AppState  – held as an opaque pointer inside the JS "app" object.
 *             It owns the route table and the global middleware list.
 *
 * Route     – one registered route (method + path pattern + handler).
 * RouteList – a simple growable array of routes.
 * ----------------------------------------------------------------------- */

#define MAX_MIDDLEWARE 32

/* -----------------------------------------------------------------------
 * request(url [, options]) → Promise
 *
 * Non-blocking HTTP client using the interpreter's shared mg_mgr.
 * Returns a promise that resolves with a response object:
 *   { status, statusText, headers, body }
 *
 * options (optional object):
 *   method  – "GET" (default), "POST", "PUT", "DELETE", etc.
 *   headers – object of header key/value pairs
 *   body    – string body for POST/PUT
 * ----------------------------------------------------------------------- */

typedef struct {
	Interpreter* interp;   /**< Interpreter instance owning the fetch */
	Value*		 promise;  /**< Promise Value to resolve/reject on completion */
	char*		 host;	   /**< Host header value for the request */
	char*		 uri;	   /**< Request URI path */
	char*		 method;   /**< HTTP method string (e.g. "GET", "POST") */
	char*		 extraHdr; /**< Additional raw headers string (may be NULL) */
	char*		 bodyStr;  /**< Request body payload (may be NULL) */
	bool		 sent;	   /**< True once the request line has been sent */
} FetchCtx;

static void _FetchEvHandler(struct mg_connection* c, int ev, void* ev_data) {
	FetchCtx* ctx = (FetchCtx*) c->fn_data;
	if (ctx == NULL)
		return; /* already handled / closing */

	if (ev == MG_EV_CONNECT) {
		/* Connection established — send the HTTP request */
		if (!ctx->sent) {
			ctx->sent	   = true;
			size_t bodyLen = ctx->bodyStr ? strlen(ctx->bodyStr) : 0;
			mg_printf(c,
					  "%s %s HTTP/1.1\r\n"
					  "Host: %s\r\n"
					  "Content-Length: %lu\r\n"
					  "%s"
					  "\r\n",
					  ctx->method,
					  ctx->uri,
					  ctx->host,
					  (unsigned long) bodyLen,
					  ctx->extraHdr ? ctx->extraHdr : "");
			if (bodyLen > 0)
				mg_send(c, ctx->bodyStr, bodyLen);
		}
		return;
	}

	if (ev == MG_EV_HTTP_MSG) {
		struct mg_http_message* hm	   = (struct mg_http_message*) ev_data;
		Interpreter*			interp = ctx->interp;

		/* Build response object: { status, statusText, headers, body } */
		Value*	 obj = NewObjectValue(interp);
		HashMap* map = CoerceToHashMap(obj);

		/* status (int) */
		int code = mg_http_status(hm);
		HashMapSet(map, "status", NewIntValue(interp, code));
		HashMapSet(map, "statusText", NewStrValue(interp, (String) _StatusText(code)));

		/* headers (object) */
		Value*	 hdrObj = NewObjectValue(interp);
		HashMap* hdrMap = CoerceToHashMap(hdrObj);
		for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
			if (hm->headers[i].name.len == 0)
				break;
			char hname[256], hval[2048];
			snprintf(hname,
					 sizeof(hname),
					 "%.*s",
					 (int) hm->headers[i].name.len,
					 hm->headers[i].name.buf);
			snprintf(hval,
					 sizeof(hval),
					 "%.*s",
					 (int) hm->headers[i].value.len,
					 hm->headers[i].value.buf);
			for (char* p = hname; *p; p++)
				*p = (char) tolower((unsigned char) *p);
			HashMapSet(hdrMap, hname, NewStrValue(interp, hval));
		}
		HashMapSet(map, "headers", hdrObj);

		/* body — try to parse as JSON, fall back to string */
		if (hm->body.len > 0
			&& _HeaderContains(hm, "Content-Type", "application/json")) {
			HashMapSet(map,
					   "body",
					   _JsonToValue(interp, mg_str_n(hm->body.buf, hm->body.len)));
		} else {
			char* body = (char*) Allocate(hm->body.len + 1);
			memcpy(body, hm->body.buf, hm->body.len);
			body[hm->body.len] = '\0';
			HashMapSet(map, "body", NewStrValue(interp, body));
			free(body);
		}

		/* Fulfill the promise */
		Promise* promise = CoerceToPromise(ctx->promise);
		PromiseFulfill(interp, promise, obj);
		ListStateMachineNode* node = promise->FullfillReactions;
		while (node != NULL) {
			EnqueueTask(interp, node->Promise);
			node = node->Next;
		}

		c->is_closing = 1;
		free(ctx->host);
		free(ctx->uri);
		free(ctx->method);
		free(ctx->extraHdr);
		free(ctx->bodyStr);
		free(ctx);
		c->fn_data = NULL;
		return;
	}

	if (ev == MG_EV_ERROR) {
		Interpreter* interp = ctx->interp;
		String		 msg	= (String) ev_data;
		Value*		 err	= NewErrorFValue(interp,
											 "%s: %s",
											 RUNTIME_ERROR,
											 msg ? msg : "request failed");

		Promise* promise = CoerceToPromise(ctx->promise);
		PromiseReject(interp, promise, err);
		ListStateMachineNode* node = promise->RejectReactions;
		while (node != NULL) {
			EnqueueTask(interp, node->Promise);
			node = node->Next;
		}

		c->is_closing = 1;
		free(ctx->host);
		free(ctx->uri);
		free(ctx->method);
		free(ctx->extraHdr);
		free(ctx->bodyStr);
		free(ctx);
		c->fn_data = NULL;
		return;
	}
}

static Value* _Request(Interpreter* interp, int argc, Value** args) {
	if (argc < 1 || !ValueIsStr(args[0]))
		return NewErrorFValue(interp,
							  "%s: request() requires a URL string",
							  TYPE_ERROR);

	String url = ValueToString(args[0]);

	/* Parse optional options object */
	char* method   = strdup("GET");
	char* extraHdr = NULL;
	char* bodyStr  = NULL;

	if (argc >= 2 && ValueIsObject(args[1])) {
		HashMap* opts = CoerceToHashMap(args[1]);

		Value* mVal = (Value*) HashMapGet(opts, "method");
		if (mVal && ValueIsStr(mVal)) {
			free(method);
			method = ValueToString(mVal);
			/* Upper-case the method */
			for (char* p = method; *p; p++)
				*p = (char) toupper((unsigned char) *p);
		}

		Value* hVal = (Value*) HashMapGet(opts, "headers");
		if (hVal && ValueIsObject(hVal)) {
			HashMap* hdrMap = CoerceToHashMap(hVal);
			/* Build header string from key/value pairs */
			size_t cap	= 1024;
			extraHdr	= (char*) Allocate(cap);
			extraHdr[0] = '\0';
			size_t len	= 0;
			for (size_t i = 0; i < hdrMap->Size; i++) {
				HashNode* node = &hdrMap->Buckets[i];
				if (node->Key == NULL)
					continue;
				while (node) {
					if (node->Key) {
						String vStr	  = ValueToString((Value*) node->Val);
						size_t needed = strlen(node->Key) + strlen(vStr) + 5;
						while (len + needed >= cap) {
							cap		 *= 2;
							extraHdr  = realloc(extraHdr, cap);
						}
						len += (size_t) snprintf(extraHdr + len,
												 cap - len,
												 "%s: %s\r\n",
												 node->Key,
												 vStr);
						free(vStr);
					}
					node = node->Next;
				}
			}
		}

		Value* bVal = (Value*) HashMapGet(opts, "body");
		if (bVal && ValueIsStr(bVal)) {
			bodyStr = ValueToString(bVal);
		} else if (bVal && ValueIsObject(bVal)) {
			bodyStr = ValueToString(bVal);
			/* Auto-add Content-Type if not set */
			if (!extraHdr || !strstr(extraHdr, "Content-Type")) {
				const char* ct	  = "Content-Type: application/json\r\n";
				size_t		ctLen = strlen(ct);
				if (!extraHdr) {
					extraHdr = (char*) Allocate(ctLen + 1);
					memcpy(extraHdr, ct, ctLen + 1);
				} else {
					size_t oldLen = strlen(extraHdr);
					extraHdr	  = realloc(extraHdr, oldLen + ctLen + 1);
					memcpy(extraHdr + oldLen, ct, ctLen + 1);
				}
			}
		}
	}

	/* Extract host and URI from URL */
	struct mg_str hostStr = mg_url_host(url);
	char*		  host	  = (char*) Allocate(hostStr.len + 1);
	memcpy(host, hostStr.buf, hostStr.len);
	host[hostStr.len] = '\0';
	char* uri		  = strdup(mg_url_uri(url));

	/* Create a pending promise */
	Promise* promise	  = CreatePromise(PENDING, NULL);
	Value*	 promiseValue = NewPromiseValue(interp, promise);

	/* Allocate context for the event handler callback */
	FetchCtx* ctx = (FetchCtx*) Allocate(sizeof(FetchCtx));
	ctx->interp	  = interp;
	ctx->promise  = promiseValue;
	ctx->host	  = host;
	ctx->uri	  = uri;
	ctx->method	  = method;
	ctx->extraHdr = extraHdr;
	ctx->bodyStr  = bodyStr;
	ctx->sent	  = false;

	struct mg_connection* c =
		mg_http_connect(&interp->MgMgr, url, _FetchEvHandler, ctx);

	free(url);

	if (c == NULL) {
		PromiseReject(
			interp,
			promise,
			NewErrorFValue(interp, "%s: request(): failed to connect", RUNTIME_ERROR));
		free(ctx->host);
		free(ctx->uri);
		free(ctx->method);
		free(ctx->extraHdr);
		free(ctx->bodyStr);
		free(ctx);
	}

	return promiseValue;
}

/* -----------------------------------------------------------------------
 * Module entry point — mirrors LoadCoreSqlite pattern
 *
 * Request, Response and Server are built as proper class values.
 * Server stores _ReqClass / _ResClass as GC-visible static members so
 * CreateClassInstance() in _ServerInit picks them up and the GC keeps
 * them alive.
 * ----------------------------------------------------------------------- */
Value* LoadCoreMongoose(Interpreter* interpreter) {
	Value* reqClass	   = _BuildClass(interpreter, "Request", _ReqClassMethods);
	Value* resClass	   = _BuildClass(interpreter, "Response", _ResClassMethods);
	Value* serverClass = _BuildClass(interpreter, "Server", _ServerClassMethods);

	/* Root the sub-classes as static members of Server (same trick as
	 * Database._StmtClass in sqlite.c) so the GC always traces them. */
	ClassDefineMemberByString(CoerceToUserClass(serverClass),
							  PROP_REQ_CLASS,
							  reqClass,
							  true);
	ClassDefineMemberByString(CoerceToUserClass(serverClass),
							  PROP_RES_CLASS,
							  resClass,
							  true);

	Value*	 module = NewObjectValue(interpreter);
	HashMap* map	= CoerceToHashMap(module);
	HashMapSet(map, "Server", serverClass);
	HashMapSet(map, "Request", reqClass);
	HashMapSet(map, "Response", resClass);
	HashMapSet(map,
			   "request",
			   NewNativeFunctionValue(
				   interpreter,
				   CreateNativeFunctionMeta("request",
											VARARG,
											(NativeFunctionCallback) _Request)));
	return module;
}

#undef _MIME_TABLE_LEN
#undef PROP_APP_PTR
#undef PROP_MG_GC_ROOTS
#undef PROP_RES_CTX
#undef PROP_RES_STATUS
#undef PROP_RES_HDRSTR
#undef PROP_REQ_CLASS
#undef PROP_RES_CLASS
#undef MAX_MIDDLEWARE
#undef ROUTE_GROW
#undef DEFINE_ROUTE_METHOD
