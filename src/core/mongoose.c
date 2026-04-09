#include "./mongoose.h"

/* -----------------------------------------------------------------------
 * Internal binding property keys
 * ----------------------------------------------------------------------- */

#define PROP_APP_PTR	"__ptr"
#define PROP_RES_CTX	"__ctx"
#define PROP_RES_STATUS "__status"
#define PROP_RES_HDRSTR "__hdrstr"
#define PROP_REQ_CLASS	"__ReqClass"
#define PROP_RES_CLASS	"__ResClass"

/* -----------------------------------------------------------------------
 * Forward declarations
 * --------------------------------------------------------------------- */
extern void	  Push(Interpreter* interpreter, Value* value);
extern Value* Popp(Interpreter* interpreter);
extern Value* DoCall(Interpreter* interp, Value* fn, int argc, bool withThis);

/* -----------------------------------------------------------------------
 * Status text helper
 * ----------------------------------------------------------------------- */

static const char* _StatusText(int code) {
	switch (code) {
		case 200:
			return "ok";
		case 201:
			return "created";
		case 204:
			return "no content";
		case 301:
			return "moved permanently";
		case 302:
			return "found";
		case 304:
			return "not modified";
		case 400:
			return "bad request";
		case 401:
			return "unauthorized";
		case 403:
			return "forbidden";
		case 404:
			return "not found";
		case 405:
			return "method not allowed";
		case 409:
			return "conflict";
		case 422:
			return "unprocessable entity";
		case 429:
			return "too many requests";
		case 500:
			return "internal server error";
		case 502:
			return "bad gateway";
		case 503:
			return "service unavailable";
		default:
			return "unknown";
	}
}

/* Escape a C string for safe embedding inside a JSON string literal. */
static String _JsonEscape(const char* src) {
	size_t len = 0;
	for (const char* p = src; *p; p++) {
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
	char* out = (char*) Allocate(len + 1);
	char* d	  = out;
	for (const char* p = src; *p; p++) {
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
	char*  method;	/* "GET", "POST", … or NULL = wildcard */
	char*  path;	/* mg_match() pattern                  */
	Value* handler; /* callable Value*                    */
} Route;

typedef struct {
	Route*		  routes;
	size_t		  count;
	size_t		  capacity;
	Value*		  middleware[MAX_MIDDLEWARE];
	size_t		  mw_count;
	Interpreter*  interp;
	struct mg_mgr mgr;
	bool		  running;
	Value*		  reqClass; /* GC-rooted via Server._ReqClass static member */
	Value*		  resClass; /* GC-rooted via Server._ResClass static member */
} AppState;

typedef struct {
	struct mg_connection*	conn;
	struct mg_http_message* msg;
	bool					responded;
} ReqResCtx;

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
	for (size_t i = 0; i < app->count; i++) {
		free(app->routes[i].method); /* strdup'd, or NULL for wildcard */
		free(app->routes[i].path);
	}
	free(app->routes);
	free(app);
}

static void _AppAddRoute(AppState*	 app,
						 const char* method,
						 const char* path,
						 Value*		 handler) {
	if (app->count >= app->capacity) {
		app->capacity += ROUTE_GROW;
		app->routes	   = realloc(app->routes, sizeof(Route) * app->capacity);
	}
	app->routes[app->count].method	= method ? strdup(method) : NULL;
	app->routes[app->count].path	= strdup(path);
	app->routes[app->count].handler = handler;
	app->count++;
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
	return NewErrorValue(interp, "Response cannot be constructed directly");
}

/* res.send(body) */
static Value* _ResSend(Interpreter* interp, int argc, Value** args) {
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	ReqResCtx*	   ctx = _GetCtx(cls);
	if (!ctx)
		return NewErrorValue(interp, "res.send(): invalid context");
	if (ctx->responded)
		return interp->Null;

	Value* sv	  = (Value*) HashMapGet(cls->Members, PROP_RES_STATUS);
	int	   status = (sv && ValueIsInt(sv)) ? sv->Value.I32 : 200;
	Value* hv	  = (Value*) HashMapGet(cls->Members, PROP_RES_HDRSTR);
	String extra  = (hv && ValueIsStr(hv)) ? ValueToString(hv) : strdup("");
	String body	  = (argc >= 2) ? ValueToString(args[1]) : strdup("");

	String		escaped = _JsonEscape(body);
	const char* stxt	= _StatusText(status);
	size_t		wrapLen = strlen(escaped) + strlen(stxt) + 128;
	String		wrapped = (String) Allocate(wrapLen);
	snprintf(wrapped,
			 wrapLen,
			 "{\"status\":%d,\"statusText\":\"%s\",\"data\":\"%s\"}",
			 status,
			 stxt,
			 escaped);

	size_t hlen	   = strlen(extra) + 64;
	String headers = (String) Allocate(hlen);
	snprintf(headers, hlen, "Content-Type: application/json\r\n%s", extra);

	mg_http_reply(ctx->conn, status, headers, "%s", wrapped);
	ctx->responded = true;
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
		return NewErrorValue(interp, "res.json() requires a body argument");

	ClassInstance* cls = CoerceToClassInstance(args[0]);
	ReqResCtx*	   ctx = _GetCtx(cls);
	if (!ctx)
		return NewErrorValue(interp, "res.json(): invalid context");
	if (ctx->responded)
		return interp->Null;

	Value* sv	  = (Value*) HashMapGet(cls->Members, PROP_RES_STATUS);
	int	   status = (sv && ValueIsInt(sv)) ? sv->Value.I32 : 200;
	Value* hv	  = (Value*) HashMapGet(cls->Members, PROP_RES_HDRSTR);
	String extra  = (hv && ValueIsStr(hv)) ? ValueToString(hv) : strdup("");

	size_t hlen	   = strlen(extra) + 64;
	String headers = (String) Allocate(hlen);
	snprintf(headers, hlen, "Content-Type: application/json\r\n%s", extra);
	free(extra);

	String		body	= ValueToString(args[1]);
	const char* stxt	= _StatusText(status);
	size_t		wrapLen = strlen(body) + strlen(stxt) + 128;
	String		wrapped = (String) Allocate(wrapLen);
	snprintf(wrapped,
			 wrapLen,
			 "{\"status\":%d,\"statusText\":\"%s\",\"data\":%s}",
			 status,
			 stxt,
			 body);

	mg_http_reply(ctx->conn, status, headers, "%s", wrapped);
	ctx->responded = true;
	free(body);
	free(wrapped);
	free(headers);
	return interp->Null;
}

/* res.status(code) → this  (chainable) */
static Value* _ResStatus(Interpreter* interp, int argc, Value** args) {
	if (argc < 2 || !ValueIsAnyNum(args[1]))
		return NewErrorValue(interp,
							 "res.status() requires a numeric status code");
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	HashMapSet(cls->Members,
			   PROP_RES_STATUS,
			   NewIntValue(interp, (int) CoerceToNum(args[1])));
	return args[0];
}

/* res.redirect(url) */
static Value* _ResRedirect(Interpreter* interp, int argc, Value** args) {
	if (argc < 2 || !ValueIsStr(args[1]))
		return NewErrorValue(interp,
							 "res.redirect() requires a URL string argument");
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	ReqResCtx*	   ctx = _GetCtx(cls);
	if (!ctx)
		return NewErrorValue(interp, "res.redirect(): invalid context");
	if (ctx->responded)
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
	mg_http_reply(ctx->conn, 302, header, "%s", wrapped);
	ctx->responded = true;
	free(location);
	free(escaped);
	return interp->Null;
}

/* res.setHeader(name, value) → this */
static Value* _ResSetHeader(Interpreter* interp, int argc, Value** args) {
	if (argc < 3 || !ValueIsStr(args[1]) || !ValueIsStr(args[2]))
		return NewErrorValue(interp,
							 "res.setHeader() requires (name, value) strings");
	ClassInstance* cls	= CoerceToClassInstance(args[0]);
	String		   name = ValueToString(args[1]);
	String		   val	= ValueToString(args[2]);
	Value*		   curV = (Value*) HashMapGet(cls->Members, PROP_RES_HDRSTR);
	String cur = (curV && ValueIsStr(curV)) ? ValueToString(curV) : strdup("");
	char   extra[512];
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
	return NewErrorValue(interp, "Request cannot be constructed directly");
}

static ModuleFunction _ReqClassMethods[] = {
	{ .Name		 = CONSTRUCTOR_NAME,
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _ReqInit,
	  .Value	 = NULL },
	{ .Name = NULL }
};

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
			char*  kbuf	   = (char*) Allocate(kbuf_sz);
			if (key.len >= 2 && key.buf[0] == '"') {
				mg_json_unescape(mg_str_n(key.buf + 1, key.len - 2),
								 kbuf,
								 kbuf_sz);
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
		char*  buf = (char*) Allocate(bsz);
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
static bool _HeaderContains(struct mg_http_message* hm,
							const char* hdr,
							const char* needle) {
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
	return _HeaderContains(hm, "Content-Type",
						   "application/x-www-form-urlencoded");
}

static bool _IsMultipartContentType(struct mg_http_message* hm) {
	return _HeaderContains(hm, "Content-Type", "multipart/form-data");
}

/* Parse an application/x-www-form-urlencoded body into an object Value */
static Value* _FormToObject(Interpreter* interp, struct mg_str body) {
	Value*	 obj = NewObjectValue(interp);
	HashMap* map = CoerceToHashMap(obj);

	const char* p   = body.buf;
	const char* end = body.buf + body.len;

	while (p < end) {
		/* find end of this key=value pair */
		const char* amp = memchr(p, '&', (size_t)(end - p));
		if (!amp)
			amp = end;

		/* find '=' separator */
		const char* eq = memchr(p, '=', (size_t)(amp - p));

		char keyBuf[512], valBuf[2048];

		if (eq) {
			mg_url_decode(p, (size_t)(eq - p), keyBuf, sizeof(keyBuf), 1);
			mg_url_decode(eq + 1, (size_t)(amp - eq - 1), valBuf,
						  sizeof(valBuf), 1);
		} else {
			mg_url_decode(p, (size_t)(amp - p), keyBuf, sizeof(keyBuf), 1);
			valBuf[0] = '\0';
		}

		HashMapSet(map, keyBuf, NewStrValue(interp, valBuf));
		p = amp < end ? amp + 1 : end;
	}
	return obj;
}

/* Parse a multipart/form-data body into an object Value */
static Value* _MultipartToObject(Interpreter*		   interp,
								 struct mg_http_message* hm) {
	Value*			  obj = NewObjectValue(interp);
	HashMap*		  map = CoerceToHashMap(obj);
	struct mg_http_part part;
	size_t			  ofs = 0;

	while ((ofs = mg_http_next_multipart(hm->body, ofs, &part)) > 0) {
		if (part.name.len == 0)
			continue;
		char key[512];
		size_t klen = part.name.len < sizeof(key) - 1
						  ? part.name.len
						  : sizeof(key) - 1;
		memcpy(key, part.name.buf, klen);
		key[klen] = '\0';

		char* val = (char*) Allocate(part.body.len + 1);
		memcpy(val, part.body.buf, part.body.len);
		val[part.body.len] = '\0';

		HashMapSet(map, key, NewStrValue(interp, val));
		free(val);
	}
	return obj;
}

/* -----------------------------------------------------------------------
 * Build req / res class instances per-request
 * ----------------------------------------------------------------------- */

static Value*
_BuildResInstance(Interpreter* interp, Value* resClass, ReqResCtx* ctx) {
	ClassInstance* inst = CreateClassInstance(resClass);
	HashMapSet(inst->Members, PROP_RES_CTX, NewOpquePtrValue(interp, ctx));
	HashMapSet(inst->Members, PROP_RES_STATUS, NewIntValue(interp, 200));
	HashMapSet(inst->Members, PROP_RES_HDRSTR, NewStrValue(interp, ""));
	return NewClassInstanceValue(interp, inst);
}

static Value* _BuildReqInstance(Interpreter*			interp,
								Value*					reqClass,
								struct mg_http_message* hm) {
	ClassInstance* inst = CreateClassInstance(reqClass);

	char method[16];
	snprintf(method,
			 sizeof(method),
			 "%.*s",
			 (int) hm->method.len,
			 hm->method.buf);
	HashMapSet(inst->Members, "method", NewStrValue(interp, method));

	char path[1024];
	snprintf(path, sizeof(path), "%.*s", (int) hm->uri.len, hm->uri.buf);
	HashMapSet(inst->Members, "url", NewStrValue(interp, path));
	HashMapSet(inst->Members, "path", NewStrValue(interp, path));

	char query[2048] = "";
	if (hm->query.len > 0)
		snprintf(query,
				 sizeof(query),
				 "%.*s",
				 (int) hm->query.len,
				 hm->query.buf);
	HashMapSet(inst->Members, "query", NewStrValue(interp, query));

	/* body: parse as JSON if application/json, as object if
	 * application/x-www-form-urlencoded or multipart/form-data,
	 * otherwise plain string */
	Value* bodyVal;
	if (hm->body.len > 0 && _IsJsonContentType(hm)) {
		bodyVal = _JsonToValue(interp, mg_str_n(hm->body.buf, hm->body.len));
	} else if (hm->body.len > 0 && _IsFormContentType(hm)) {
		bodyVal = _FormToObject(interp,
								mg_str_n(hm->body.buf, hm->body.len));
	} else if (hm->body.len > 0 && _IsMultipartContentType(hm)) {
		bodyVal = _MultipartToObject(interp, hm);
	} else if (hm->body.len > 0) {
		char* body = (char*) Allocate(hm->body.len + 1);
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
		for (char* p = hname; *p; p++)
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
	Interpreter*			interp = app->interp;

	ReqResCtx ctx = { .conn = c, .msg = hm, .responded = false };

	Value* reqVal = _BuildReqInstance(interp, app->reqClass, hm);
	Value* resVal = _BuildResInstance(interp, app->resClass, &ctx);

	/* Middleware */
	for (size_t i = 0; i < app->mw_count && !ctx.responded; i++) {
		Value* mw = app->middleware[i];
		if (!ValueIsCallable(mw))
			continue;
		Push(interp, resVal);
		Push(interp, reqVal);
		DoCall(interp, mw, 2, false);
		Popp(interp);
	}

	if (ctx.responded)
		return;

	/* Route matching */
	char uri[1024];
	snprintf(uri, sizeof(uri), "%.*s", (int) hm->uri.len, hm->uri.buf);

	char meth[16];
	snprintf(meth, sizeof(meth), "%.*s", (int) hm->method.len, hm->method.buf);

	bool matched = false;
	for (size_t i = 0; i < app->count && !matched; i++) {
		Route* r = &app->routes[i];

		if (r->method && strcasecmp(r->method, meth) != 0)
			continue;

		struct mg_str caps[4];
		memset(caps, 0, sizeof(caps));
		if (!mg_match(mg_str_n(uri, strlen(uri)), mg_str(r->path), caps))
			continue;

		matched = true;

		/* Populate req.params with wildcard captures */
		ClassInstance* reqInst	 = CoerceToClassInstance(reqVal);
		Value*		   paramsObj = NewObjectValue(interp);
		HashMap*	   paramsMap = CoerceToHashMap(paramsObj);
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
		HashMapSet(reqInst->Members, "params", paramsObj);

		Push(interp, resVal);
		Push(interp, reqVal);
		DoCall(interp, r->handler, 2, false);
		Value* result = Popp(interp);

		if ((ValueIsPromise(result)
			 && CoerceToStateMachine(result)->State == REJECTED)) {
			StateMachine* sm	  = CoerceToStateMachine(result);
			String		  errMsg  = ValueToString(sm->Value);
			String		  escaped = _JsonEscape(errMsg);
			size_t		  wlen	  = strlen(escaped) + 128;
			String		  wrapped = (String) Allocate(wlen);
			snprintf(wrapped,
					 wlen,
					 "{\"status\":500,\"statusText\":\"internal server error\","
					 "\"data\":\"%s\"}",
					 escaped);
			mg_http_reply(c,
						  500,
						  "Content-Type: application/json\r\n",
						  "%s",
						  wrapped);
			ctx.responded = true;
			free(errMsg);
			free(escaped);
			free(wrapped);
		}
	}

	if (!matched && !ctx.responded)
		mg_http_reply(
			c,
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
		return NewErrorValue(interp,
							 "internal: Request/Response class not found");

	AppState* app = (AppState*) Callocate(1, sizeof(AppState));
	app->interp	  = interp;
	app->running  = false;
	app->reqClass = reqClass;
	app->resClass = resClass;

	HashMapSet(cls->Members, PROP_APP_PTR, NewOpquePtrValue(interp, app));
	return interp->Null;
}

#define DEFINE_ROUTE_METHOD(ZsName, HttpMethod)                                \
	static Value* _App##ZsName(Interpreter* interp, int argc, Value** args) {  \
		if (argc != 3 || !ValueIsStr(args[1]) || !ValueIsCallable(args[2]))    \
			return NewErrorValue(interp,                                       \
								 #ZsName "() requires (path, handler)");       \
		ClassInstance* cls = CoerceToClassInstance(args[0]);                   \
		AppState*	   app = _GetApp(cls);                                     \
		if (!app)                                                              \
			return NewErrorValue(interp,                                       \
								 #ZsName "(): server not initialised");        \
		String path = ValueToString(args[1]);                                  \
		_AppAddRoute(app, HttpMethod, path, args[2]);                          \
		free(path);                                                            \
		return args[0];                                                        \
	}

DEFINE_ROUTE_METHOD(Get, "GET")
DEFINE_ROUTE_METHOD(Post, "POST")
DEFINE_ROUTE_METHOD(Put, "PUT")
DEFINE_ROUTE_METHOD(Delete, "DELETE")
DEFINE_ROUTE_METHOD(Patch, "PATCH")
DEFINE_ROUTE_METHOD(All, NULL)

static Value* _AppUse(Interpreter* interp, int argc, Value** args) {
	if (argc < 2 || !ValueIsCallable(args[1]))
		return NewErrorValue(interp, "use() requires a callback function");
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	AppState*	   app = _GetApp(cls);
	if (!app)
		return NewErrorValue(interp, "use(): server not initialised");
	if (app->mw_count >= MAX_MIDDLEWARE)
		return NewErrorValue(interp, "use(): maximum middleware count reached");
	app->middleware[app->mw_count++] = args[1];
	return args[0];
}

static Value* _AppListen(Interpreter* interp, int argc, Value** args) {
	if (argc < 2 || !ValueIsAnyNum(args[1]))
		return NewErrorValue(interp,
							 "listen() requires a port number argument");
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	AppState*	   app = _GetApp(cls);
	if (!app)
		return NewErrorValue(interp, "listen(): server not initialised");

	int	   port = (int) CoerceToNum(args[1]);
	Value* cb	= (argc >= 3 && ValueIsCallable(args[2])) ? args[2] : NULL;

	char url[64];
	snprintf(url, sizeof(url), "http://0.0.0.0:%d", port);

	mg_mgr_init(&app->mgr);
	struct mg_connection* lc = mg_http_listen(&app->mgr, url, _EvHandler, app);
	if (lc == NULL) {
		mg_mgr_free(&app->mgr);
		_AppStateFree(app);
		HashMapSet(cls->Members, PROP_APP_PTR, NewOpquePtrValue(interp, NULL));
		return NewErrorFValue(interp,
							  "listen(): failed to bind on port %d",
							  port);
	}

	app->running = true;

	if (cb) {
		String msg	  = FormatString("Server listening on port %d", port);
		Value* msgVal = NewStrValue(interp, msg);
		free(msg);
		Push(interp, msgVal);
		DoCall(interp, cb, 1, false);
		Popp(interp);
	}

	while (app->running)
		mg_mgr_poll(&app->mgr, 100);

	mg_mgr_free(&app->mgr);
	_AppStateFree(app);
	HashMapSet(cls->Members, PROP_APP_PTR, NewOpquePtrValue(interp, NULL));
	return interp->Null;
}

static Value* _AppClose(Interpreter* interp, int argc, Value** args) {
	(void) argc;
	ClassInstance* cls = CoerceToClassInstance(args[0]);
	AppState*	   app = _GetApp(cls);
	if (app)
		app->running = false;
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
_BuildClass(Interpreter* interp, const char* name, ModuleFunction methods[]) {
	Value* classVal =
		NewClassValue(interp, CreateUserClass((String) name, NULL));
	Class* cls = CoerceToUserClass(classVal);

	for (int i = 0; methods[i].Name != NULL; i++) {
		ModuleFunction* func = &methods[i];
		if (func->CFunction) {
			ClassDefineMemberByString(
				cls,
				func->Name,
				NewNativeFunctionValue(
					interp,
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
 *             It owns the mg_mgr, the route table, and the global
 *             middleware list.
 *
 * Route     – one registered route (method + path pattern + handler).
 * RouteList – a simple growable array of routes.
 * ----------------------------------------------------------------------- */

#define MAX_MIDDLEWARE 32

/* -----------------------------------------------------------------------
 * Module entry point — mirrors LoadCoreSqlite pattern
 *
 * Request, Response and Server are built as proper class values.
 * Server stores _ReqClass / _ResClass as GC-visible static members so
 * CreateClassInstance() in _ServerInit picks them up and the GC keeps
 * them alive.
 * ----------------------------------------------------------------------- */
Value* LoadCoreMongoose(Interpreter* interpreter) {
	Value* reqClass = _BuildClass(interpreter, "Request", _ReqClassMethods);
	Value* resClass = _BuildClass(interpreter, "Response", _ResClassMethods);
	Value* serverClass =
		_BuildClass(interpreter, "Server", _ServerClassMethods);

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
	return module;
}
