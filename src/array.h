/**
 * @file array.h
 * @brief Generic dynamic array (resizable array) interface.
 *
 * Provides a dynamically resizable array that stores generic void pointers.
 * Supports standard operations: push, pop, peek, indexed get/set, extend,
 * and string conversion.
 */

#include "./global.h"

#ifndef ARRAY_H
#	define ARRAY_H

/**
 * @brief Creates a new empty dynamic array.
 *
 * Allocates and initializes an Array structure with zero items and a default
 * initial capacity.
 *
 * @return Pointer to the newly created Array structure, or NULL on allocation
 * failure.
 */
Array* CreateArray();

/**
 * @brief Appends a value to the end of the array.
 *
 * Grows the array's internal buffer if necessary before inserting the value.
 *
 * @param array Pointer to the Array structure.
 * @param value Pointer to the value to append (any pointer type).
 */
void ArrayPush(Array* array, void* value);

/**
 * @brief Removes and returns the last value in the array.
 *
 * Decrements the item count and returns the value that was at the end.
 * Does not shrink the underlying buffer.
 *
 * @param array Pointer to the Array structure.
 * @return Pointer to the removed value, or NULL if the array is empty.
 */
void* ArrayPop(Array* array);

/**
 * @brief Returns the last value in the array without removing it.
 *
 * Provides read access to the top-of-stack element without modifying the
 * array.
 *
 * @param array Pointer to the Array structure.
 * @return Pointer to the last value, or NULL if the array is empty.
 */
void* ArrayPeek(Array* array);

/**
 * @brief Overwrites the value at a specific index in the array.
 *
 * Sets the element at the given index to the provided value. The index
 * must be within the current bounds of the array.
 *
 * @param array Pointer to the Array structure.
 * @param index Zero-based index of the element to overwrite.
 * @param value Pointer to the new value to store at the index.
 * @return Pointer to the newly stored value.
 */
void* ArraySet(Array* array, size_t index, void* value);

/**
 * @brief Retrieves the value at a specific index in the array.
 *
 * Performs a bounds-checked lookup and returns the element pointer stored
 * at the given index.
 *
 * @param array Pointer to the Array structure.
 * @param index Zero-based index of the element to retrieve.
 * @return Pointer to the value at the specified index, or NULL if the index
 * is out of bounds.
 */
void* ArrayGet(Array* array, size_t index);

/**
 * @brief Returns the number of elements currently in the array.
 *
 * @param array Pointer to the Array structure.
 * @return Current element count of the array.
 */
size_t ArrayLength(Array* array);

/**
 * @brief Appends all elements of another array to this array.
 *
 * Iterates over the source array and pushes each element onto the
 * destination array in order.
 *
 * @param array Pointer to the destination Array structure to extend.
 * @param other Pointer to the source Array structure whose elements will be
 * appended.
 */
void ArrayExtend(Array* array, Array* other);

/**
 * @brief Converts the array to a human-readable string representation.
 *
 * Produces a formatted string listing the array's elements, suitable for
 * debugging or display.
 *
 * @param array Pointer to the Array structure.
 * @return Newly allocated string representation of the array. The caller is
 * responsible for freeing this string.
 */
String ArrayToString(Array* array);

#endif