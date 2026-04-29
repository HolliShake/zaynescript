
/**
 * @file statemachine.c
 * @brief Implements Promise allocation, reaction registration, and settlement
 *        state updates.
 *
 * These helpers keep the Promise struct intentionally small: they only mutate
 * state and linked reaction lists while leaving scheduling decisions to the
 * interpreter.
 */

#include "./statemachine.h"

/**
 * @brief Allocates a Promise struct and seeds its execution metadata.
 *
 * @param initial Initial settlement state for the new promise.
 * @param suspendedCallFrame Frame to resume when the promise is dispatched, or
 *                           `NULL` when the frame will be created lazily.
 * @param parent Parent promise linked through await/chaining, or `NULL` for a
 *               root task.
 * @param globals Global environment visible when the callback or coroutine
 *                resumes.
 * @param callback Function Value to execute when this promise is dispatched.
 * @return Newly allocated Promise struct with empty reaction lists.
 */
Promise* CreatePromise(PromiseState initial,
					   CallFrame*	suspendedCallFrame,
					   Value*		parent,
					   Value*		globals,
					   Value*		callback) {
	Promise* p			  = Allocate(sizeof(Promise));
	p->State			  = initial;
	p->SuspendedCallFrame = suspendedCallFrame;
	p->Callback			  = callback;
	p->Parent			  = parent;
	p->Globals			  = globals;
	p->Result			  = NULL;
	p->FullfillReactions  = NULL;
	p->RejectReactions	  = NULL;
	return p;
}

/**
 * @brief Appends a continuation to the fulfillment reaction list.
 *
 * @param promise Promise whose fulfillment queue should grow.
 * @param promiseValue Promise Value representing the chained continuation.
 */
static void _PushFulfillReaction(Promise* promise, Value* promiseValue) {
	ListStateMachineNode* node = Allocate(sizeof(ListStateMachineNode));
	node->Promise			   = promiseValue;
	node->Next				   = NULL;

	if (promise->FullfillReactions == NULL) {
		promise->FullfillReactions = node;
	} else {
		ListStateMachineNode* tail = promise->FullfillReactions;
		while (tail->Next != NULL)
			tail = tail->Next;
		tail->Next = node;
	}
}

/**
 * @brief Appends a continuation to the rejection reaction list.
 *
 * @param promise Promise whose rejection queue should grow.
 * @param promiseValue Promise Value representing the chained continuation.
 */
static void _PushRejectReaction(Promise* promise, Value* promiseValue) {
	ListStateMachineNode* node = Allocate(sizeof(ListStateMachineNode));
	node->Promise			   = promiseValue;
	node->Next				   = NULL;

	if (promise->RejectReactions == NULL) {
		promise->RejectReactions = node;
	} else {
		ListStateMachineNode* tail = promise->RejectReactions;
		while (tail->Next != NULL)
			tail = tail->Next;
		tail->Next = node;
	}
}

/**
 * @brief Registers the same continuation for both fulfillment and rejection.
 *
 * @param promise Promise being observed.
 * @param promiseValue Suspended continuation promise to enqueue on settlement.
 */
static void _PushReactions(Promise* promise, Value* promiseValue) {
	_PushFulfillReaction(promise, promiseValue);
	_PushRejectReaction(promise, promiseValue);
}

/**
 * @brief Registers an await-style continuation that resumes on any settlement.
 *
 * @param promise Promise being observed.
 * @param promiseValue Suspended continuation promise to enqueue later.
 */
void PromiseAddReaction(Promise* promise, Value* promiseValue) {
	/* Register as both fulfill and reject reaction (used by await,
	 * which needs to be notified whichever way the promise settles). */
	_PushReactions(promise, promiseValue);
}

/**
 * @brief Registers a continuation that should run only when the promise
 *        fulfills.
 *
 * @param promise Promise whose fulfillment should trigger the continuation.
 * @param promiseValue Chained promise representing the `.then()` callback.
 */
void PromiseFulfillReactionAdd(Promise* promise, Value* promiseValue) {
	/* Used by .then: fires only on fulfillment. */
	_PushFulfillReaction(promise, promiseValue);
}

/**
 * @brief Registers a continuation that should run only when the promise
 *        rejects.
 *
 * @param promise Promise whose rejection should trigger the continuation.
 * @param promiseValue Chained promise representing the `.error()` callback.
 */
void PromiseRejectReactionAdd(Promise* promise, Value* promiseValue) {
	/* Used by .error: fires only on rejection. */
	_PushRejectReaction(promise, promiseValue);
}

/**
 * @brief Stores the promise currently being awaited by a suspended coroutine.
 *
 * @param promise Continuation promise that will resume later.
 * @param promiseValue Promise whose settled result should be observed.
 */
void PromiseAwait(Promise* promise, Value* promiseValue) {
	/* Just record which promise this coroutine is waiting on.
	 * Reaction registration is handled by the OP_AWAIT opcode. */
	promise->Parent = promiseValue;
}

/**
 * @brief Marks a promise as fulfilled and records the resolved value.
 *
 * @param promise Promise to settle.
 * @param value Fulfillment payload to expose to downstream continuations.
 */
void PromiseFulfill(Promise* promise, Value* value) {
	promise->State	= FULFILLED;
	promise->Result = value;
}

/**
 * @brief Marks a promise as rejected and records the rejection reason.
 *
 * @param promise Promise to settle.
 * @param value Rejection payload to expose to downstream continuations.
 */
void PromiseReject(Promise* promise, Value* value) {
	promise->State	= REJECTED;
	promise->Result = value;
}

/**
 * @brief Releases the raw Promise struct after GC has handled linked lists and
 *        referenced Values.
 *
 * @param promise Promise allocation to free.
 */
void FreePromise(Promise* promise) {
	free(promise);
}