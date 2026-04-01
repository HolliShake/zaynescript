/**
 * @file environment.h
 * @brief Execution environment (variable scope) interface.
 *
 * An Environment holds the local variable cells for a single function or
 * block scope. Environments are linked through a parent chain to implement
 * lexical scoping. EnvCells can be shared across closures via reference
 * counting and the IsCaptured flag.
 */

#include "./global.h"

#ifndef ENVIRONMENT_H
#	define ENVIRONMENT_H

/**
 * @brief Creates a new environment cell wrapping a value.
 *
 * Allocates an EnvCell that holds a reference to the given value. Cells
 * are used as the individual variable slots within an Environment.
 *
 * @param value Pointer to the Value to store in the environment cell.
 * @return Pointer to the newly created EnvCell structure, or NULL on
 * allocation failure.
 */
EnvCell* CreateEnvCell(Value* value);

/**
 * @brief Creates a new execution environment.
 *
 * Allocates an Environment with the specified number of local variable
 * slots and links it to the given parent environment.
 *
 * @param parent Pointer to the parent environment Value (for lexical scoping),
 * or NULL for the root environment.
 * @param localC Number of local variable slots to allocate for this
 * environment.
 * @return Pointer to the newly created Environment structure, or NULL on
 * allocation failure.
 */
Environment* CreateEnvironment(Value* parent, int localC);

/**
 * @brief Stores a value in a specific local variable slot.
 *
 * Writes the given value into the environment cell at the specified offset.
 * The offset must be within the bounds of the environment's local count.
 *
 * @param environment Pointer to the Environment to modify.
 * @param offset Zero-based index of the local variable slot to set.
 * @param value Pointer to the Value to store in the slot.
 */
void EnvironmentSetLocal(Environment* environment, int offset, Value* value);

/**
 * @brief Retrieves the environment cell at a specific local variable slot.
 *
 * Returns the EnvCell at the given offset, which holds the current value
 * and capture metadata for that variable slot.
 *
 * @param environment Pointer to the Environment to query.
 * @param offset Zero-based index of the local variable slot to retrieve.
 * @return Pointer to the EnvCell at the given offset.
 */
EnvCell* EnvironmentGetLocal(Environment* environment, int offset);

/**
 * @brief Clones an environment from an Environment value.
 *
 * Creates a shallow copy of the Environment contained in the given Value.
 * The cloned environment shares the same parent but has independent local
 * variable cells, suitable for creating independent closure snapshots.
 *
 * @param envValue Pointer to the Value wrapping the Environment to clone.
 * @return Pointer to the newly cloned Environment structure, or NULL on
 * allocation failure.
 */
Environment* EnvironmentCloneFromValue(Value* envValue);

/**
 * @brief Synchronizes the local variable cells of one environment into
 * another.
 *
 * Copies references to the local cells from the source environment into
 * the matching slots of the destination environment. This is used to
 * propagate captured variable mutations back to their owning environment
 * after a closure call.
 *
 * @param src Pointer to the source Environment to synchronize from.
 * @param dst Pointer to the destination Environment to synchronize into.
 */
void EnvironmentSync(Environment* src, Environment* dst);

/**
 * @brief Releases all memory associated with an environment.
 *
 * Frees the Environment structure and its local variable cell array.
 * Individual EnvCell values are not freed here; they are managed by the
 * garbage collector.
 *
 * @param environment Pointer to the Environment to free.
 */
void FreeEnvironment(Environment* environment);

#endif