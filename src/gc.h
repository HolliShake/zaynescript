/**
 * @file gc.h
 * @brief Garbage collection interface for the interpreter
 *
 * This header defines the garbage collection API used to manage memory
 * and reclaim unused objects in the interpreter.
 */

#include "./global.h"

#ifndef GC_H
#define GC_H

/**
 * @brief Marks a value as reachable during garbage collection
 *
 * This function marks a value and its dependencies as reachable, preventing
 * them from being collected during the mark-and-sweep garbage collection phase.
 *
 * @param value Pointer to the value to mark. Must not be NULL.
 */
void Mark(Value* value);

/**
 * @brief Performs garbage collection on the interpreter
 *
 * Executes a garbage collection cycle to reclaim memory from unreachable
 * objects. This is typically called automatically when memory pressure
 * reaches a threshold.
 *
 * @param interpreter Pointer to the interpreter instance. Must not be NULL.
 */
void GarbageCollect(Interpreter* interpreter);

/**
 * @brief Forces an immediate garbage collection cycle
 *
 * Triggers garbage collection regardless of current memory pressure or
 * allocation thresholds. This is useful for testing or when deterministic
 * cleanup is required.
 *
 * @param interpreter Pointer to the interpreter instance. Must not be NULL.
 */
void ForceGarbageCollect(Interpreter* interpreter);


#endif /* GC_H */