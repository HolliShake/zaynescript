#include "./statemachine.h"

StateMachine* CreateStateMachine(StateMachineState initial, bool isCallback, size_t ip, Value* env, Value* promise, Value* function) {
    StateMachine* sm = Allocate(sizeof(StateMachine));
    sm->State        = initial;
    sm->IsCallback   = isCallback;
    sm->CallEnv      = env;
    sm->WaitFor      = promise;
    sm->Value        = NULL;
    sm->Function     = function;
    sm->Ip           = ip;
    sm->WaitListC    = 0;
    sm->WaitList     = Allocate(sizeof(Value*)), sm->WaitList[0] = NULL; // initial capacity for wait list
    return sm;
}

void StateMachineSet(StateMachine* stateMachine, StateMachineState newState, size_t ip, Value* env,  Value* waitFor, Value* value) {
    stateMachine->State   = newState;
    stateMachine->Ip      = ip;
    stateMachine->CallEnv = env;
    stateMachine->WaitFor = waitFor;
    stateMachine->Value   = value;
}

void StateMachineAddWaitList(StateMachine* stateMachine, Value* value) {
    stateMachine->WaitList[stateMachine->WaitListC++] = value;
    stateMachine->WaitList = Reallocate(stateMachine->WaitList, sizeof(Value*) * (stateMachine->WaitListC + 1));
    stateMachine->WaitList[stateMachine->WaitListC] = NULL; // keep NULL-terminated
}


void FreeStateMachine(StateMachine* sm) {
    free(sm->WaitList);
    free(sm);
}