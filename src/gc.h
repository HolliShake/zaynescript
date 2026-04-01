/**
 * @file gc.h
 * @brief Garbage collection interface for the interpreter.
 *
 * Defines the three-function API used to drive the mark-and-sweep garbage
 * collector. The collector traces all reachable values starting from the
 * interpreter's root set and reclaims any unreachable memory.
 */

#include "./environment.h"
#include "./function.h"
#include "./global.h"
#include "./hashmap.h"
#include "./statemachine.h"

#ifndef GC_H
#	define GC_H

/**
 * @brief Marks a value and all values reachable from it as live.
 *
 * Recursively traverses the object graph starting from the given value,
 * setting the Marked flag on every reachable Value. Called during the
 * mark phase of garbage collection to prevent live objects from being
 * collected.
 *
 * @param value Pointer to the Value to mark. Must not be NULL.
 */
void Mark(Value* value);

/**
 * @brief Performs a garbage collection cycle on the interpreter.
 *
 * Runs a full mark-and-sweep cycle: marks all values reachable from the
 * interpreter's root set, then sweeps the heap to free unreachable objects.
 * Typically triggered automatically when the number of allocations exceeds
 * the current GC threshold.
 *
 * @param interpreter Pointer to the Interpreter instance. Must not be NULL.
 */
void GarbageCollect(Interpreter* interpreter);

/**
 * @brief Forces an immediate garbage collection cycle regardless of threshold.
 *
 * Triggers a full mark-and-sweep cycle unconditionally, bypassing the
 * allocation threshold check. Useful for testing, deterministic cleanup,
 * or situations where memory must be reclaimed immediately.
 *
 * @param interpreter Pointer to the Interpreter instance. Must not be NULL.
 */
void ForceGarbageCollect(Interpreter* interpreter);


#endif /* GC_H */