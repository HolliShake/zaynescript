#include "./global.h"
#include "./hashmap.h"
#include "./function.h"

#ifndef VALUE_H
#define VALUE_H

/**
 * @brief Creates a new 32-bit integer value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param value The integer value to store
 * @return Pointer to newly allocated Value structure containing the integer
 */
Value* NewIntValue(Interpreter* interpreter, int value);

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
Value* NewBoolValue(Interpreter* interpreter, int value);

/**
 * @brief Creates a new null value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @return Pointer to newly allocated Value structure representing null
 */
Value* NewNullValue(Interpreter* interpreter);

/**
 * @brief Converts a value to a string representation
 * 
 * @param value Pointer to the Value to convert
 * @return String representation of the value
 */
String ValueToString(Value* value);

/**
 * @brief Checks if a value is an integer type
 * 
 * @param value Pointer to the Value to check
 * @return Non-zero if the value is an integer, 0 otherwise
 */
int ValueIsInt(Value* value);

/**
 * @brief Checks if a value is a numeric (double) type
 * 
 * @param value Pointer to the Value to check
 * @return Non-zero if the value is a number, 0 otherwise
 */
int ValueIsNum(Value* value);

/**
 * @brief Checks if a value is a string type
 * 
 * @param value Pointer to the Value to check
 * @return Non-zero if the value is a string, 0 otherwise
 */
int ValueIsStr(Value* value);

/**
 * @brief Checks if a value is a boolean type
 * 
 * @param value Pointer to the Value to check
 * @return Non-zero if the value is a boolean, 0 otherwise
 */
int ValueIsBool(Value* value);

/**
 * @brief Checks if a value is null
 * 
 * @param value Pointer to the Value to check
 * @return Non-zero if the value is null, 0 otherwise
 */
int ValueIsNull(Value* value);

#endif