#include "./global.h"

#ifndef STATEMACHINE_H
#define STATEMACHINE_H


StateMachine* CreateStateMachine(StateMachineState initial, size_t ip, size_t stack, Value* env, Value* waitFor, Value* function, Value* thenCallback, Value* catchCallback);

void StateMachineSet(StateMachine* stateMachine, StateMachineState newState, size_t ip, Value* env,  Value* waitFor, Value* value);

void StateMachineAddWaitList(StateMachine* stateMachine, Value* value);

#endif