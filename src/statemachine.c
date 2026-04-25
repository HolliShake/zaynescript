
#include "./statemachine.h"

StateMachine* CreateStateMachine(StateMachineState initial,
								 bool			   isCallback,
								 Value*			   promise,
								 Value*			   function) {
	StateMachine* sm = Allocate(sizeof(StateMachine));
	sm->State		 = initial;
	sm->Frame		 = NULL;
	sm->IsCallback	 = isCallback;
	sm->GlobalEnv	 = NULL;
	sm->Callback	 = function;
	sm->WaitFor		 = promise;
	sm->Value		 = NULL;
	sm->IsCatched	 = false;
	sm->WaitList = Allocate(sizeof(Value*)), sm->WaitList[0] = NULL;
	sm->WaitListC = 0;
	return sm;
}

void StateMachineSet(StateMachine*	   stateMachine,
					 StateMachineState newState,
					 Value*			   waitFor,
					 Value*			   value) {
	stateMachine->State	  = newState;
	stateMachine->WaitFor = waitFor;
	stateMachine->Value	  = value;
}

void StateMachineUpdate(StateMachine*	  stateMachine,
						StateMachineState newState,
						Value*			  value) {
	stateMachine->State = newState;
	stateMachine->Value = value;
}

void StateMachineAwait(StateMachine* stateMachine, Value* promise) {
	stateMachine->State	  = PENDING;
	stateMachine->WaitFor = promise;
}

void StateMachineFulfill(StateMachine* stateMachine, Value* value) {
	stateMachine->State = FULFILLED;
	stateMachine->Value = value;
}

void StateMachineReject(StateMachine* stateMachine, Value* value) {
	stateMachine->State = REJECTED;
	stateMachine->Value = value;
}

void StateMachineAddWaitList(StateMachine* stateMachine, Value* value) {
	stateMachine->WaitList[stateMachine->WaitListC++] = value;
	stateMachine->WaitList =
		Reallocate(stateMachine->WaitList,
				   sizeof(Value*) * (stateMachine->WaitListC + 1));
	stateMachine->WaitList[stateMachine->WaitListC] =
		NULL;  // keep NULL-terminated
}

void FreeStateMachine(StateMachine* sm) {
	if (sm->WaitList != NULL)
		free(sm->WaitList);
	free(sm);
}