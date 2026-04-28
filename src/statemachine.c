
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

static void _PushReactions(Promise* promise, Value* promiseValue) {
	ListStateMachineNode* node = Allocate(sizeof(ListStateMachineNode));
	node->Promise			   = promiseValue;
	node->Next				   = NULL;

	// Add to the end of the FullfillReactions list
	if (promise->FullfillReactions == NULL) {
		promise->FullfillReactions = node;
	} else {
		ListStateMachineNode* ftail = promise->FullfillReactions;
		while (ftail->Next != NULL) {
			ftail = ftail->Next;
		}
		ftail->Next = node;
	}

	// Add to the end of the RejectReactions list
	if (promise->RejectReactions == NULL) {
		promise->RejectReactions = node;
	} else {
		ListStateMachineNode* rtail = promise->RejectReactions;
		while (rtail->Next != NULL) {
			rtail = rtail->Next;
		}
		rtail->Next = node;
	}
}

void PromiseAddReaction(Promise* promise, Value* promiseValue) {
	_PushReactions(promise, promiseValue);
}

void PromiseAwait(Promise* promise, Value* promiseValue) {
	promise->State	= PENDING;
	promise->Parent = promiseValue;
	_PushReactions(promise, promiseValue);
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