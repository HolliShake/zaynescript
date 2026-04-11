/**
 * @file gc.h
 * @brief Garbage collection interface for the interpreter.
 *
 * Declares the public garbage collector API: marking, young and
 * major collection cycles, forced collection, and heap teardown.
 * The collector traces reachable values from the interpreter root
 * set and reclaims unreachable memory.
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
 * Recursively traverses the object graph starting from the given
 * value, setting the Marked flag on every reachable Value.
 * Called during the mark phase of garbage collection to prevent
 * live objects from being collected.
 *
 * @param value Pointer to the Value to mark. Must not be NULL.
 */
void Mark(Value* value);

/**
 * @brief Performs a young-generation garbage collection cycle.
 *
 * Marks all values reachable from the interpreter root set,
 * sweeps the young heap to free unreachable objects, promotes
 * survivors, clears marks on the old generation, and resets the
 * allocation threshold for the next young collection. Typically
 * invoked automatically when allocation volume exceeds the young
 * GC threshold.
 *
 * @param interpreter Pointer to the Interpreter instance. Must
 * not be NULL.
 */
void GarbageCollect(Interpreter* interpreter);

/**
 * @brief Performs a major (full) garbage collection cycle.
 *
 * Runs a complete mark-and-sweep over both the young and old
 * generations.  Young survivors are promoted to the old
 * generation; old survivors remain and the old threshold is
 * updated with GC_GROWTH_FACTOR.  Triggered automatically when
 * OldCount reaches OldThreshold.
 *
 * @param interpreter Pointer to the Interpreter instance. Must
 * not be NULL.
 */
void MajorGarbageCollect(Interpreter* interpreter);

/**
 * @brief Forces an immediate major garbage collection cycle
 * regardless of threshold.
 *
 * Triggers a full mark-and-sweep cycle unconditionally,
 * bypassing the allocation threshold check. Useful for testing,
 * deterministic cleanup, or situations where memory must be
 * reclaimed immediately.
 *
 * @param interpreter Pointer to the Interpreter instance. Must
 * not be NULL.
 */
void ForceGarbageCollect(Interpreter* interpreter);

/**
 * @brief Clears interpreter-owned roots and frees every
 * GC-tracked Value on the young and old heaps.
 *
 * Used on interpreter shutdown. Runs before @ref bf_context_end
 * so big-number values can still use the bf context during
 * teardown.
 *
 * @param interpreter Pointer to the Interpreter instance whose
 * heaps are drained and freed.
 */
void GcDestroyHeap(Interpreter* interpreter);

#endif /* GC_H */