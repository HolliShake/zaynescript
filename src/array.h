#include "./global.h"

#ifndef ARRAY_H
#    define ARRAY_H

/**
 * CreateArray - Creates a new dynamic array
 *
 * Return: Pointer to the newly created Array structure, or NULL on failure
 */
Array* CreateArray();

/**
 * ArrayPush - Pushes a value to the end of the array
 * @array: Pointer to the Array structure
 * @value: Value to push into the array
 *
 * Return: void
 */
void ArrayPush(Array* array, void* value);

/**
 * ArrayPop - Pops a value from the end of the array
 * @array: Pointer to the Array structure
 *
 * Return: Popped value from the array, or NULL if array is empty
 */
void* ArrayPop(Array* array);

/**
 * ArrayPeek - Peeks at the last value in the array without removing it
 * @array: Pointer to the Array structure
 *
 * Return: Last value in the array, or NULL if array is empty
 */
void* ArrayPeek(Array* array);

/**
 * ArraySet - Sets a value at a specific index in the array
 * @array: Pointer to the Array structure
 * @index: Index at which to set the value
 * @value: Value to set at the specified index
 *
 * Return: The value set at the specified index
 */
void* ArraySet(Array* array, size_t index, void* value);

/**
 * ArrayGet - Gets a value at a specific index in the array
 * @array: Pointer to the Array structure
 * @index: Index from which to get the value
 *
 * Return: Value at the specified index, or NULL if index is out of bounds
 */
void* ArrayGet(Array* array, size_t index);

/**
 * ArrayLength - Returns the length of the array
 * @array: Pointer to the Array structure
 *
 * Return: Length of the array
 */
size_t ArrayLength(Array* array);

/**
 * ArrayExtend - Extends the array by appending all elements from another array
 * @array: Pointer to the Array structure to extend
 * @other: Pointer to the Array structure whose elements will be appended
 *
 * Return: void
 */
void ArrayExtend(Array* array, Array* other);

/**
 * ArrayToString - Converts the array to a string representation
 * @array: Pointer to the Array structure
 *
 * Return: String representation of the array
 */
String ArrayToString(Array* array);

#endif