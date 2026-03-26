#include "./array.h"
#include "./class.h"
#include "./function.h"
#include "./global.h"
#include "./hashmap.h"

#ifndef VALUE_H
#define VALUE_H

/**
 * @enum ValueType
 * @brief Enumeration of possible value types in the interpreter
 */
Value* NewErrorValue(Interpreter* interpreter, String message);

/**
 * @brief Creates a new error value with formatted message
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param fmt Format string for the error message
 * @param ... Additional arguments for formatting the message
 * @return Pointer to newly allocated Value structure containing the error message
 */
Value* NewErrorFValue(Interpreter* interpreter, String fmt, ...);

/**
 * @brief Creates a new 32-bit integer value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param value The integer value to store
 * @return Pointer to newly allocated Value structure containing the integer
 */
Value* NewIntValue(Interpreter* interpreter, int value);

/**
 * @brief Creates a new big integer value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param value The big integer value to store
 * @return Pointer to newly allocated Value structure containing the big integer
 */
Value* NewBigIntValue(Interpreter* interpreter, bf_t* value);

/**
 * @brief Creates a new double-precision floating point number value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param value The numeric value to store
 * @return Pointer to newly allocated Value structure containing the number
 */
Value* NewNumValue(Interpreter* interpreter, double value);

/**
 * @brief Creates a new bignum value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param value The bignum value to store
 * @return Pointer to newly allocated Value structure containing the bignum
 */
Value* NewBigNumValue(Interpreter* interpreter, bf_t* value);

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
 * @brief Creates a new promise value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param stateMachine Pointer to the StateMachine structure representing the promise's execution state
 * @return Pointer to newly allocated Value structure representing a promise
 */
Value* NewPromiseValue(Interpreter* interpreter, StateMachine* stateMachine);

/**
 * @brief Creates a new user function value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param userFunction Pointer to the UserFunction structure to wrap
 * @return Pointer to newly allocated Value structure containing the user function
 */
Value* NewUserFunctionValue(Interpreter* interpreter, UserFunction* userFunction);

/**
 * @brief Creates a new native function value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param nativeFunctionMeta Pointer to the NativeFunction structure to wrap
 * @return Pointer to newly allocated Value structure containing the native function
 */
Value* NewNativeFunctionValue(Interpreter* interpreter, NativeFunction* nativeFunctionMeta);

/**
 * @brief Creates a new environment value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param environment Pointer to the Environment structure to wrap
 * @return Pointer to newly allocated Value structure containing the environment
 */
Value* NewEnvironmentValue(Interpreter* interpreter, Environment* environment);

/**
 * @brief Creates a new array value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @return Pointer to newly allocated Value structure containing the array
 */
Value* NewArrayValue(Interpreter* interpreter);

 /**
  * @brief Creates a new object value
  * 
  * @param interpreter Pointer to the interpreter instance
  * @return Pointer to newly allocated Value structure containing the object
  */
Value* NewObjectValue(Interpreter* interpreter);

/**
 * @brief Creates a new class value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param cls Pointer to the Class structure to wrap
 * @return Pointer to newly allocated Value structure containing the class
 */
Value* NewClassValue(Interpreter* interpreter, Class* cls);

/**
 * @brief Creates a new class instance value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param instance Pointer to the ClassInstance structure to wrap
 * @return Pointer to newly allocated Value structure containing the class instance
 */
Value* NewClassInstanceValue(Interpreter* interpreter, ClassInstance* instance);

/**
 * @brief Converts a value to a string representation
 * 
 * @param value Pointer to the Value to convert
 * @return String representation of the value
 */
String ValueToString(Value* value);

/**
 * @brief Gets the type of a value as a string
 * 
 * @param value Pointer to the Value to check
 * @return String representing the type of the value
 */
String ValueTypeOf(Value* value);

/**
 * @brief Checks if a value is an integer type
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is an integer, false otherwise
 */
bool ValueIsInt(Value* value);

/**
 * @brief Checks if a value is a big integer type
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is a big integer, false otherwise
 */
bool ValueIsBigInt(Value* value);

/**
 * @brief Checks if a value is a numeric (double) type
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is a number, false otherwise
 */
bool ValueIsNum(Value* value);

/**
 * @brief Checks if a value is a big numeric (bignum) type
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is a big number, false otherwise
 */
bool ValueIsBigNum(Value* value);

/**
 * @brief Creates a new string value
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param value The string to store
 * @return Pointer to newly allocated Value structure containing the string
 */
bool ValueIsAnyNum(Value* value);

/**
 * @brief Checks if a value is a string type
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is a string, false otherwise
 */
bool ValueIsStr(Value* value);

/**
 * @brief Checks if a value is a boolean type
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is a boolean, false otherwise
 */
bool ValueIsBool(Value* value);

/**
 * @brief Checks if a value is null
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is null, false otherwise
 */
bool ValueIsNull(Value* value);

/**
 * @brief Checks if a value is an error
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is an error, false otherwise
 */
bool ValueIsError(Value* value);

/**
 * @brief Checks if a value is a user function
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is a user function, false otherwise
 */
bool ValueIsUserFunction(Value* value);

/**
 * @brief Checks if a value is a native function
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is a native function, false otherwise
 */
bool ValueIsNativeFunction(Value* value);

/**
 * @brief Checks if a value is callable
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is callable, false otherwise
 */
bool ValueIsCallable(Value* value);

/**
 * @brief Checks if a value is an array
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is an array, false otherwise
 */
bool ValueIsArray(Value* value);

/**
 * @brief Checks if a value is an object
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is an object, false otherwise
 */
bool ValueIsObject(Value* value);

/**
 * @brief Checks if a value is a class
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is a class, false otherwise
 */
bool ValueIsClass(Value* value);

/**
 * @brief Checks if a value is a class instance
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is a class instance, false otherwise
 */
bool ValueIsClassInstance(Value* value);

/**
 * @brief Checks if a value is a promise
 * 
 * @param value Pointer to the Value to check
 * @return true if the value is a promise, false otherwise
 */
bool ValueIsPromise(Value* value);

/**
 * @brief Compares two values for equality
 * 
 * @param a Pointer to the first Value
 * @param b Pointer to the second Value
 * @return true if the values are equal, false otherwise
 */
bool ValueIsEqual(Value* a, Value* b);

#endif