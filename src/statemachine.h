/**
 * @file statemachine.h
 * @brief State machine implementation for handling asynchronous
 * operations (e.g., promises).
 *
 * This header defines the StateMachine structure and related
 * functions for managing the state of asynchronous operations,
 * including pending, fulfilled, and rejected states.
 */

#include "./global.h"

#ifndef STATEMACHINE_H
#	define STATEMACHINE_H

/**
 * @brief Creates a new StateMachine instance.
 *
 * Allocates and initializes a StateMachine with the given
 * initial state and execution context. The state machine is used
 * to track the lifecycle of asynchronous operations such as
 * promises.
 *
 * @param initial    The initial state of the state machine
 * (e.g., PENDING).
 * @param isCallback Whether this state machine is a callback
 * (e.g., then/catch handler).
 * @param waitFor    The value this state machine is waiting on
 * (e.g., a promise), or NULL.
 * @param function   The function being executed by this state
 * machine.
 * @return Pointer to a newly allocated StateMachine, or NULL on
 * allocation failure.
 */
Promise* CreatePromise(PromiseState initial,
					   CallFrame*	suspendedCallFrame,
					   Value*		parent,
					   Value*		globals,
					   Value*		callback);

/**
 * @brief Adds a reaction to the Promise.
 *
 * This function adds a reaction to the Promise, which is used to
 * track the reactions to the Promise.
 *
 * @param promise Pointer to the Promise instance.
 * @param promiseValue The value to add to the reaction.
 */
void PromiseAddReaction(Promise* promise, Value* promiseValue);

/**
 * @brief Sets the Promise to an awaiting state with a
 * specified waitFor value.
 *
 * This function transitions the StateMachine to a new state
 * (e.g., pending) and sets the waitFor field to indicate that
 * the state machine is now waiting on a specific value (e.g., a
 * promise). This is typically called when an asynchronous
 * operation is initiated and the state machine needs to wait for
 * its completion.
 *
 * @param stateMachine Pointer to the StateMachine instance to
 * update.
 * @param value The value that the StateMachine is waiting for
 * (e.g., a promise).
 */
void PromiseAwait(Promise* promise, Value* promiseValue);

/**
 * @brief Transitions the StateMachine to a fulfilled state with
 * a specified value.
 *
 * This function updates the StateMachine's state to fulfilled
 * and sets the associated value (e.g., the resolved value of a
 * promise). It is typically called when an asynchronous
 * operation completes successfully and the state machine needs
 * to transition to a fulfilled state.
 *
 * @param stateMachine Pointer to the StateMachine instance to
 * update.
 * @param value The value to set on the StateMachine (e.g.,
 * resolved value).
 */
void PromiseFulfill(Promise* promise, Value* value);

/**
 * @brief Transitions the StateMachine to a rejected state with a
 * reason value.
 *
 * Updates the StateMachine's state to rejected and records the
 * given value as the rejection reason (e.g., an error or
 * exception). Callbacks registered via catch handlers will be
 * scheduled to receive this value.
 *
 * @param stateMachine Pointer to the StateMachine instance to
 * update.
 * @param value The rejection reason value (e.g., an error
 * Value).
 */
void PromiseReject(Promise* promise, Value* value);

/**
 * @brief Frees a StateMachine and its resources.
 *
 * This function deallocates the memory used by a StateMachine
 * instance, including its wait list. It should be called when
 * the StateMachine is no longer needed to prevent memory leaks.
 *
 * @param sm Pointer to the StateMachine to free.
 */
void FreePromise(Promise* promise);

#endif