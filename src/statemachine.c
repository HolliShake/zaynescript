#include "./statemachine.h"

extern void	  FPush(Interpreter* interpreter, CallFrame* frame, Value* value);
extern String ValueToString(Value* value);
extern void	  EnqueueTask(Interpreter* interpreter, Value* task);

/* ============================================================
 * CreatePromise
 * ============================================================ */
Promise* CreatePromise(PromiseState initial, CallFrame* suspendedCallFrame) {
	Promise* p			  = Allocate(sizeof(Promise));
	p->State			  = initial;
	p->SuspendedCallFrame = suspendedCallFrame;
	p->Callback			  = NULL;
	p->Parent			  = NULL;
	p->Globals			  = NULL;
	p->Result			  = NULL;
	p->FullfillReactions  = NULL;
	p->RejectReactions	  = NULL;
	return p;
}

/* ============================================================
 * Internal reaction list helpers  (unchanged)
 * ============================================================ */
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

/* ============================================================
 * Public reaction registration  (unchanged)
 * ============================================================ */

/* Used by await — needs to fire on EITHER fulfill or reject. */
void PromiseAddReaction(Promise* promise, Value* promiseValue) {
	_PushReactions(promise, promiseValue);
}

/* Used by .then() — fires only on fulfillment. */
void PromiseFulfillReactionAdd(Promise* promise, Value* promiseValue) {
	_PushFulfillReaction(promise, promiseValue);
}

/* Used by .catch() / .error() — fires only on rejection. */
void PromiseRejectReactionAdd(Promise* promise, Value* promiseValue) {
	_PushRejectReaction(promise, promiseValue);
}

/* Record which promise this coroutine is suspended on. */
void PromiseAwait(Promise* promise, Value* promiseValue) {
	promise->Parent = promiseValue;
}

/* ============================================================
 * _PropagateRejection
 *
 * Called when a rejection reaches a child that has no reject
 * handler (e.g. a plain .then() with no .catch()).  Skips that
 * child and forwards the rejection down to ITS children so the
 * chain keeps moving until a .catch() is found.
 * ============================================================ */
static void _PropagateRejection(Interpreter* interpreter,
								Promise*	 promise,
								Value*		 error); /* forward decl */

static void
_PropagateRejection(Interpreter* interpreter, Promise* promise, Value* error) {
	promise->State	= REJECTED;
	promise->Result = error;

	/* Does this promise have a reject handler? */
	if (promise->RejectReactions != NULL) {
		ListStateMachineNode* node = promise->RejectReactions;
		while (node != NULL) {
			Promise* child = CoerceToPromise(node->Promise);
			child->Result  = error;
			child->State   = REJECTED;
			EnqueueTask(interpreter, node->Promise);
			node = node->Next;
		}
		return;
	}

	/* No reject handler here — skip to fulfill-chained children
	 * (the next .then()s in the chain) and propagate through them. */
	ListStateMachineNode* node = promise->FullfillReactions;
	if (node == NULL) {
		/* End of chain with no handler anywhere. */
		String errStr = ValueToString(error);
		fprintf(stderr, "UnhandledPromiseRejection: %s\n", errStr);
		free(errStr);
		return;
	}

	while (node != NULL) {
		_PropagateRejection(interpreter, CoerceToPromise(node->Promise), error);
		node = node->Next;
	}
}

/* ============================================================
 * PromiseFulfill  (fixed)
 *
 * Was: only set State + Result, never notified anyone.
 * Now: walks FulfillReactions and wakes each child correctly
 *      depending on whether it is an await-frame or a callback.
 * ============================================================ */
void PromiseFulfill(Interpreter* interpreter, Promise* promise, Value* value) {
	if (promise->State != PENDING)
		return; /* already settled — ignore */

	promise->State	= FULFILLED;
	promise->Result = value;

	ListStateMachineNode* node = promise->FullfillReactions;
	while (node != NULL) {
		Promise* child = CoerceToPromise(node->Promise);

		if (child->SuspendedCallFrame != NULL) {
			/* ── await path ─────────────────────────────────
			 * The frame is sitting at OP_AWAIT.  Push the
			 * resolved value onto its operand stack so the
			 * next instruction (OP_GET_AWAITED_VALUE or just
			 * the following opcode) can consume it, then
			 * re-enqueue so the event loop resumes it. */
			FPush(interpreter, child->SuspendedCallFrame, value);
			child->SuspendedCallFrame->Suspend = false;
			child->Result					   = value;
			child->State					   = FULFILLED;
			EnqueueTask(interpreter, node->Promise);

		} else if (child->Callback != NULL) {
			/* ── .then() path ───────────────────────────────
			 * Store the input value so the event loop can
			 * pass it to the callback as argument slot 0.
			 * Do NOT set child->State to FULFILLED yet —
			 * the child only settles after the callback runs
			 * and _ChainResult decides what value it produces. */
			child->Result = value; /* input to the callback */
			EnqueueTask(interpreter, node->Promise);

		}
		/* If child has neither a frame nor a callback it is an
		 * intermediate chain link — just mark it and propagate. */
		else {
			child->Result = value;
			child->State  = FULFILLED;
			/* Recurse into its own FulfillReactions. */
			PromiseFulfill(interpreter, child, value);
		}

		node = node->Next;
	}
}

/* ============================================================
 * PromiseReject  (fixed)
 *
 * Was: only set State + Result, never notified anyone.
 * Now: walks RejectReactions and wakes each child.  If no
 *      reject reaction exists on a child it propagates the
 *      rejection down the chain via _PropagateRejection.
 * ============================================================ */
void PromiseReject(Interpreter* interpreter, Promise* promise, Value* error) {
	if (promise->State != PENDING)
		return; /* already settled — ignore */

	promise->State	= REJECTED;
	promise->Result = error;

	/* No reactions at all — fully unhandled. */
	if (promise->RejectReactions == NULL && promise->FullfillReactions == NULL) {
		String errStr = ValueToString(error);
		fprintf(stderr, "UnhandledPromiseRejection: %s\n", errStr);
		free(errStr);
		return;
	}

	/* Walk RejectReactions (.catch / await). */
	ListStateMachineNode* node = promise->RejectReactions;
	while (node != NULL) {
		Promise* child = CoerceToPromise(node->Promise);

		if (child->SuspendedCallFrame != NULL) {
			/* ── await rejection path ───────────────────────
			 * Inject the error into the suspended frame.
			 * _RaiseError will handle it when the frame runs. */
			child->SuspendedCallFrame->Error   = error;
			child->SuspendedCallFrame->Suspend = false;
			child->Result					   = error;
			child->State					   = REJECTED;
			EnqueueTask(interpreter, node->Promise);

		} else if (child->Callback != NULL) {
			/* ── .catch() / .error() path ───────────────────
			 * Pass rejection reason as argument slot 0.
			 * Child state stays as-is; event loop dispatches it. */
			child->Result = error;
			child->State  = REJECTED; /* signals REJECTED dispatch in event loop */
			EnqueueTask(interpreter, node->Promise);

		} else {
			/* Intermediate link with no handler — skip forward. */
			_PropagateRejection(interpreter, child, error);
		}

		node = node->Next;
	}

	/* If there are FulfillReactions but no RejectReactions,
	 * the rejection must bypass all the .then()s and look for
	 * a .catch() further down the chain. */
	if (promise->RejectReactions == NULL && promise->FullfillReactions != NULL) {
		node = promise->FullfillReactions;
		while (node != NULL) {
			_PropagateRejection(interpreter, CoerceToPromise(node->Promise), error);
			node = node->Next;
		}
	}
}

/* ============================================================
 * FreePromise
 * ============================================================ */
void FreePromise(Promise* promise) {
	/* Free the reaction linked lists. */
	ListStateMachineNode* node = promise->FullfillReactions;
	while (node != NULL) {
		ListStateMachineNode* next = node->Next;
		free(node);
		node = next;
	}
	node = promise->RejectReactions;
	while (node != NULL) {
		ListStateMachineNode* next = node->Next;
		free(node);
		node = next;
	}
	free(promise);
}