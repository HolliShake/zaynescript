#include "./core/loader.h"
#include "./environment.h"
#include "./global.h"
#include "./gc.h"
#include "./value.h"


#ifndef OPERATION_H
#define OPERATION_H

/**
 * Checks if a method exists on an object.
 * Searches through arrays, objects, classes, and class instances
 * by checking their prototype chains.
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
 * Handles arrays (by index or prototype), objects (by key or prototype),
 * classes (static members), and class instances (instance members or prototype).
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
 * Loads a core module by name using LoadCoreModule.
 * 
 * @param interp      The interpreter instance
 * @param moduleName  The name of the module to import
 * 
 * @return Module value, or error value if not found
 */
Value* DoImportCore(Interpreter* interp, String moduleName);

/**
 * Sets an index on an object.
 * Supports arrays (by numeric index), objects (by key),
 * class instances (member), and classes (static member).
 * 
 * @param interp The interpreter instance
 * @param obj    The object to set the index on
 * @param index  The index/key to set
 * @param val    The value to set
 * 
 * @return Null value on success, or error value on failure
 */
Value* DoSetIndex(Interpreter* interp, Value* obj, Value* index, Value* val);

/**
 * Retrieves an attribute from an object.
 * Delegates to GenericGetAttribute with forMethodCall=false.
 * 
 * @param interp        The interpreter instance
 * @param obj           The object to retrieve the attribute from
 * @param index         The index/key to retrieve
 * 
 * @return The retrieved attribute value
 */
Value* DoGetIndex(Interpreter* interp, Value* obj, Value* index);

/**
 * Performs constructor call operation.
 * Creates a new class instance and calls the constructor if present.
 * If no constructor exists, expects 0 arguments.
 * 
 * @param interp      The interpreter instance
 * @param rootEnvObj  The root environment object
 * @param envObj      The current environment object
 * @param clsValue    The class to instantiate
 * @param argc        Number of arguments
 * 
 * @return Null value on success, or error value on failure
 */
Value* DoCallCtor(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* clsValue, int argc);

/**
 * Performs function call operation.
 * Handles both native functions and user-defined functions.
 * Validates argument count and creates appropriate environment for execution.
 * 
 * @param interp      The interpreter instance
 * @param rootEnvObj  The root environment object
 * @param envObj      The current environment object
 * @param fn          The function to call
 * @param argc        Number of arguments
 * 
 * @return Null value on success, or error value on failure
 */
Value* DoCall(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* fn, int argc);

/**
 * Performs method call operation.
 * Retrieves the method from the object and calls it.
 * Automatically handles 'this' argument for method calls.
 * 
 * @param interp      The interpreter instance
 * @param rootEnvObj  The root environment object
 * @param envObj      The current environment object
 * @param obj         The object on which the method is called
 * @param methodName  The name of the method to call
 * @param argc        Number of arguments
 * 
 * @return Null value on success, or error value on failure
 */
Value* DoCallMethod(Interpreter* interp, Value* rootEnvObj, Value* envObj, Value* obj, Value* methodName, int argc);

/**
 * Performs logical NOT operation on a value.
 * Coerces the value to boolean and negates it.
 * 
 * @param interp The interpreter instance
 * @param val    The value to perform the logical NOT operation on
 * @param out    Output parameter for the result
 * 
 * @return FLG_SUCCESS
 */
Value* DoNot(Interpreter* interp, Value* val);

/**
 * Performs unary plus operation on a value.
 * Returns the value unchanged.
 * 
 * @param interp The interpreter instance
 * @param val    The value to perform the unary plus operation on
 * @param out    Output parameter for the result
 * 
 * @return FLG_SUCCESS
 */
Value* DoPos(Interpreter* interp, Value* val);

/**
 * Performs unary minus operation on a value.
 * Returns the value unchanged (note: implementation may need review).
 * 
 * @param interp The interpreter instance
 * @param val    The value to perform the unary minus operation on
 * @param out    Output parameter for the result
 * 
 * @return FLG_SUCCESS
 */
Value* DoNeg(Interpreter* interp, Value* val);

/**
 * Performs multiplication operation on two values.
 * Supports integer and numeric operands.
 * Returns int if result fits in int range, otherwise returns num.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the multiplication, or error value for invalid operands
 */
Value* DoMul(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs division operation on two values.
 * Supports integer and numeric operands.
 * Returns error for division by zero.
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
 * Supports integer and numeric operands.
 * Returns error for modulo by zero.
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
 * Supports integer and numeric operands.
 * 
 * @param interp The interpreter instance
 * @param val    The value to increment
 * 
 * @return Resulting value of the increment operation, or error value for invalid operand
 */
Value* DoInc(Interpreter* interp, Value* val);

/**
 * Performs addition operation on two values.
 * Supports integer, numeric, and string operands.
 * String operands are concatenated.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the addition, or error value for invalid operands
 */
Value* DoAdd(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs decrement operation on a value.
 * Supports integer and numeric operands.
 * 
 * @param interp The interpreter instance
 * @param val    The value to decrement
 * 
 * @return Resulting value of the decrement operation, or error value for invalid operand
 */
Value* DoDec(Interpreter* interp, Value* val);

/**
 * Performs subtraction operation on two values.
 * Supports integer and numeric operands.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the subtraction, or error value for invalid operands
 */
Value* DoSub(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs left shift operation on two values.
 * Supports numeric operands (coerced to integers).
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the left shift operation, or error value for invalid operands
 */
Value* DoLShift(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs right shift operation on two values.
 * Supports numeric operands (coerced to integers).
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the right shift operation, or error value for invalid operands
 */
Value* DoRShift(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs less than comparison on two values.
 * Supports numeric operands.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Boolean value (True or False), or error value for invalid operands
 */
Value* DoLT(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs less than or equal to comparison on two values.
 * Supports numeric operands.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Boolean value (True or False), or error value for invalid operands
 */
Value* DoLTE(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs greater than comparison on two values.
 * Supports numeric operands.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Boolean value (True or False), or error value for invalid operands
 */
Value* DoGT(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs greater than or equal to comparison on two values.
 * Supports numeric operands.
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Boolean value (True or False), or error value for invalid operands
 */
Value* DoGTE(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs equality comparison on two values.
 * Uses ValueIsEqual for comparison.
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
 * Uses ValueIsEqual for comparison and negates the result.
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
 * Supports integer and numeric operands (coerced to integers).
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the bitwise AND operation, or error value for invalid operands
 */
Value* DoAnd(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs bitwise OR operation on two values.
 * Supports integer and numeric operands (coerced to integers).
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the bitwise OR operation, or error value for invalid operands
 */
Value* DoOr(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Performs bitwise XOR operation on two values.
 * Supports integer and numeric operands (coerced to integers).
 * 
 * @param interp The interpreter instance
 * @param lhs    Left-hand side value
 * @param rhs    Right-hand side value
 * 
 * @return Resulting value of the bitwise XOR operation, or error value for invalid operands
 */
Value* DoXor(Interpreter* interp, Value* lhs, Value* rhs);

/**
 * Loads a function from the interpreter's functions array.
 * If closure is true, clones the function.
 * Sets up captures from root and local environments.
 * 
 * @param interp     The interpreter instance
 * @param rootEnvObj The root environment object
 * @param envObj     The local environment object
 * @param offset     The offset of the function in the functions array
 * @param closure    Whether to create a closure (clone) of the function
 * 
 * @return The loaded function value
 */
Value* DoLoadFunction(Interpreter* interp, Value* rootEnvObj, Value* envObj, int offset, bool closure);

#endif