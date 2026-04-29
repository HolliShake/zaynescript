/**
 * @file array.h
 * @brief Dynamic array implementation for managing sequences of loosely-typed generic pointer values within the interpreter.
 */

#include "./global.h"

#ifndef ARRAY_H
#define ARRAY_H

/**
 * @brief Allocates an empty array backed by an internal buffer initialized to
 *        capacity 4. Memory is allocated via Allocate() which aborts on failure.
 *
 * @return Newly constructed array with 0 items. 
 */
Array* CreateArray();

/**
 * @brief Inserts a value pointer at the end of the backing buffer, doubling
 *        internal capacity (and triggering Reallocate) if the current limit
 *        is reached. 
 *
 * @param array The target list whose capacity and items will be mutated.
 * @param value The untyped heap payload or pointer being tracked. Kept as-is.
 */
void ArrayPush(Array* array, void* value);

/**
 * @brief Decrements the bounds of the array, returning the most recently
 *        pushed pointer. 
 *
 * @param array The sequence to shorten. Buffer size does not shrink; only the active view reduces.
 * @return The unlinked tail value, or NULL if the array has no items left.
 */
void* ArrayPop(Array* array);

/**
 * @brief Looks at the final pushed element in the active sequence bounds.
 *
 * @param array The sequence structure to read from.
 * @return The final value in the current items slice, or NULL if bounds are size 0.
 */
void* ArrayPeek(Array* array);

/**
 * @brief Replaces an existing slot within current sequence limits. Does not perform
 *        any capacity adjustments.
 *
 * @param array The mutated target array structure.
 * @param index The exact numerical offset to target, must strictly reflect an existing element (index < count).
 * @param value The substituting pointer replacing the original item.
 * @return The newly applied pointer payload, or NULL if bounds check fails.
 */
void* ArraySet(Array* array, size_t index, void* value);

/**
 * @brief Locates a stored pointer by offset within the active array view limits.
 *
 * @param array Source sequence for lookup operations.
 * @param index Numerical requested offset.
 * @return The matching referenced item, or NULL if accessed past the recorded Count.
 */
void* ArrayGet(Array* array, size_t index);

/**
 * @brief Exposes the number of active tracked items. Internal buffer bounds
 *        (Capacity) are typically higher than this returned bound limit.
 *
 * @param array Collection to measure.
 * @return Count of actively set items.
 */
size_t ArrayLength(Array* array);

/**
 * @brief Concatenates another array onto the end, calculating the final combined bounds upfront
 *        then iterating and copying element pointers sequentially.
 *
 * @param array Receptacle appending sequence, potentially going through capacity growth.
 * @param other Input slice. Read-only operation on its items buffer.
 */
void ArrayExtend(Array* array, Array* other);

/**
 * @brief Deeply evaluates and combines each element via ValueToString(), encapsulating
 *        the result in `[ ]` brackets and separating by commas. Elements referencing
 *        their own parent array resolve safely to `[self]`.
 * 
 * @param array Collection list traversing to text representation.
 * @return Pre-populated final C-string allocation ready for runtime passing. The sequence caller owns this pointer and must free() it.
 */
String ArrayToString(Array* array);

#endif
