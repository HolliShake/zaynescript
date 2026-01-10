#include "./global.h"

#ifndef ARRAY_H
#define ARRAY_H

/**
 * @brief Creates a new dynamic array
 * 
 * @return Pointer to the newly created Array structure
 */
Array* CreateArray();

/**
 * @brief Pushes a value to the end of the array
 * 
 * @param array Pointer to the Array structure
 * @param value Value to push into the array
 */
void ArrayPush(Array* array, void* value);

/**
 * @brief Pops a value from the end of the array
 * 
 * @param array Pointer to the Array structure
 * @return Popped value from the array
 */
void* ArrayPop(Array* array);

/**
 * @brief Peeks at the last value in the array without removing it
 * 
 * @param array Pointer to the Array structure
 * @return Last value in the array
 */
void* ArrayPeek(Array* array);

/**
 * @brief Sets a value at a specific index in the array
 * 
 * @param array Pointer to the Array structure
 * @param index Index at which to set the value
 * @param value Value to set at the specified index
 * @return The value set at the specified index
 */
void* ArraySet(Array* array, size_t index, void* value);

/**
 * @brief Gets a value at a specific index in the array
 * 
 * @param array Pointer to the Array structure
 * @param index Index from which to get the value
 * @return Value at the specified index
 */
void* ArrayGet(Array* array, size_t index);

/**
 * @brief Returns the length of the array
 * 
 * @param array Pointer to the Array structure
 * @return Length of the array
 */
size_t ArrayLength(Array* array);

/**
 * @brief Extends the array by appending all elements from another array
 * 
 * @param array Pointer to the Array structure to extend
 * @param other Pointer to the Array structure whose elements will be appended
 */
void ArrayExtend(Array* array, Array* other);

/**
 * @brief Converts the array to a string representation
 * 
 * @param array Pointer to the Array structure
 * @return String representation of the array
 */
String ArrayToString(Array* array);

#endif