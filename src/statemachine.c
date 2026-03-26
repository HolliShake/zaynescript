#include "./statemachine.h"

StateMachine* CreateStateMachine(StateMachineState initial, size_t ip, size_t stack, Value* env, Value* promise, Value* function, Value* thenCallback, Value* catchCallback) {
    StateMachine* sm = Allocate(sizeof(StateMachine));
    sm->State        = initial;
    sm->CallEnv      = env;
    sm->WaitFor      = promise;
    sm->Value        = NULL;
    sm->Then         = thenCallback;
    sm->Catch        = catchCallback;
    sm->Function     = function;
    sm->Ip           = ip;
    sm->Stack        = stack;
    sm->Awaited      = false;
    return sm;
}


void StateMachineSet(StateMachine* stateMachine, StateMachineState newState, size_t ip, Value* env,  Value* waitFor, Value* value) {
    stateMachine->State   = newState;
    stateMachine->Ip      = ip;
    stateMachine->CallEnv = env;
    stateMachine->WaitFor = waitFor;
    stateMachine->Value   = value;
}