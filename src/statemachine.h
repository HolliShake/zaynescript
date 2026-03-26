#include "./global.h"

#ifndef STATEMACHINE_H
#define STATEMACHINE_H


StateMachine* CreateStateMachine(StateMachineState initial, bool isCallback, size_t ip, Value* env, Value* waitFor, Value* function);

void StateMachineSet(StateMachine* stateMachine, StateMachineState newState, size_t ip, Value* env,  Value* waitFor, Value* value);

void StateMachineAddWaitList(StateMachine* stateMachine, Value* value);

void FreeStateMachine(StateMachine* sm);

#endif