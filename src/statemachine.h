#include "./global.h"

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

/**
 * @file statemachine.h
 * @brief State machine implementation for handling asynchronous operations (e.g., promises).
 * 
 * This header defines the StateMachine structure and related functions for managing
 * the state of asynchronous operations, including pending, fulfilled, and rejected states.
 */
StateMachine* CreateStateMachine(StateMachineState initial, bool isCallback, size_t ip, Value* env, Value* waitFor, Value* function);

/**
 * @brief Sets the state and related information of a StateMachine.
 * 
 * This function updates the state of the StateMachine along with its
 * instruction pointer, environment, waitFor value, and associated function.
 * It is typically called when transitioning the state machine to a new state
 * (e.g., from pending to fulfilled or rejected).
 * 
 * @param stateMachine Pointer to the StateMachine instance to update.
 * @param newState The new state to set for the StateMachine.
 * @param ip The instruction pointer to set for the StateMachine.
 * @param env The environment value to associate with the StateMachine.
 * @param waitFor The value that the StateMachine is waiting for (e.g., a promise).
 * @param value The value to set on the StateMachine (e.g., resolved value or rejection reason).
 */
void StateMachineSet(StateMachine* stateMachine, StateMachineState newState, size_t ip, Value* env,  Value* waitFor, Value* value);

/**
 * @brief Adds a value to the StateMachine's wait list.
 * 
 * This function appends a value to the StateMachine's wait list, which is
 * used to track promises or other asynchronous operations that are waiting
 * for the state machine to resolve. The wait list will be processed when
 * the state machine transitions to a fulfilled or rejected state.
 * 
 * @param stateMachine Pointer to the StateMachine instance.
 * @param value The value to add to the wait list (e.g., a promise).
 */
void StateMachineAddWaitList(StateMachine* stateMachine, Value* value);

/**
 * @brief Frees a StateMachine and its resources.
 * 
 * This function deallocates the memory used by a StateMachine instance,
 * including its wait list. It should be called when the StateMachine is
 * no longer needed to prevent memory leaks.
 * 
 * @param sm Pointer to the StateMachine to free.
 */
void FreeStateMachine(StateMachine* sm);

#endif