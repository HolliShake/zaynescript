#include "./core/loader.h"
#include "./environment.h"
#include "./error.h"
#include "./gc.h"
#include "./global.h"
#include "./value.h"


#ifndef OPERATION_H
#    define OPERATION_H

/**
 * @file operation.h
 * @brief Declarations for core operations and utilities used by the interpreter.
 *
 * This file contains function declarations for operations such as method lookup,
 * attribute access, function calls, and other core behaviors of the language.
 */

/**
 * Saves an environment as the root environment on the interpreter's environment stack.
 *
 * @param interp The interpreter instance
 * @param env    The environment value to save as root
 */
void SaveRootEnv(Interpreter* interp, Value* env);

/**
 * Pushes the current environment onto the interpreter's environment stack.
 *
 * @param interp   The interpreter instance
 * @param envObj   The environment value to save
 */
void SaveEnv(Interpreter* interp, Value* envObj);

/**
 * Pops the current environment from the environment stack.
 *
 * @param interp The interpreter instance
 */
void RestoreEnv(Interpreter* interp);

/**
 * Restores the nth environment from the environment stack and synchronizes it with the current
 * CallEnv.
 *
 * @param interp The interpreter instance
 * @param n The index of the environment to restore (0-based)
 */
void RestoreNthEnvAndSync(Interpreter* interp, int n);

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
 * @param index         The attribute/key to retrieve
 * @param forMethodCall Whether the attribute is being retrieved for a method call
 *
 * @return The retrieved attribute value, or Null value if not found
 */
Value* GenericGetAttribute(Interpreter* interp, Value* obj, Value* index, bool forMethodCall);

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
 * Performs import lib operation.
 * Loads a user library module by reading and compiling a .zs file.
 *
 * @param interp      The interpreter instance
 * @param moduleName  The name/path of the module to import (e.g., "request/app")
 *
 * @return Module value, or error value if not found
 */
Value* DoImportLib(Interpreter* interp, String moduleName);

/**
 * Performs import file operation.
 * Loads a module from a specified file path.
 *
 * @param interp    The interpreter instance
 * @param filePath  The file path of the module to import
 *
 * @return Module value, or error value if not found
 */
Value* DoImportFile(Interpreter* interp, String filePath);

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
 * @param clsValue    The class to instantiate
 * @param argc        Number of arguments
 *
 * @return Null value on success, or error value on failure
 */
Value* DoCallCtor(Interpreter* interp, Value* clsValue, int argc);

/**
 * Performs method call operation.
 * Retrieves the method from the object and calls it.
 * Automatically handles 'this' argument for method calls.
 *
 * @param interp      The interpreter instance
 * @param obj         The object on which the method is called
 * @param methodName  The name of the method to call
 * @param argc        Number of arguments
 *
 * @return Null value on success, or error value on failure
 */
Value* DoCallMethod(Interpreter* interp, Value* obj, Value* methodName, int argc);

/**
 * Performs function call operation.
 * Handles both native functions and user-defined functions.
 * Validates argument count and creates appropriate environment for execution.
 *
 * @param interp      The interpreter instance
 * @param fn          The function to call
 * @param argc        Number of arguments
 * @param withThis    Whether to include 'this' in the arguments
 *
 * @return Null value on success, or error value on failure
 */
Value* DoCall(Interpreter* interp, Value* fn, int argc, bool withThis);

/**
 * Performs logical NOT operation on a value.
 * Coerces the value to boolean and negates it.
 *
 * @param interp The interpreter instance
 * @param val    The value to perform the logical NOT operation on
 *
 * @return Boolean value (True or False)
 */
Value* DoNot(Interpreter* interp, Value* val);

/**
 * Performs unary plus operation on a value.
 * Returns the value unchanged.
 *
 * @param interp The interpreter instance
 * @param val    The value to perform the unary plus operation on
 *
 * @return The value unchanged
 */
Value* DoPos(Interpreter* interp, Value* val);

/**
 * Performs unary minus operation on a value.
 * Negates numeric values.
 *
 * @param interp The interpreter instance
 * @param val    The value to perform the unary minus operation on
 *
 * @return Negated numeric value, or error value for invalid operand
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
 * @param offset     The offset of the function in the functions array
 * @param closure    Whether to create a closure (clone) of the function
 *
 * @return The loaded function value
 */
Value* DoLoadFunction(Interpreter* interp, int offset, bool closure);

#endif