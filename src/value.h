#include "./global.h"

#ifndef VALUE_H
#define VALUE_H

/**
 * @brief Creates a new value of the specified type
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param type The type of value to create
 * @return Pointer to newly allocated Value structure
 */
Value* CreateValue(Interpreter* interpreter, ValueType type);

/**
 * @brief Creates a new 32-bit integer value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param value The integer value to store
 * @return Pointer to newly allocated Value structure containing the integer
 */
Value* NewI32Value(Interpreter* interpreter, int value);

/**
 * @brief Creates a new double-precision floating point number value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param value The numeric value to store
 * @return Pointer to newly allocated Value structure containing the number
 */
Value* NewNumValue(Interpreter* interpreter, double value);

/**
 * @brief Creates a new string value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param value The string to store
 * @return Pointer to newly allocated Value structure containing the string
 */
Value* NewStrValue(Interpreter* interpreter, String value);

/**
 * @brief Creates a new boolean value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param value The boolean value to store
 * @return Pointer to newly allocated Value structure containing the boolean
 */
Value* NewBoolValue(Interpreter* interpreter, bool value);

/**
 * @brief Creates a new null value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @return Pointer to newly allocated Value structure representing null
 */
Value* NewNullValue(Interpreter* interpreter);

#endif