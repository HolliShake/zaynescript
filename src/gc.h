/*
 * gc.h - Garbage collection interface for the interpreter
 *
 * This header defines the garbage collection API used to manage memory
 * and reclaim unused objects in the interpreter.
 */

#include "./environment.h"
#include "./function.h"
#include "./global.h"
#include "./hashmap.h"
#include "./statemachine.h"

#ifndef GC_H
#    define GC_H

/*
 * Mark - Marks a value as reachable during garbage collection
 * @value: Pointer to the value to mark. Must not be NULL.
 *
 * This function marks a value and its dependencies as reachable, preventing
 * them from being collected during the mark-and-sweep garbage collection phase.
 */
void Mark(Value* value);

/*
 * GarbageCollect - Performs garbage collection on the interpreter
 * @interpreter: Pointer to the interpreter instance. Must not be NULL.
 *
 * Executes a garbage collection cycle to reclaim memory from unreachable
 * objects. This is typically called automatically when memory pressure
 * reaches a threshold.
 */
void GarbageCollect(Interpreter* interpreter);

/*
 * ForceGarbageCollect - Forces an immediate garbage collection cycle
 * @interpreter: Pointer to the interpreter instance. Must not be NULL.
 *
 * Triggers garbage collection regardless of current memory pressure or
 * allocation thresholds. This is useful for testing or when deterministic
 * cleanup is required.
 */
void ForceGarbageCollect(Interpreter* interpreter);


#endif /* GC_H */