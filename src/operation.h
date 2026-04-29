/**
 * @file operation.h
 * @brief Declares the runtime helpers that implement bytecode-level object,
 *        import, call, and operator semantics.
 *
 * The interpreter's dispatch loop delegates most nontrivial behavior to these
 * helpers so that imports, attribute resolution, constructor calls, and unary /
 * binary operators stay centralized and reusable across opcodes.
 */

#ifndef OPERATION_H
#define OPERATION_H

#include "./core/loader.h"
#include "./environment.h"
#include "./error.h"
#include "./gc.h"
#include "./global.h"
#include "./import.h"
#include "./value.h"

/**
 * @brief Placeholder hook for root-environment stack management.
 *
 * The current implementation is intentionally inert: legacy environment-stack
 * bookkeeping has been commented out, so calling this function has no effect.
 *
 * @param interp Interpreter whose environment stack would be updated in a
 *               fuller implementation.
 * @param env Environment Value that would become the saved root.
 */
void SaveRootEnv(Interpreter* interp, Value* env);

/**
 * @brief Placeholder hook for saving the current environment.
 *
 * This currently performs no mutation; it exists so older call sites can keep
 * compiling while the runtime uses direct frame environments instead.
 *
 * @param interp Interpreter whose environment stack would be extended.
 * @param envObj Environment Value that would be pushed.
 */
void SaveEnv(Interpreter* interp, Value* envObj);

/**
 * @brief Placeholder hook for restoring the most recently saved environment.
 *
 * The implementation is currently a no-op.
 *
 * @param interp Interpreter whose environment stack would be popped.
 */
void RestoreEnv(Interpreter* interp);

/**
 * @brief Placeholder hook for restoring a non-top saved environment.
 *
 * Older code expected this to resynchronize a previously saved lexical scope,
 * but the current implementation leaves interpreter state unchanged.
 *
 * @param interp Interpreter whose environment stack would be adjusted.
 * @param n Zero-based logical depth of the environment to restore.
 */
void RestoreNthEnvAndSync(Interpreter* interp, int n);

/**
 * @brief Determines whether a call should keep the receiver as `this` when
 *        dispatching a named member.
 *
 * The implementation checks Promise, Array, and class-instance prototype
 * chains for callable members. Plain objects currently do not expose a
 * prototype chain here, so only direct type-specific method tables participate.
 *
 * @param interp Interpreter whose built-in prototype singletons are queried.
 * @param obj Candidate receiver.
 * @param method Member name Value to resolve.
 * @return `true` when the lookup finds a method that should receive `this`.
 */
bool IsMethodOfObject(Interpreter* interp, Value* obj, Value* method);

/**
 * @brief Resolves an index or attribute lookup across the runtime's supported
 *        receiver types.
 *
 * Arrays accept numeric indices and, for method calls, Array prototype members;
 * plain objects read hash-map properties; classes search both instance and
 * static members across the inheritance chain; class instances search their
 * prototype then per-instance members; strings expose integer indexing as
 * single-character strings; unresolved lookups return `interpreter->Null`.
 *
 * @param interp Interpreter that provides built-in prototype objects and Null.
 * @param obj Receiver being indexed.
 * @param index Property key or numeric index Value.
 * @param forMethodCall When `true`, prototype method tables are considered for
 *                      callable dispatch.
 * @return Resolved member Value, an Error Value for invalid/out-of-range
 *         indexing, or `interpreter->Null` when no attribute exists.
 */
Value* GenericGetAttribute(Interpreter* interp,
						   Value*		obj,
						   Value*		index,
						   bool			forMethodCall);

/**
 * @brief Loads a built-in core module and memoizes it in the interpreter's
 *        import cache.
 *
 * Repeated imports return the cached module object rather than rebuilding it.
 *
 * @param interp Interpreter whose import cache and core loader are consulted.
 * @param moduleName Logical core-module name such as `math` or `promise`.
 * @return Cached/built module Value, or an Error Value when the named core
 *         module is unavailable.
 */
Value* DoImportCore(Interpreter* interp, String moduleName);

/**
 * @brief Imports a library module from the configured `lib/` search paths.
 *
 * The implementation resolves `<name>.zs`, performs cycle detection, compiles
 * the file, executes it in a fresh module environment, and caches the result.
 *
 * @param interp Interpreter whose import graph and module cache are mutated.
 * @param moduleName Library-relative module path without the `.zs` suffix.
 * @return Module export Value on success, or an Error Value if resolution,
 *         parsing, compilation, or execution fails.
 */
Value* DoImportLib(Interpreter* interp, String moduleName);

/**
 * @brief Imports a module from an explicit file path relative to the caller.
 *
 * This follows the same lex/parse/compile/execute/cache path as `DoImportLib`,
 * but resolves the target from the provided filesystem path stem instead of the
 * library search path.
 *
 * @param interp Interpreter whose import graph and module cache are mutated.
 * @param filePath File path stem without the `.zs` suffix.
 * @return Module export Value on success, or an Error Value if the file cannot
 *         be resolved or executed.
 */
Value* DoImportFile(Interpreter* interp, String filePath);

/**
 * @brief Stores `val` into an indexable receiver using the runtime's assignment
 *        rules.
 *
 * Arrays require an in-range numeric index, objects reject writes when marked
 * read-only, class instances write instance members, and classes write static
 * members.
 *
 * @param interp Interpreter that owns the Null singleton and any Error Values.
 * @param obj Receiver being mutated.
 * @param index Numeric or string-like key used for the assignment.
 * @param val Value to store.
 * @return `interpreter->Null` on success, or an Error Value when the receiver
 *         cannot be indexed or the index is invalid.
 */
Value* DoSetIndex(Interpreter* interp, Value* obj, Value* index, Value* val);

/**
 * @brief Resolves a non-method index lookup.
 *
 * This is a thin wrapper over `GenericGetAttribute(..., false)` used by the
 * `OP_GET_INDEX` opcode.
 *
 * @param interp Interpreter that provides built-ins and Null.
 * @param obj Receiver being indexed.
 * @param index Property key or numeric index Value.
 * @return Resolved member Value, Error Value, or `interpreter->Null` when the
 *         lookup misses.
 */
Value* DoGetIndex(Interpreter* interp, Value* obj, Value* index);

/**
 * @brief Instantiates a class and, when present, invokes its `init`
 *        constructor.
 *
 * Built-in `Object`, `Array`, and `Blob` classes allocate specialized backing
 * Values; other classes produce a generic class-instance wrapper. When no
 * constructor exists the function requires zero user arguments and returns the
 * fresh instance directly.
 *
 * @param interp Interpreter that owns the new instance and any Error Values.
 * @param frame Caller frame whose operand stack already contains constructor
 *              arguments.
 * @param clsValue Class Value to instantiate.
 * @param argc Number of user-supplied constructor arguments currently on the
 *             stack.
 * @return Constructor result/Error sentinel exactly as expected by the caller:
 *         `interpreter->Null` for normal completion paths or an Error Value on
 *         invalid class/arity conditions.
 */
Value*
DoCallCtor(Interpreter* interp, CallFrame* frame, Value* clsValue, int argc);

/**
 * @brief Resolves a named member call and normalizes whether the receiver stays
 *        on the stack as `this`.
 *
 * If the resolved member is not treated as a method, the helper rotates the
 * operand stack so the receiver is discarded and only explicit call arguments
 * remain.
 *
 * @param interp Interpreter that owns lookup helpers and any Error Values.
 * @param frame Caller frame whose operand stack holds receiver plus arguments.
 * @param obj Receiver Value used for member lookup.
 * @param methodName Property key naming the method or callable member.
 * @param argc Number of operands associated with the call.
 * @return Result from `DoCall`, or an Error Value when the member is missing.
 */
Value* DoCallMethod(Interpreter* interp,
					CallFrame*	 frame,
					Value*		 obj,
					Value*		 methodName,
					int			 argc);

/**
 * @brief Dispatches fn as a callable: wires environments for user functions,
 * unwraps async targets into promises, validates native arity, consumes argc
 * stack operands, and leaves the callee result (or an Error value) per the
 * concrete callee kind.
 *
 * @param interpreter Full VM state (stack, env stack, traces, active task).
 * @param frame       Current call frame for environment context (user function
 * scope, caller env, etc.).
 * @param fn          User function, native function, promise continuation, or
 * related callable Value.
 * @param argc        Operand count already present on the stack for this call
 * (see withThis for layout).
 * @param withThis    When true, the lowest logical argument on the stack is
 * bound as the callee's this.
 *
 * @return Callee result, interpreter->Null for async promise bootstrap paths,
 * or an Error Value on failure.
 */
Value* DoCall(Interpreter* interpreter,
			  CallFrame*   frame,
			  Value*	   fn,
			  int		   argc,
			  bool		   withThis);

/**
 * @brief Returns the boolean negation of `val` using the VM's truthiness rules.
 *
 * @param interp Interpreter that owns the canonical `True` and `False`
 *               singletons.
 * @param val Value to coerce to boolean before negation.
 * @return Either `interpreter->True` or `interpreter->False`.
 */
Value* DoNot(Interpreter* interp, Value* val);

/**
 * @brief Applies unary `+` by copying numeric values into a fresh Value.
 *
 * Small ints and doubles are duplicated directly; big-number operands are
 * cloned through libbf so later mutations do not alias the original.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param val Operand to normalize as a numeric value.
 * @return Fresh numeric Value matching the operand, or an Error Value for
 *         non-numeric input.
 */
Value* DoPos(Interpreter* interp, Value* val);

/**
 * @brief Applies unary `-` to a numeric operand.
 *
 * Big-number operands are copied and sign-flipped through libbf so the original
 * operand remains unchanged.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param val Operand to negate.
 * @return Negated numeric Value, or an Error Value for non-numeric input.
 */
Value* DoNeg(Interpreter* interp, Value* val);

/**
 * @brief Applies bitwise complement to integer-like operands.
 *
 * Big-number values use the two's-complement identity `~n == -n - 1` because
 * libbf does not expose a direct unary-not helper.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param val Operand to complement.
 * @return Complemented numeric Value, or an Error Value for non-numeric input.
 */
Value* DoBitNot(Interpreter* interp, Value* val);

/**
 * @brief Materializes the runtime type tag of `val` as a script string.
 *
 * @param interpreter Interpreter that owns the returned string Value.
 * @param val Value whose type name should be exposed.
 * @return New string Value containing the result of `ValueTypeOf(val)`.
 */
Value* DoGetType(Interpreter* interpreter, Value* val);

/**
 * @brief Multiplies two numeric operands while preserving small-int results
 *        when possible.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return Numeric product, or an Error Value when either operand is not
 *         numeric.
 */
Value* DoMul(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Divides one numeric operand by another.
 *
 * The implementation rejects division by zero and promotes to wider numeric
 * representations as needed.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param lhs Dividend.
 * @param rhs Divisor.
 * @return Quotient Value, or an Error Value for non-numeric operands or zero
 *         divisors.
 */
Value* DoDiv(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Computes the remainder of one numeric operand divided by another.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param lhs Dividend.
 * @param rhs Divisor.
 * @return Remainder Value, or an Error Value for non-numeric operands or zero
 *         divisors.
 */
Value* DoMod(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Returns `val + 1` for numeric operands.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param val Operand to increment.
 * @return Incremented numeric Value, or an Error Value for non-numeric input.
 */
Value* DoInc(Interpreter* interp, Value* val);

/**
 * @brief Adds two operands using LanguageX's numeric and string concatenation
 *        rules.
 *
 * Numeric pairs are summed arithmetically; if either operand is a string the
 * runtime concatenates their string forms instead.
 *
 * @param interp Interpreter that owns the returned Value.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return Sum/concatenation result, or an Error Value for unsupported operand
 *         combinations.
 */
Value* DoAdd(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Returns `val - 1` for numeric operands.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param val Operand to decrement.
 * @return Decremented numeric Value, or an Error Value for non-numeric input.
 */
Value* DoDec(Interpreter* interp, Value* val);

/**
 * @brief Subtracts one numeric operand from another.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param lhs Minuend.
 * @param rhs Subtrahend.
 * @return Difference Value, or an Error Value for non-numeric operands.
 */
Value* DoSub(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Applies bitwise left shift after coercing both operands to integer
 *        form.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param lhs Value supplying the bits to shift.
 * @param rhs Shift distance.
 * @return Shifted numeric Value, or an Error Value for non-numeric operands.
 */
Value* DoLShift(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Applies bitwise right shift after coercing both operands to integer
 *        form.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param lhs Value supplying the bits to shift.
 * @param rhs Shift distance.
 * @return Shifted numeric Value, or an Error Value for non-numeric operands.
 */
Value* DoRShift(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Compares two numeric operands with `<`.
 *
 * @param interp Interpreter that owns the canonical boolean singletons.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return `True`/`False` on success, or an Error Value for non-numeric input.
 */
Value* DoLT(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Compares two numeric operands with `<=`.
 *
 * @param interp Interpreter that owns the canonical boolean singletons.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return `True`/`False` on success, or an Error Value for non-numeric input.
 */
Value* DoLTE(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Compares two numeric operands with `>`.
 *
 * @param interp Interpreter that owns the canonical boolean singletons.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return `True`/`False` on success, or an Error Value for non-numeric input.
 */
Value* DoGT(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Compares two numeric operands with `>=`.
 *
 * @param interp Interpreter that owns the canonical boolean singletons.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return `True`/`False` on success, or an Error Value for non-numeric input.
 */
Value* DoGTE(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Compares two Values using the runtime's equality semantics.
 *
 * @param interp Interpreter that owns the canonical boolean singletons.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return `True` when `ValueIsEqual(lhs, rhs)` succeeds, otherwise `False`.
 */
Value* DoEQ(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Negates the runtime equality comparison between two operands.
 *
 * @param interp Interpreter that owns the canonical boolean singletons.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return `True` when the operands are not equal under `ValueIsEqual`.
 */
Value* DoNE(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Applies bitwise AND after coercing operands to integer form.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return Bitwise-AND result, or an Error Value for non-numeric operands.
 */
Value* DoAnd(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Applies bitwise OR after coercing operands to integer form.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return Bitwise-OR result, or an Error Value for non-numeric operands.
 */
Value* DoOr(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Applies bitwise XOR after coercing operands to integer form.
 *
 * @param interp Interpreter that owns the returned numeric Value.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return Bitwise-XOR result, or an Error Value for non-numeric operands.
 */
Value* DoXor(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * @brief Materializes a function Value from the interpreter's compiled-function
 *        table.
 *
 * When `closure` is true the helper clones the function metadata and captures
 * the current lexical environment so later calls see the correct closed-over
 * values.
 *
 * @param interp Interpreter whose function registry is being indexed.
 * @param frame Current frame supplying local environment cells for captures.
 * @param offset Index into `interpreter->Functions`.
 * @param closure Whether to clone the function into a closure instead of
 *                returning the shared compiled definition directly.
 * @return Callable Value ready to push onto the operand stack.
 */
Value*
DoLoadFunction(Interpreter* interp, CallFrame* frame, int offset, bool closure);

#endif
