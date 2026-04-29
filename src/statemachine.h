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
 */

#include "./global.h"

/**
 * @brief Allocates a Promise record for either an async coroutine or a chained
 *        reaction callback.
 *
 * The promise stores settlement state plus enough execution context for the
 * event loop to resume suspended code or invoke a registered `.then()` or
 * `.error()` callback later.
 *
 * @param initial Initial settlement state; new promises are typically created
 *                as `PENDING`.
 * @param suspendedCallFrame Frame to resume when this promise is dispatched,
 *                           or `NULL` for callbacks whose frame will be built
 *                           lazily at run time.
 * @param parent Settled or awaited parent promise linked to this promise, or
 *               `NULL` for root async tasks.
 * @param globals Global environment visible to the resumed callback/coroutine.
 * @param callback Function Value to execute when this promise is dispatched.
 *                 May be the async function itself or a `.then()`/`.error()`
 *                 handler.
 * @return Newly allocated Promise owned by a GC-managed Value wrapper.
 */
Promise* CreatePromise(PromiseState initial,
					   CallFrame*	suspendedCallFrame,
					   Value*		parent,
					   Value*		globals,
					   Value*		callback);

/**
 * @brief Registers a listener that should run on either fulfillment or
 *        rejection.
 *
 * This is the await path: the suspended coroutine must resume regardless of
 * whether the awaited promise resolves or rejects.
 *
 * @param promise Promise being observed.
 * @param promiseValue Promise Value representing the suspended continuation to
 *                     enqueue when `promise` settles.
 */
void PromiseAddReaction(Promise* promise, Value* promiseValue);

/**
 * @brief Registers a continuation that should run only after fulfillment.
 *
 * Used by `.then()` so rejected parents skip this callback entirely.
 *
 * @param promise Promise whose fulfillment should schedule the continuation.
 * @param promiseValue Promise Value representing the chained `.then()` task.
 */
void PromiseFulfillReactionAdd(Promise* promise, Value* promiseValue);

/**
 * @brief Registers a continuation that should run only after rejection.
 *
 * Used by `.error()` and by skipped `.then()` chains that need downstream
 * handlers to observe the propagated rejection.
 *
 * @param promise Promise whose rejection should schedule the continuation.
 * @param promiseValue Promise Value representing the chained rejection task.
 */
void PromiseRejectReactionAdd(Promise* promise, Value* promiseValue);

/**
 * @brief Records which promise an async coroutine is currently awaiting.
 *
 * The opcode handler uses this parent link so `OP_GET_AWAITED_VALUE` can read
 * the settled result or rejection reason when the coroutine resumes.
 *
 * @param promise Suspended coroutine promise whose await parent is being set.
 * @param promiseValue Promise currently being awaited.
 */
void PromiseAwait(Promise* promise, Value* promiseValue);

/**
 * @brief Marks a promise as fulfilled and stores the resolved value.
 *
 * Reaction queues are not drained here; callers settle first and then decide
 * whether to enqueue chained tasks.
 *
 * @param promise Promise whose state should become `FULFILLED`.
 * @param value Value delivered to awaiters and `.then()` callbacks.
 */
void PromiseFulfill(Promise* promise, Value* value);

/**
 * @brief Marks a promise as rejected and stores the rejection reason.
 *
 * The caller remains responsible for scheduling reject reactions or unhandled
 * rejection reporting after settlement.
 *
 * @param promise Promise whose state should become `REJECTED`.
 * @param value Error or rejection payload to expose to downstream handlers.
 */
void PromiseReject(Promise* promise, Value* value);

/**
 * @brief Releases the raw Promise allocation after GC has detached its linked
 *        reaction nodes.
 *
 * @param promise Promise struct to free. Its callback/result Values remain
 *                owned by the garbage collector.
 */
void FreePromise(Promise* promise);

#endif