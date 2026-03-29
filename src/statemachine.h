#include "./global.h"

#ifndef STATEMACHINE_H
#    define STATEMACHINE_H

/**
 * @file statemachine.h
 * @brief State machine implementation for handling asynchronous operations (e.g., promises).
 *
 * This header defines the StateMachine structure and related functions for managing
 * the state of asynchronous operations, including pending, fulfilled, and rejected states.
 */

/**
 * @brief Creates a new StateMachine instance.
 *
 * Allocates and initializes a StateMachine with the given initial state and
 * execution context. The state machine is used to track the lifecycle of
 * asynchronous operations such as promises.
 *
 * @param initial    The initial state of the state machine (e.g., PENDING).
 * @param isCallback Whether this state machine is a callback (e.g., then/catch handler).
 * @param ip         The instruction pointer for execution within the state machine.
 * @param env        The environment value associated with this state machine.
 * @param waitFor    The value this state machine is waiting on (e.g., a promise), or NULL.
 * @param function   The function being executed by this state machine.
 * @return Pointer to a newly allocated StateMachine, or NULL on allocation failure.
 */
StateMachine* CreateStateMachine(StateMachineState initial,
                                 bool              isCallback,
                                 size_t            ip,
                                 Value*            env,
                                 Value*            waitFor,
                                 Value*            function);

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
void StateMachineSet(StateMachine*     stateMachine,
                     StateMachineState newState,
                     size_t            ip,
                     Value*            env,
                     Value*            waitFor,
                     Value*            value);

/**
 * @brief Updates the state and value of a StateMachine.
 *
 * This function is a simplified version of StateMachineSet that only updates
 * the state and value of the StateMachine, without modifying the instruction
 * pointer, environment, or waitFor fields. It is useful for quickly transitioning
 * the state machine to a new state with an associated value.
 *
 * @param stateMachine Pointer to the StateMachine instance to update.
 * @param newState The new state to set for the StateMachine.
 * @param value The value to set on the StateMachine (e.g., resolved value or rejection reason).
 */
void StateMachineUpdate(StateMachine* stateMachine, StateMachineState newState, Value* value);

/**
 * @brief Sets the StateMachine to an awaiting state with a specified waitFor value.
 *
 * This function transitions the StateMachine to a new state (e.g., pending) and
 * sets the waitFor field to indicate that the state machine is now waiting on a
 * specific value (e.g., a promise). This is typically called when an asynchronous
 * operation is initiated and the state machine needs to wait for its completion.
 *
 * @param stateMachine Pointer to the StateMachine instance to update.
 * @param ip The instruction pointer to set for the StateMachine when awaiting.
 * @param value The value that the StateMachine is waiting for (e.g., a promise).
 */
void StateMachineAwait(StateMachine* stateMachine, size_t ip, Value* value);

/**
 * @brief Transitions the StateMachine to a fulfilled state with a specified value.
 *
 * This function updates the StateMachine's state to fulfilled and sets the
 * associated value (e.g., the resolved value of a promise). It is typically
 * called when an asynchronous operation completes successfully and the state
 * machine needs to transition to a fulfilled state.
 *
 * @param stateMachine Pointer to the StateMachine instance to update.
 * @param value The value to set on the StateMachine (e.g., resolved value).
 */
void StateMachineFulfill(StateMachine* stateMachine, Value* value);

/**
 *
 */
void StateMachineReject(StateMachine* stateMachine, Value* value);

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