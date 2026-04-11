/**
 * @file zsjit.h
 * @brief ZayneScript TCC-based JIT compiler interface.
 *
 * Translates UserFunction bytecode to C source on the fly, compiles it with
 * libtcc in-memory, and returns a native function pointer.  Functions that
 * contain unsupported opcodes (try/catch, async/await, class/array/object
 * literals) transparently fall back to the bytecode interpreter.
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

/* ── JIT ABI helpers ────────────────────────────────────────────────────────
 * Defined in zsjit.c and registered with tcc_add_symbol() so that
 * TCC-compiled functions can call them without embedding any struct layout
 * knowledge in the generated C source.
 *
 * Naming convention:  _zsjit_<verb>[_<noun>]
 *   get*  — read a value from the interpreter or an object; never modifies
 *            reference counts or GC state.
 *   set*  — write a value into the interpreter.
 *   push, popp, peek  — standard value-stack operations.
 *   lock* — copy-on-write helper for captured variables.
 *   pushtry/popptry  — exception-handler stack management.
 * ───────────────────────────────────────────────────────────────────────── */

/**
 * @brief Pop and return the top value from the interpreter's value stack.
 *
 * Equivalent to: return _i->Stacks[--_i->StckC];
 *
 * @param _i  The active interpreter.
 * @return    The value that was on top of the stack.
 */
Value* _zsjit_popp(Interpreter* _i);

/**
 * @brief Return the top value of the interpreter's value stack without
 *        popping it.
 *
 * Equivalent to: return _i->Stacks[_i->StckC - 1];
 *
 * @param _i  The active interpreter.
 * @return    The value currently on top of the stack.
 */
Value* _zsjit_peek(Interpreter* _i);

/**
 * @brief Push a value onto the interpreter's value stack.
 *
 * Equivalent to: _i->Stacks[_i->StckC++] = v;
 *
 * @param _i  The active interpreter.
 * @param v   The value to push.
 */
void _zsjit_push(Interpreter* _i, Value* v);

/**
 * @brief Fetch a value from the interpreter's constant pool by index.
 *
 * Equivalent to: return _i->Constants[off];
 *
 * @param _i  The active interpreter.
 * @param off Zero-based index into the constant pool.
 * @return    The constant Value* at position @p off.
 */
Value* _zsjit_getconst(Interpreter* _i, int off);

/**
 * @brief Return the interpreter's canonical `true` singleton.
 *
 * @param _i  The active interpreter.
 * @return    _i->True
 */
Value* _zsjit_gettrue(Interpreter* _i);

/**
 * @brief Return the interpreter's canonical `false` singleton.
 *
 * @param _i  The active interpreter.
 * @return    _i->False
 */
Value* _zsjit_getfalse(Interpreter* _i);

/**
 * @brief Return the interpreter's canonical `null` singleton.
 *
 * @param _i  The active interpreter.
 * @return    _i->Null
 */
Value* _zsjit_getnull(Interpreter* _i);

/**
 * @brief Unwrap the Value* stored inside an EnvCell.
 *
 * Used by OP_LOAD_LOCAL / OP_LOAD_NAME after EnvironmentGetLocal() returns
 * the cell — the generated code calls this to get the actual runtime value
 * without knowing the EnvCell struct layout.
 *
 * @param c  A non-NULL EnvCell pointer.
 * @return   The Value* held by the cell.
 */
Value* _zsjit_getcellvalue(EnvCell* c);

/**
 * @brief Return the captured variable at index @p off from a UserFunction's
 *        capture list.
 *
 * Delegates to UserFunctionGetCapture(_uf, off).  Used by OP_LOAD_CAPTURE.
 *
 * @param _uf  The UserFunction owning the capture list.
 * @param off  Zero-based capture slot index.
 * @return     The captured Value*.
 */
Value* _zsjit_getcap(UserFunction* _uf, int off);

/**
 * @brief Push an ExceptionHandler frame onto the interpreter's exception
 *        handler stack.
 *
 * Called by the code generated for OP_SETUP_TRY.  The @p pausedAddress is
 * the bytecode IP immediately after the SETUP_TRY instruction, recorded so
 * that if a thrown error is caught by the bytecode interpreter (because the
 * JIT function is paused mid-execution) it can resume at the right place.
 *
 * @param _i             The active interpreter.
 * @param jmp            Bytecode IP of the corresponding catch/finally block.
 * @param pausedAddress  Bytecode IP of the first instruction inside the try
 *                       body (i.e. ip right after SETUP_TRY).
 */
void _zsjit_pushtry(Interpreter* _i, int jmp, size_t pausedAddress);

/**
 * @brief Pop the most recently pushed ExceptionHandler frame.
 *
 * Called by the code generated for OP_POP_TRY after a try block exits
 * normally (no exception was raised).
 *
 * @param _i  The active interpreter.
 */
void _zsjit_popptry(Interpreter* _i);

/**
 * @brief Store an error value in the interpreter's pending-error slot.
 *
 * Equivalent to: _i->Error = err;
 *
 * The JIT checks this slot after every call that can fail; if it is non-NULL
 * and a local handler is active the generated code jumps to the catch label,
 * otherwise it propagates by returning the error to the host.
 *
 * @param _i   The active interpreter.
 * @param err  The error Value* to store, or NULL to clear the error.
 */
void _zsjit_seterror(Interpreter* _i, Value* err);

/**
 * @brief Read the interpreter's pending-error slot.
 *
 * Equivalent to: return _i->Error;
 *
 * @param _i  The active interpreter.
 * @return    The current error Value*, or NULL if no error is pending.
 */
Value* _zsjit_geterror(Interpreter* _i);

/**
 * @brief Return the current stack pointer (number of live stack slots).
 *
 * Equivalent to: return _i->StckC;
 *
 * Used by the rotation opcodes (OP_ROT2/3/4) which need absolute stack
 * indices to swap values without knowing the interpreter struct layout.
 *
 * @param _i  The active interpreter.
 * @return    Current value of the stack depth counter.
 */
size_t _zsjit_getstackpntr(Interpreter* _i);

/**
 * @brief Read a value at an absolute stack index without popping.
 *
 * Equivalent to: return _i->Stacks[i];
 *
 * @param _i  The active interpreter.
 * @param i   Absolute (zero-based) stack index.
 * @return    The Value* at that index.
 */
Value* _zsjit_getstack(Interpreter* _i, int i);

/**
 * @brief Overwrite a value at an absolute stack index.
 *
 * Equivalent to: _i->Stacks[i] = val;
 *
 * @param _i  The active interpreter.
 * @param i   Absolute (zero-based) stack index.
 * @param val The new Value* to store at that position.
 */
void _zsjit_setstack(Interpreter* _i, int i, Value* val);

/**
 * @brief Copy-on-write helper for OP_LOCK_VAR.
 *
 * When a local variable slot is shared with a closure (IsCaptured && RefCount
 * > 0), this function decrements the reference count and replaces the slot
 * with a fresh EnvCell wrapping the same Value*.  This ensures the local
 * binding and the captured binding diverge correctly at assignment time.
 *
 * @param _i      The active interpreter.
 * @param envObj  The Value* wrapping the current call-frame Environment.
 * @param off     Zero-based local slot index within that environment.
 */
void _zsjit_lockvar(Interpreter* _i, Value* envObj, int off);

#endif /* JIT_ZSJIT_H */
