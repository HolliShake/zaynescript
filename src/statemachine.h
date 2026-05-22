#ifndef STATEMACHINE_H
#define STATEMACHINE_H

/**
 * @file statemachine.h
 * @brief Declares the Promise state container used by async functions and
 *        chained callbacks.
 *
 * Promise objects in LanguageX double as coroutine resumption records: they
 * track settlement state, queued reactions, and any suspended call frame that
 * must be resumed by the event loop.
 *
 * Settlement functions (PromiseFulfill / PromiseReject) are responsible for
 * walking reaction lists and enqueuing continuations.  Callers must never
 * call EnqueueTask directly on a promise — only the settlement functions may
 * do so, guaranteeing that a dequeued task is always in a settled state.
 */

#include "./global.h"

/* ============================================================
 * Lifecycle
 * ============================================================ */

/**
 * @brief Allocates a Promise record for either an async coroutine or a
 *        chained reaction callback.
 *
 * @param initial              Initial settlement state.  New promises are
 *                             almost always created as PENDING.
 * @param suspendedCallFrame   Frame to resume when this promise is dispatched,
 *                             or NULL for .then()/.error() callbacks whose
 *                             frame is built by _PromiseThen / _PromiseError.
 * @return Newly allocated Promise.  Ownership is transferred to the caller
 *         (typically wrapped immediately in a GC-managed Value).
 */
Promise* CreatePromise(PromiseState initial, CallFrame* suspendedCallFrame);

/**
 * @brief Releases the Promise allocation and frees both reaction linked lists.
 *
 * The GC manages all Value* fields (Callback, Result, Parent, Globals); this
 * function only frees the Promise struct itself and its ListStateMachineNode
 * chains.  Call only after the GC has already detached those pointers.
 *
 * @param promise Promise to free.
 */
void FreePromise(Promise* promise);

/* ============================================================
 * Reaction registration
 * ============================================================ */

/**
 * @brief Registers a continuation that fires on EITHER fulfillment or
 *        rejection.
 *
 * Used by the OP_AWAIT opcode: a suspended coroutine must resume regardless
 * of how the awaited promise settles.  Adds promiseValue to both
 * FullfillReactions and RejectReactions.
 *
 * @param promise       Promise being observed.
 * @param promiseValue  Promise Value representing the suspended continuation.
 */
void PromiseAddReaction(Promise* promise, Value* promiseValue);

/**
 * @brief Registers a continuation that fires only on fulfillment.
 *
 * Used by .then(): a rejected parent skips this callback entirely and
 * propagates the rejection downstream via _PropagateRejection.
 *
 * @param promise       Promise whose fulfillment schedules the continuation.
 * @param promiseValue  Promise Value representing the chained .then() task.
 */
void PromiseFulfillReactionAdd(Promise* promise, Value* promiseValue);

/**
 * @brief Registers a continuation that fires only on rejection.
 *
 * Used by .error() / .catch() and by skipped .then() chain links that need
 * downstream handlers to observe the propagated rejection.
 *
 * @param promise       Promise whose rejection schedules the continuation.
 * @param promiseValue  Promise Value representing the chained rejection task.
 */
void PromiseRejectReactionAdd(Promise* promise, Value* promiseValue);

/**
 * @brief Records which promise an async coroutine is currently awaiting.
 *
 * Sets promise->Parent so OP_GET_AWAITED_VALUE can read the settled result
 * or rejection reason when the coroutine resumes.
 *
 * @param promise       Suspended coroutine promise.
 * @param promiseValue  Promise currently being awaited (the one being watched).
 */
void PromiseAwait(Promise* promise, Value* promiseValue);

/* ============================================================
 * Settlement  — these are the ONLY callers of EnqueueTask
 * ============================================================ */

/**
 * @brief Settles a promise as FULFILLED, stores the resolved value, and
 *        notifies all registered fulfill reactions.
 *
 * For each child in FullfillReactions:
 *   - await path (SuspendedCallFrame != NULL): pushes value onto the frame's
 *     operand stack, clears Suspend, and enqueues the child.
 *   - .then() path (Callback != NULL): stores value as child->Result (input
 *     to the callback) and enqueues the child.  The child's own state is NOT
 *     set to FULFILLED here — it settles only after its callback runs and
 *     _ChainResult decides the outcome.
 *   - intermediate link (neither): recursively calls PromiseFulfill so the
 *     value propagates past chain gaps.
 *
 * No-op if promise->State != PENDING.
 *
 * @param interpreter  Active interpreter (needed for FPush and EnqueueTask).
 * @param promise      Promise to settle.
 * @param value        Fulfillment value delivered to awaiters and callbacks.
 */
void PromiseFulfill(Interpreter* interpreter, Promise* promise, Value* value);

/**
 * @brief Settles a promise as REJECTED, stores the rejection reason, and
 *        notifies all registered reject reactions.
 *
 * For each child in RejectReactions:
 *   - await path: sets frame->PendingError, clears Suspend, enqueues.
 *   - .catch() path: stores error as child->Result, marks child REJECTED,
 *     enqueues.
 *   - intermediate link: calls _PropagateRejection to skip past .then()
 *     children that have no reject handler, until a .catch() is found.
 *
 * If RejectReactions is NULL but FullfillReactions is not, propagates the
 * rejection through every .then() child looking for a downstream .catch().
 *
 * If no handler exists anywhere in the chain, prints an
 * UnhandledPromiseRejection warning to stderr.
 *
 * No-op if promise->State != PENDING.
 *
 * @param interpreter  Active interpreter (needed for EnqueueTask).
 * @param promise      Promise to settle.
 * @param error        Rejection reason delivered to .catch() handlers and
 *                     re-raised into suspended await frames.
 */
void PromiseReject(Interpreter* interpreter, Promise* promise, Value* error);

#endif