/**
 * @file zsjit.h
 * @brief ZayneScript TCC-based JIT compiler interface.
 *
 * Translates UserFunction bytecode to C source on the fly, compiles it with
 * libtcc in-memory, and returns a native function pointer.  Functions that
 * contain unsupported opcodes (try/catch, async/await, class/array/object
 * literals) transparently fall back to the bytecode interpreter.
 */

#include "../src/environment.h"
#include "../src/function.h"
#include "../src/global.h"
#include "../src/hashmap.h"
#include "../src/operation.h"

#include <libtcc.h>


#ifndef JIT_ZSJIT_H
#	define JIT_ZSJIT_H

/**
 * @brief Type signature for a TCC-compiled JIT function.
 *
 * @param _i   The active Interpreter instance.
 * @param _re  The module-level root environment Value*.
 * @param _cr  The current call-frame environment Value*.
 * @param _fn  The UserFunction whose bytecode is being executed.
 *
 * On success, the function pushes its return value onto the interpreter's main
 * value stack (mirroring OP_RETURN semantics) and returns NULL. On an
 * unrecoverable runtime error, it returns the error Value*, so the caller
 * (such as Run) can invoke the interpreter's normal error-handling path.
 */
typedef void* (*ZJittedFn)(Interpreter* _i, Value* _re, Value* _cr, Value* _fn);

/**
 * @brief JIT-compile a function using TCC as the backend.
 *
 * On the first call for a given function, this function:
 *   1. Walks the bytecode and generates a C translation.
 *   2. Compiles it in-memory with libtcc.
 *   3. Caches the resulting native code pointer.
 *
 * On subsequent calls, returns the cached pointer immediately.
 * Returns NULL if the function contains unsupported opcodes or if compilation
 * fails; in such cases, the caller should fall back to the interpreter.
 *
 * @param interpreter  The active interpreter, for context and constants.
 * @param fn           The function Value* to compile.
 * @return Compiled native function pointer cast to uint8_t*, or NULL on
 * failure.
 */
ZJittedFn* ZJitCompile(Interpreter* interpreter, Value* fn);

/**
 * @brief Release all cached TCC compilation contexts.
 *
 * Must be called before the interpreter is freed to avoid leaking the TCC
 * internal code regions.  After this call every previously compiled
 * uf->JitFn pointer is invalidated.
 */
void ZJitFree(void);

/* ── JIT ABI helpers ────────────────────────────────────────────────────────
 * Defined in zsjit.c and registered with tcc_add_symbol so TCC-compiled
 * code can call them without needing any struct layout information.
 * ───────────────────────────────────────────────────────────────────────── */
Value* _zsjit_popp(Interpreter* _i);
Value* _zsjit_peek(Interpreter* _i);
void   _zsjit_push(Interpreter* _i, Value* v);
Value* _zsjit_getconst(Interpreter* _i, int off);
Value* _zsjit_gettrue(Interpreter* _i);
Value* _zsjit_getfalse(Interpreter* _i);
Value* _zsjit_getnull(Interpreter* _i);
Value* _zsjit_getcellvalue(EnvCell* c);

#endif /* JIT_ZSJIT_H */
