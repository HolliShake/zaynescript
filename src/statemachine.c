

#include "./statemachine.h"

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

static void _PushReactions(Promise* promise, Value* promiseValue) {
	_PushFulfillReaction(promise, promiseValue);
	_PushRejectReaction(promise, promiseValue);
}

void PromiseAddReaction(Promise* promise, Value* promiseValue) {
	/* Register as both fulfill and reject reaction (used by await,
	 * which needs to be notified whichever way the promise settles). */
	_PushReactions(promise, promiseValue);
}

void PromiseFulfillReactionAdd(Promise* promise, Value* promiseValue) {
	/* Used by .then: fires only on fulfillment. */
	_PushFulfillReaction(promise, promiseValue);
}

void PromiseRejectReactionAdd(Promise* promise, Value* promiseValue) {
	/* Used by .error: fires only on rejection. */
	_PushRejectReaction(promise, promiseValue);
}

void PromiseAwait(Promise* promise, Value* promiseValue) {
	/* Just record which promise this coroutine is waiting on.
	 * Reaction registration is handled by the OP_AWAIT opcode. */
	promise->Parent = promiseValue;
}

void PromiseFulfill(Promise* promise, Value* value) {
	promise->State	= FULFILLED;
	promise->Result = value;
}

void PromiseReject(Promise* promise, Value* value) {
	promise->State	= REJECTED;
	promise->Result = value;
}

void FreePromise(Promise* promise) {
	free(promise);
}