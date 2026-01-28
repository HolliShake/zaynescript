#include "./core/loader.h"
#include "./environment.h"
#include "./global.h"
#include "./gc.h"
#include "./value.h"


#ifndef OPERATION_H
#define OPERATION_H

#define FLG_ARG_MISMATCH      -6
#define FLG_ERROR             -1
#define FLG_INVALID_OPERATION -4
#define FLG_NOTFOUND          -2
#define FLG_OUT_OF_BOUNDS     -5
#define FLG_SUCCESS            0
#define FLG_ZERO_DIV          -3

/**
 * Checks if a method exists on an object.
 * 
 * @param interp      The interpreter instance
 * @param obj         The object to check
 * @param method      The method name to check for
 * 
 * @return true if the method exists, false otherwise
 */
bool IsMethodOfObject(Interpreter* interp, Value* obj, Value* method);

/**
 * Generic attribute retrieval function.
 * 
 * @param interp        The interpreter instance
 * @param obj           The object to retrieve the attribute from
 * @param attr          The attribute/key to retrieve
 * @param forMethodCall Whether the attribute is being retrieved for a method call
 * 
 * @return The retrieved attribute value, or Null value if not found
 */
Value* GenericGetAttribute(Interpreter* interp, Value* obj, Value* attr, bool forMethodCall);

/**
 * Performs import core operation.
 * 
 * @param interp      The interpreter instance
 * @param moduleName  The name of the module to import
 * @param out         Output parameter for the result (if not NULL, result is stored here)
 * 
 * @return Offset in constants array, or error flag (FLG_NOTFOUND)
 */
int DoImportCore(Interpreter* interp, String moduleName, Value** out);

/**
 * Sets an index on an object.
 * 
 * @param interp The interpreter instance
 * @param obj    The object to set the index on
 * @param index  The index/key to set
 * @param val    The value to set
 * 
 * @return FLG_SUCCESS, or error flag (FLG_INVALID_OPERATION, FLG_OUT_OF_BOUNDS)
 */
int DoSetIndex(Interpreter* interp, Value* obj, Value* index, Value* val);

/**
 * Retrieves an attribute from an object.
 * 
 * @param interp        The interpreter instance
 * @param obj           The object to retrieve the attribute from
 * @param index         The index/key to retrieve
 * @param out           Output parameter for the result (if not NULL, result is stored here)
 * 
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoGetIndex(Interpreter* interp, Value* obj, Value* index, Value** out);

/**
 * Performs constructor call operation.
 * 
 * @param interp      The interpreter instance
 * @param rootEnvObj  The root environment object
 * @param envObj      The current environment object
 * @param clsValue    The class to instantiate
 * @param argc        Number of arguments
 * 
 * @return Offset in constants array, or error flag (FLG_ARG_MISMATCH)
 */
int DoCallCtor(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* clsValue, int argc);

/**
 * Performs function call operation.
 * 
 * @param interp      The interpreter instance
 * @param rootEnvObj  The root environment object
 * @param envObj      The current environment object
 * @param fn          The function to call
 * @param argc        Number of arguments
 * 
 * @return Offset in constants array, or error flag (FLG_ARG_MISMATCH)
 */
int DoCall(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* fn, int argc);

/**
 * Performs method call operation.
 * 
 * @param interp      The interpreter instance
 * @param rootEnvObj  The root environment object
 * @param envObj      The current environment object
 * @param obj         The object on which the method is called
 * @param methodName  The name of the method to call
 * @param argc        Number of arguments
 * 
 * @return Offset in constants array, or error flag (FLG_ARG_MISMATCH)
 */
int DoCallMethod(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* obj, Value* methodName, int argc);

/**
 * Performs logical NOT operation on a value.
 * 
 * @param interp The interpreter instance
 * @param val    The value to perform the logical NOT operation on
 * @param out    Output parameter for the result (if not NULL, result is stored here)
 * 
 * @return FLG_SUCCESS
 */
int DoNot(Interpreter* interp, Value* val, Value** out);

/**
 * Performs unary plus operation on a value.
 * 
 * @param interp The interpreter instance
 * @param val    The value to perform the unary plus operation on
 * @param out    Output parameter for the result (if not NULL, result is stored here)
 * 
 * @return FLG_SUCCESS
 */
int DoPos(Interpreter* interp, Value* val, Value** out);

/**
 * Performs unary minus operation on a value.
 * 
 * @param interp The interpreter instance
 * @param val    The value to perform the unary minus operation on
 * @param out    Output parameter for the result (if not NULL, result is stored here)
 * 
 * @return FLG_SUCCESS
 */
int DoNeg(Interpreter* interp, Value* val, Value** out);

/**
 * Performs multiplication operation on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the multiplication
 */
Value* DoMul(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs division operation on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the division, or error value
 */
Value* DoDiv(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs modulo operation on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the modulo operation, or error value
 */
Value* DoMod(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs increment operation on a value.
 * 
 * @param interp The interpreter instance
 * @param val    The value to increment
 * 
 * @return Resulting value of the increment operation, or error value
 */
Value* DoInc(Interpreter* interp, Value* val);

/**
 * Performs addition operation on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the addition, or error value
 */
Value* DoAdd(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs decrement operation on a value.
 * 
 * @param interp The interpreter instance
 * @param val    The value to decrement
 * 
 * @return Resulting value of the decrement operation, or error value
 */
Value* DoDec(Interpreter* interp, Value* val);

/**
 * Performs subtraction operation on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the subtraction, or error value
 */
Value* DoSub(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs left shift operation on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the left shift operation, or error value
 */
Value* DoLShift(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs right shift operation on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the right shift operation, or error value
 */
Value* DoRShift(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs less than comparison on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Boolean value (True or False), or error value
 */
Value* DoLT(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs less than or equal to comparison on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Boolean value (True or False), or error value
 */
Value* DoLTE(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs greater than comparison on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Boolean value (True or False), or error value
 */
Value* DoGT(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs greater than or equal to comparison on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Boolean value (True or False), or error value
 */
Value* DoGTE(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs equality comparison on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Boolean value (True or False)
 */
Value* DoEQ(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs inequality comparison on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Boolean value (True or False)
 */
Value* DoNE(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs bitwise AND operation on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the bitwise AND operation, or error value
 */
Value* DoAnd(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs bitwise OR operation on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the bitwise OR operation, or error value
 */
Value* DoOr(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs bitwise XOR operation on two values.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the bitwise XOR operation, or error value
 */
Value* DoXor(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Loads a function from the interpreter's functions array.
 * 
 * @param interp     The interpreter instance
 * @param rootEnvObj The root environment object
 * @param envObj     The local environment object
 * @param offset     The offset of the function in the functions array
 * @param closure    Whether to create a closure (clone) of the function
 * @param out        Output parameter for the result (if not NULL, result is stored here)
 * 
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION)
 */
int DoLoadFunction(Interpreter* interp, Value* rootEnvObj, Value* envObj, int offset, bool closure, Value** out);

#endif