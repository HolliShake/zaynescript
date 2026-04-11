/**
 * @file zsjit.h
 * @brief ZayneScript TCC-based JIT compiler interface.
 *
 * Translates UserFunction bytecode to C source on the fly, compiles it with
 * libtcc in-memory, and returns a native function pointer.  Functions that
 * contain unsupported opcodes (async/await) transparently fall back to the
 * bytecode interpreter.
 *
 * ## Architecture overview
 *
 *  Bytecode (UserFunction)
 *       │
 *       ▼
 *  Codegen()  ── walks opcodes, emits a single C function body into a
 *               StrBuilder, wraps it in the `fncode` template, and returns
 *               the complete C source as a heap string.
 *       │
 *       ▼
 *  libtcc  ── compiles the C source in-memory (TCC_OUTPUT_MEMORY),
 *             symbols are resolved via tcc_add_symbol() to the _zsjit_*
 *             ABI helpers below, then tcc_relocate() patches all
 *             call sites.
 *       │
 *       ▼
 *  __jit_fn  ── the resolved native function pointer, cached on
 *               UserFunction->JitFn so subsequent calls skip codegen
 *               and compilation entirely.
 *
 * ## JIT ABI
 *
 * All generated functions share the same C signature:
 *
 *   Value* __jit_fn(Interpreter* _i, Value* _re, Value* _ce, Value* _fn);
 *
 * They never access interpreter or environment struct fields directly;
 * every runtime operation goes through the `_zsjit_*` helper functions
 * declared in this file so that no struct layout information needs to be
 * embedded in the generated source.
 */

#include "../src/environment.h"
#include "../src/function.h"
#include "../src/global.h"
#include "../src/hashmap.h"
#include "../src/operation.h"
#include <libtcc.h>


#ifndef JIT_ZSJIT_H
#	define JIT_ZSJIT_H

/* ── Public type ────────────────────────────────────────────────────────── */

/**
 * @brief Type signature for a TCC-compiled JIT function.
 *
 * @param _i   The active Interpreter instance.
 * @param _re  The module-level root environment Value*.
 * @param _ce  The current call-frame environment Value*.
 * @param _fn  The UserFunction Value* whose bytecode is being executed.
 *
 * On a clean return the function has already pushed its result onto the
 * interpreter stack (mirroring OP_RETURN semantics) and returns NULL.
 * On an unrecoverable runtime error it returns the error Value* directly,
 * so the caller (e.g. Run) can hand it to the normal error-handling path.
 */
typedef void* (*ZJittedFn)(Interpreter* _i, Value* _re, Value* _ce, Value* _fn);

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief JIT-compile a UserFunction using TCC as the backend.
 *
 * On the first call for a given UserFunction this function:
 *   1. Walks the bytecode with Compute() to collect all jump-target offsets
 *      so they can be emitted as C labels.
 *   2. Walks the bytecode a second time with Codegen() to emit a C
 *      translation of every supported opcode.
 *   3. Wraps the body in the `fncode` template and compiles it in-memory
 *      with libtcc.
 *   4. Registers all _zsjit_* ABI symbols with tcc_add_symbol().
 *   5. Relocates the code and resolves the `__jit_fn` symbol.
 *   6. Stores the native pointer in UserFunction->JitFn and appends the
 *      TCCState to the internal registry so ZJitFree() can release it later.
 *
 * On subsequent calls the cached pointer is returned immediately — no
 * codegen or compilation happens.
 *
 * Returns NULL without modifying state if:
 *   - Codegen() encounters an unsupported opcode (caller must fall back
 *     to the bytecode interpreter).
 *   - tcc_new() or tcc_compile_string() or tcc_relocate() fails.
 *   - The compiled module does not export `__jit_fn`.
 *
 * @param interpreter  The active interpreter (used for constant pool access
 *                     and to pass to Codegen for context).
 * @param fn           The function Value* to compile; must wrap a
 *                     UserFunction.
 * @return             Pointer to the compiled ZJittedFn, or NULL on failure.
 */
ZJittedFn* ZJitCompile(Interpreter* interpreter, Value* fn);

/**
 * @brief Release all cached TCC compilation contexts.
 *
 * Iterates the internal TCCState registry and calls tcc_delete() on each
 * entry, freeing the in-memory code regions they own.  After this call every
 * previously compiled UserFunction->JitFn pointer is invalidated and must
 * not be called.
 *
 * Must be called before the interpreter is freed (e.g. from
 * FreeInterpreter) to avoid leaking TCC's internal memory regions.
 *
 * This function is idempotent: calling it when no states are registered
 * (or calling it a second time) is safe.
 */
void ZJitFree(void);

#endif /* JIT_ZSJIT_H */
