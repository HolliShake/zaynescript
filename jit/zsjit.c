#include "./zsjit.h"

#include <stdio.h>

static int _rdint(uint8_t* bytecode, size_t start) {
	int offset	= 0;
	offset	   |= bytecode[start + 0] << 24;
	offset	   |= bytecode[start + 1] << 16;
	offset	   |= bytecode[start + 2] << 8;
	offset	   |= bytecode[start + 3] << 0;
	return offset;
}

static String _rdstr(uint8_t* bytecode, size_t start) {
	char   chr;
	String str = Allocate(1);
	size_t len = 0;
	while ((chr = bytecode[start++]) != '\0') {
		str[len++] = chr;
		str		   = Reallocate(str, sizeof(char) * (len + 1));
		str[len]   = '\0';
	}
	return str;
}

typedef struct str_builder {
	String buffer;
	size_t len;
	size_t capacity;
} StrBuilder;

static void StrBuilderInit(StrBuilder* sb) {
	sb->capacity  = 4096;
	sb->len		  = 0;
	sb->buffer	  = Allocate(sb->capacity);
	sb->buffer[0] = '\0';
}

static void StrBuilderFree(StrBuilder* sb) {
	free(sb->buffer);
	sb->buffer	 = NULL;
	sb->len		 = 0;
	sb->capacity = 0;
}

static String StrBuilderGet(const StrBuilder* sb) {
	return sb->buffer;
}

/* Ensure at least `need` extra bytes fit without a realloc. */
static void _sb_reserve(StrBuilder* sb, size_t need) {
	if (sb->len + need + 1 <= sb->capacity)
		return;
	size_t cap = sb->capacity * 2;
	while (sb->len + need + 1 > cap)
		cap *= 2;
	sb->buffer	 = Reallocate(sb->buffer, cap);
	sb->capacity = cap;
}

/* Append a literal string — no format interpretation, safe for strings
   that may contain '%' characters. */
static void StrAppend(StrBuilder* sb, String s) {
	size_t n = strlen(s);
	_sb_reserve(sb, n);
	memcpy(sb->buffer + sb->len, s, n + 1); /* +1 copies the null terminator */
	sb->len += n;
}

/* Append a printf-style formatted string. */
static void StrAppendf(StrBuilder* sb, String fmt, ...) {
	va_list args;

	va_start(args, fmt);
	int n = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (n <= 0)
		return;

	_sb_reserve(sb, (size_t) n);

	va_start(args, fmt);
	vsnprintf(sb->buffer + sb->len, sb->capacity - sb->len, fmt, args);
	va_end(args);

	sb->len += (size_t) n;
}

Value* _zsjit_popp(Interpreter* _i) {
	return _i->Stacks[--_i->StckC];
}

Value* _zsjit_peek(Interpreter* _i) {
	return _i->Stacks[_i->StckC - 1];
}

void _zsjit_push(Interpreter* _i, Value* v) {
	_i->Stacks[_i->StckC++] = v;
}

Value* _zsjit_getconst(Interpreter* _i, int off) {
	return _i->Constants[off];
}

Value* _zsjit_gettrue(Interpreter* _i) {
	return _i->True;
}

Value* _zsjit_getfalse(Interpreter* _i) {
	return _i->False;
}

Value* _zsjit_getnull(Interpreter* _i) {
	return _i->Null;
}

Value* _zsjit_getcellvalue(EnvCell* _c) {
	return _c->Value;
}

Value* _zsjit_getcap(UserFunction* _uf, int off) {
	return UserFunctionGetCapture(_uf, off);
}

void _zsjit_pushtry(Interpreter* _i, int jmp, size_t pausedAddress) {
	_i->ExceptionHandlerStacks[_i->ExceptionHandlerStackC++] =
		(ExceptionHandler){
			.JumpAddress	  = jmp,
			.JitPausedAddress = pausedAddress,
		};
}

void _zsjit_popptry(Interpreter* _i) {
	_i->ExceptionHandlerStacks[--_i->ExceptionHandlerStackC];
}

void _zsjit_seterror(Interpreter* _i, Value* err) {
	_i->Error = err;
}

Value* _zsjit_geterror(Interpreter* _i) {
	return _i->Error;
}

struct JumpTargets {
	int* Target;
	int	 Size;
};

String fncode =
	"#ifndef NULL\n#define NULL ((void*)0)\n#endif\n"
	"#ifndef _SIZE_T_DEFINED\n"
	"#define _SIZE_T_DEFINED\n"
	"typedef unsigned long size_t;\n"
	"#endif\n"
	"#ifndef __STDBOOL_H\n"
	"#define __STDBOOL_H\n"
	"#define __bool_true_false_are_defined 1\n"
	"#define bool _Bool\n"
	"#define true 1\n"
	"#define false 0\n"
	"#endif\n"
	/* opaque forward declarations — no struct layouts needed */
	"typedef struct interpreter_struct  Interpreter;\n"
	"typedef struct hashmap_struct      HashMap;\n"
	"typedef struct value_struct        Value;\n"
	"typedef struct userfn_struct       UserFunction;\n"
	"typedef struct environment_struct  Environment;\n"
	"typedef struct envcell_struct      EnvCell;\n"
	"typedef char* String;\n"
	/*Hashmap*/
	"extern void* HashMapGet(HashMap* hashmap, String key);\n"
	/* stack / constant helpers */
	"extern Value* _zsjit_popp(Interpreter* i);\n"
	"extern Value* _zsjit_peek(Interpreter* i);\n"
	"extern void   _zsjit_push(Interpreter* i, Value* v);\n"
	"extern Value* _zsjit_getconst(Interpreter* i, int off);\n"
	"extern Value* _zsjit_gettrue(Interpreter* i);\n"
	"extern Value* _zsjit_getfalse(Interpreter* i);\n"
	"extern Value* _zsjit_getnull(Interpreter* i);\n"
	"extern Value* _zsjit_getcellvalue(EnvCell* c);\n"
	"extern Value* _zsjit_getcap(UserFunction* _uf, int off);\n"
	"extern void _zsjit_pushtry(Interpreter* _i, int jmp, size_t "
	"pausedAddress);\n"
	"extern void _zsjit_popptry(Interpreter* _i);\n"
	"extern void _zsjit_seterror(Interpreter* _i, Value* err);\n"
	"extern Value* _zsjit_geterror(Interpreter* _i);\n"
	/* runtime function externs */
	"extern bool CoerceToBool(Value* value);\n"
	"extern HashMap* CoerceToHashMap(Value* value);\n"
	"extern UserFunction* CoerceToUserFunction(Value* value);\n"
	"extern Environment*  CoerceToEnvironment(Value* value);\n"
	"extern void   EnvironmentSetLocal(Environment* e, int off, Value* v);\n"
	"extern EnvCell* EnvironmentGetLocal(Environment* e, int off);\n"
	"extern Value* DoImportCore(Interpreter* interpreter, String moduleName);\n"
	"extern Value* DoImportLib(Interpreter* interpreter, String moduleName);\n"
	"extern Value* DoLoadFunction(Interpreter* interpreter, int offset, bool "
	"closure);\n"
	"extern Value* DoCall(Interpreter* interpreter, Value* fn, int argc, bool "
	"withThis);\n"
	"extern Value* DoMul(Interpreter* _i, Value* a, Value* b);\n"
	"extern Value* DoAdd(Interpreter* _i, Value* a, Value* b);\n"
	"extern Value* DoSub(Interpreter* _i, Value* a, Value* b);\n"
	"extern Value* DoLTE(Interpreter* interpreter, Value* lhs, Value* rhs);\n"
	"extern bool    ValueIsError(Value* v);\n"
	/* JIT entry point */
	"Value* __jit_fn(Interpreter* _i, Value* _re, Value* _ce, Value* _fn)\n"
	"{\n"
	"(void)_fn;\n"
	"UserFunction* _uf = CoerceToUserFunction(_fn);\n"
	"Value* res = NULL, *a = NULL, *b = NULL;\n"
	"bool hasLocalErrorHandler = false;\n"
	"int handlerc  =  0;\n"
	"%s\n"
	"}\n";

static struct JumpTargets Compute(UserFunction* _uf) {
	size_t ip  = 0;
	size_t len = _uf->CodeC;

	struct JumpTargets jt = { .Size = 0, .Target = Allocate(sizeof(int)) };

#define _jt_push(tgt)                                                          \
	do {                                                                       \
		int _t	 = (tgt);                                                      \
		int _dup = 0;                                                          \
		for (int _k = 0; _k < jt.Size; _k++)                                   \
			if (jt.Target[_k] == _t) {                                         \
				_dup = 1;                                                      \
				break;                                                         \
			}                                                                  \
		if (!_dup) {                                                           \
			jt.Target =                                                        \
				Reallocate(jt.Target, sizeof(int) * (size_t) (jt.Size + 1));   \
			jt.Target[jt.Size++] = _t;                                         \
		}                                                                      \
	} while (0)

	while (ip < len) {
		OpcodeEnum op = _uf->Codes[ip++];

		switch (op) {
			/* ── no operand ──────────────────────────────────────── */
			case OP_LOAD_NULL:
			case OP_ARRAY_EXTEND:
			case OP_ARRAY_PUSH:
			case OP_OBJECT_EXTEND:
			case OP_CLASS_EXTEND:
			case OP_CLASS_DEFINE_STATIC_MEMBER:
			case OP_CLASS_DEFINE_INSTANCE_MEMBER:
			case OP_SET_INDEX:
			case OP_GET_INDEX:
			case OP_NOT:
			case OP_POS:
			case OP_NEG:
			case OP_AWAIT:
			case OP_GET_AWAITED_VALUE:
			case OP_MUL:
			case OP_DIV:
			case OP_MOD:
			case OP_POSTINC:
			case OP_INC:
			case OP_ADD:
			case OP_POSTDEC:
			case OP_DEC:
			case OP_SUB:
			case OP_LSHFT:
			case OP_RSHFT:
			case OP_LT:
			case OP_LTE:
			case OP_GT:
			case OP_GTE:
			case OP_EQ:
			case OP_NE:
			case OP_AND:
			case OP_OR:
			case OP_XOR:
			case OP_DUPTOP:
			case OP_DUP2:
			case OP_POPTOP:
			case OP_ROT2:
			case OP_ROT3:
			case OP_ROT4:
			case OP_POP_TRY:
			case OP_RAISE:
			case OP_RETURN:
				break;

			/* ── 4-byte int operand, not a jump target ───────────── */
			case OP_LOAD_CONST:
			case OP_LOAD_BOOL:
			case OP_LOAD_CAPTURE:
			case OP_LOAD_NAME:
			case OP_LOAD_LOCAL:
			case OP_LOAD_FUNCTION:
			case OP_LOAD_FUNCTION_CLOSURE:
			case OP_CALL_CTOR:
			case OP_CALL:
			case OP_CALL_METHOD:
			case OP_STORE_CAPTURE:
			case OP_STORE_NAME:
			case OP_STORE_LOCAL:
			case OP_LOCK_VAR:
			case OP_ARRAY_MAKE:
			case OP_OBJECT_MAKE:
			case OP_POPN_TRY:
				ip += 4;
				break;

			/* ── 4-byte int operand that IS a jump target ─────────── */
			case OP_POP_JUMP_IF_FALSE:
			case OP_POP_JUMP_IF_TRUE:
			case OP_JUMP_IF_FALSE_OR_POP:
			case OP_JUMP_IF_TRUE_OR_POP:
			case OP_JUMP:
			case OP_ABSOLUTE_JUMP:
			case OP_SETUP_TRY:
				_jt_push(_rdint(_uf->Codes, ip));
				ip += 4;
				break;

			/* ── null-terminated string operand ──────────────────── */
			case OP_IMPORT_CORE:
			case OP_IMPORT_LIB:
			case OP_IMPORT_RELATIVE:
			case OP_LOAD_STRING:
			case OP_OBJECT_PLUCK_ATTRIBUTE:
			case OP_CLASS_MAKE:
				{
					String s  = _rdstr(_uf->Codes, ip);
					ip		 += strlen(s) + 1;
					free(s);
					break;
				}

			default:
				break;
		}
	}

#undef _jt_push
	return jt;
}

static String Codegen(Interpreter* interpreter, Value* fn) {
	UserFunction* uf	   = CoerceToUserFunction(fn);
	StateMachine* sm	   = NULL;
	uint8_t*	  bytecode = uf->Codes;
	String		  str	   = NULL;
	size_t		  ip	   = 0;
	size_t		  len	   = uf->CodeC;
	int			  off	   = 0;
	int			  arg	   = 0;
	OpcodeEnum	  op	   = 0;
	StrBuilder	  sb;
	StrBuilderInit(&sb);

	struct JumpTargets t = Compute(uf);
	/* sort targets ascending so we can consume them in IP order */
	for (int _i = 0; _i < t.Size - 1; _i++)
		for (int _j = _i + 1; _j < t.Size; _j++)
			if (t.Target[_j] < t.Target[_i]) {
				int _tmp	 = t.Target[_i];
				t.Target[_i] = t.Target[_j];
				t.Target[_j] = _tmp;
			}
	int ti = 0; /* cursor into t.Target */

	/* tracks which IP offsets have already had their C label emitted */
	int* EMITTED = Allocate(sizeof(int) * uf->CodeC);
	memset(EMITTED, 0, sizeof(int) * uf->CodeC);

	int* handler_targets	 = Allocate(sizeof(int));
	int	 handler_targets_top = 0;

#define forward(size) (ip += size)

#define pushtmphandler(handler)                                                \
	{                                                                          \
		handler_targets[handler_targets_top++] = handler;                      \
		handler_targets = Reallocate(handler_targets,                          \
									 sizeof(int) * (handler_targets_top + 1)); \
	}

#define popptmphandler() (handler_targets[--handler_targets_top])

#define peektmphandler() (handler_targets[handler_targets_top - 1])

#define handleError()                                                          \
	{                                                                          \
		if (handler_targets_top > 0) {                                         \
			int _catch_ip = peektmphandler();                                  \
			StrAppendf(&sb,                                                    \
					   "if(_zsjit_geterror(_i)!=NULL&&hasLocalErrorHandler){"  \
					   "_zsjit_seterror(_i, NULL);goto _L%d;}",                \
					   _catch_ip);                                             \
		}                                                                      \
	}

	while (ip < len) {
		while (ti < t.Size && t.Target[ti] == (int) ip) {
			int lv = t.Target[ti++];
			if (!EMITTED[lv]) {
				StrAppendf(&sb, "_L%d:;", lv);
				EMITTED[lv] = 1;
			}
		}

		op = uf->Codes[ip++];

		switch (op) {
			case OP_IMPORT_CORE:
				{
					str = _rdstr(bytecode, ip);
					StrAppendf(&sb,
							   "{res=DoImportCore(_i, "
							   "\"%s\");}if(ValueIsError(res)){return "
							   "res;}else{_zsjit_push(_i,res);}",
							   str);
					forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_IMPORT_LIB:
				{
					str = _rdstr(bytecode, ip);
					StrAppendf(&sb,
							   "{res=DoImportLib(_i, "
							   "\"%s\");}if(ValueIsError(res)){return "
							   "res;}else{_zsjit_push(_i,res);}",
							   str);
					forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_OBJECT_PLUCK_ATTRIBUTE:
				{
					str = _rdstr(bytecode, ip);
					StrAppendf(
						&sb,
						"{a=_zsjit_peek(_i);res=HashMapGet("
						"CoerceToHashMap(a), "
						"\"%s\");}_zsjit_push(_i, res==NULL?_zsjit_getnull(_"
						"i):res);",
						str);
					forward(strlen(str) + 1);
					free(str);
					break;
				}
			case OP_LOAD_CONST:
				{
					off = _rdint(bytecode, ip);
					StrAppendf(&sb,
							   "{_zsjit_push(_i,_zsjit_getconst(_i,%d));}",
							   off);
					forward(4);
					break;
				}
			case OP_LOAD_BOOL:
				{
					off = _rdint(bytecode, ip);
					StrAppendf(&sb,
							   "{_zsjit_push(_i,%d?_zsjit_gettrue(_i):_zsjit_"
							   "getfalse(_i));}",
							   off);
					forward(4);
					break;
				}
			case OP_LOAD_NULL:
				{
					StrAppend(&sb, "{_zsjit_push(_i,_zsjit_getnull(_i));}");
					break;
				}
			case OP_LOAD_CAPTURE:
				{
					off = _rdint(bytecode, ip);
					StrAppendf(&sb,
							   "{_zsjit_push(_i,_zsjit_getcap(_uf,%d));}",
							   off);
					forward(4);
					break;
				}
			case OP_LOAD_NAME:
				{
					off = _rdint(bytecode, ip);
					StrAppendf(&sb,
							   "{_zsjit_push(_i,_zsjit_getcellvalue("
							   "EnvironmentGetLocal("
							   "CoerceToEnvironment(_re),%d)));}",
							   off);
					forward(4);
					break;
				}
			case OP_LOAD_LOCAL:
				{
					off = _rdint(bytecode, ip);
					StrAppendf(&sb,
							   "{_zsjit_push(_i,_zsjit_getcellvalue("
							   "EnvironmentGetLocal("
							   "CoerceToEnvironment(_ce),%d)));}",
							   off);
					forward(4);
					break;
				}
			case OP_LOAD_FUNCTION_CLOSURE:
			case OP_LOAD_FUNCTION:
				{
					off = _rdint(bytecode, ip);
					StrAppendf(
						&sb,
						"{res=DoLoadFunction(_i,%d,%d);}_zsjit_push(_i,res);",
						off,
						op == OP_LOAD_FUNCTION_CLOSURE);
					forward(4);
					break;
				}
			case OP_CALL:
				{
					arg = _rdint(bytecode, ip);
					StrAppendf(
						&sb,
						"{a=_zsjit_popp(_i);res=DoCall(_i,a,%d,0);}"
						"if(ValueIsError(res)){_zsjit_seterror(_i,res);}",
						arg);
					handleError();
					forward(4);
					break;
				}
			case OP_MUL:
				{
					StrAppend(&sb,
							  "{b=_zsjit_popp(_i);a=_zsjit_popp(_i);res=DoMul(_"
							  "i,a,b);_zsjit_push(_i,res);}if(ValueIsError(res)"
							  "){_zsjit_seterror(_i,res);}");
					handleError();
					break;
				}
			case OP_ADD:
				{
					StrAppend(&sb,
							  "{b=_zsjit_popp(_i);a=_zsjit_popp(_i);res=DoAdd(_"
							  "i,a,b);_zsjit_push(_i,res);}if(ValueIsError(res)"
							  "){_zsjit_seterror(_i,res);}");

					handleError();
					break;
				}
			case OP_SUB:
				{
					StrAppend(&sb,
							  "{b=_zsjit_popp(_i);a=_zsjit_popp(_i);res=DoSub(_"
							  "i,a,b);_zsjit_push(_i,res);}if(ValueIsError(res)"
							  "){_zsjit_seterror(_i,res);}");
					handleError();
					break;
				}
			case OP_LTE:
				{
					StrAppend(&sb,
							  "{b=_zsjit_popp(_i);a=_zsjit_popp(_i);res=DoLTE(_"
							  "i,a,b);_zsjit_push(_i,res);}if(ValueIsError(res)"
							  "){_zsjit_seterror(_i,res);}");
					handleError();
					break;
				}
			case OP_RETURN:
				{
					StrAppend(&sb, "return NULL;");
					break;
				}
			case OP_STORE_NAME:
				{
					off = _rdint(bytecode, ip);
					StrAppendf(&sb,
							   "{EnvironmentSetLocal(CoerceToEnvironment(_re),"
							   "%d,_zsjit_popp(_i));}",
							   off);
					forward(4);
					break;
				}
			case OP_STORE_LOCAL:
				{
					off = _rdint(bytecode, ip);
					StrAppendf(&sb,
							   "{EnvironmentSetLocal(CoerceToEnvironment(_ce),"
							   "%d,_zsjit_popp(_i));}",
							   off);
					forward(4);
					break;
				}
			case OP_POPTOP:
				{
					StrAppend(&sb, "_zsjit_popp(_i);");
					break;
				}
			case OP_POP_JUMP_IF_FALSE:
				{
					off = _rdint(bytecode, ip);
					StrAppendf(&sb,
							   "{res=_zsjit_popp(_i);if(CoerceToBool(res)==0){"
							   "goto _L%d;}}",
							   off);
					forward(4);
					break;
				}
			case OP_JUMP:
				{
					off = _rdint(bytecode, ip);
					StrAppendf(&sb, "goto _L%d;", off);
					forward(4);
					break;
				}
			case OP_SETUP_TRY:
				{
					off = _rdint(bytecode, ip);
					pushtmphandler(off);
					StrAppendf(&sb,
							   "_TRY_%d_to_%d:;{hasLocalErrorHandler=true;++"
							   "handlerc;_zsjit_"
							   "pushtry(_i,%d,%zu);}",
							   ip,
							   off,
							   off,
							   ip);
					forward(4);
					break;
				}
			case OP_POP_TRY:
				{
					popptmphandler();
					StrAppend(&sb,
							  "{hasLocalErrorHandler=(--handlerc)>0;_zsjit_"
							  "popptry(_i);}");
					break;
				}
			default:
				{
					printf("unknown code %d\n", op);
					return NULL;
				}
		}
	}
	String code = FormatString(fncode, sb.buffer);
	StrBuilderFree(&sb);
	free(handler_targets);
	return code;
}

/* ── TCC state registry (for ZJitFree) ─────────────────────────────────── */
static TCCState** _tcc_states	  = NULL;
static size_t	  _tcc_states_len = 0;
static size_t	  _tcc_states_cap = 0;

static void _tcc_register(TCCState* s) {
	if (_tcc_states_len == _tcc_states_cap) {
		_tcc_states_cap = _tcc_states_cap ? _tcc_states_cap * 2 : 8;
		_tcc_states =
			Reallocate(_tcc_states, sizeof(TCCState*) * _tcc_states_cap);
	}
	_tcc_states[_tcc_states_len++] = s;
}

ZJittedFn* ZJitCompile(Interpreter* interpreter, Value* fn) {
	UserFunction* uf = CoerceToUserFunction(fn);

	if (uf->JitFn != NULL)
		return (ZJittedFn*) uf->JitFn;

	String code = Codegen(interpreter, fn);
	if (code == NULL)
		return NULL;

	// printf(">>%s\n", code);

	TCCState* s = tcc_new();
	if (s == NULL) {
		free(code);
		return NULL;
	}

	tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

	if (tcc_compile_string(s, code) == -1) {
		free(code);
		tcc_delete(s);
		return NULL;
	}
	free(code);

	tcc_add_symbol(s, "_zsjit_popp", (void*) _zsjit_popp);
	tcc_add_symbol(s, "_zsjit_peek", (void*) _zsjit_peek);
	tcc_add_symbol(s, "_zsjit_push", (void*) _zsjit_push);
	tcc_add_symbol(s, "_zsjit_getconst", (void*) _zsjit_getconst);
	tcc_add_symbol(s, "_zsjit_gettrue", (void*) _zsjit_gettrue);
	tcc_add_symbol(s, "_zsjit_getfalse", (void*) _zsjit_getfalse);
	tcc_add_symbol(s, "_zsjit_getnull", (void*) _zsjit_getnull);
	tcc_add_symbol(s, "_zsjit_getcellvalue", (void*) _zsjit_getcellvalue);
	tcc_add_symbol(s, "_zsjit_getcap", (void*) _zsjit_getcap);
	tcc_add_symbol(s, "_zsjit_pushtry", (void*) _zsjit_pushtry);
	tcc_add_symbol(s, "_zsjit_popptry", (void*) _zsjit_popptry);
	tcc_add_symbol(s, "_zsjit_seterror", (void*) _zsjit_seterror);
	tcc_add_symbol(s, "_zsjit_geterror", (void*) _zsjit_geterror);
	tcc_add_symbol(s, "HashMapGet", (void*) HashMapGet);
	tcc_add_symbol(s, "CoerceToBool", (void*) CoerceToBool);
	tcc_add_symbol(s, "CoerceToHashMap", (void*) CoerceToHashMap);
	tcc_add_symbol(s, "CoerceToUserFunction", (void*) CoerceToUserFunction);
	tcc_add_symbol(s, "CoerceToEnvironment", (void*) CoerceToEnvironment);
	tcc_add_symbol(s, "EnvironmentSetLocal", (void*) EnvironmentSetLocal);
	tcc_add_symbol(s, "EnvironmentGetLocal", (void*) EnvironmentGetLocal);
	tcc_add_symbol(s, "DoImportCore", (void*) DoImportCore);
	tcc_add_symbol(s, "DoImportLib", (void*) DoImportLib);
	tcc_add_symbol(s, "DoLoadFunction", (void*) DoLoadFunction);
	tcc_add_symbol(s, "DoCall", (void*) DoCall);
	tcc_add_symbol(s, "DoMul", (void*) DoMul);
	tcc_add_symbol(s, "DoAdd", (void*) DoAdd);
	tcc_add_symbol(s, "DoSub", (void*) DoSub);
	tcc_add_symbol(s, "DoLTE", (void*) DoLTE);
	tcc_add_symbol(s, "ValueIsError", (void*) ValueIsError);

	if (tcc_relocate(s) == -1) {
		tcc_delete(s);
		return NULL;
	}

	void* sym = tcc_get_symbol(s, "__jit_fn");
	if (sym == NULL) {
		tcc_delete(s);
		return NULL;
	}

	_tcc_register(s);
	uf->JitFn = sym;
	return (ZJittedFn*) sym;
}

void ZJitFree(void) {
	for (size_t i = 0; i < _tcc_states_len; i++)
		tcc_delete(_tcc_states[i]);
	free(_tcc_states);
	_tcc_states		= NULL;
	_tcc_states_len = 0;
	_tcc_states_cap = 0;
}
