#include "./core/io.h"
#include "./core/math.h"
#include "./global.h"
#include "./gc.h"
#include "./value.h"

#ifndef OPERATION_H
#define OPERATION_H

#define FLG_NOTFOUND -1
#define FLG_ZERO_DIV -2
#define FLG_INVALID_OPERATION -3

/**
 * @brief Performs import core operation
 * 
 * @param interp The interpreter instance
 * @param moduleName The name of the module to import
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_NOTFOUND)
 */
int DoImportCore(Interpreter* interp, String moduleName, Value** out);

/**
 * Performs multiplication operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoMul(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs division operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_ZERO_DIV, FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoDiv(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs modulo operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_ZERO_DIV, FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoMod(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs increment operation on a value.
 * @param interp The interpreter instance
 * @param val The value to increment
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoInc(Interpreter* interp, Value* val, Value** out);

/**
 * Performs addition operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoAdd(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs decrement operation on a value.
 * @param interp The interpreter instance
 * @param val The value to decrement
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoDec(Interpreter* interp, Value* val, Value** out);

/**
 * Performs subtraction operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoSub(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs left shift operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoLShift(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs right shift operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoRShift(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs less than operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoLT(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs less than or equal to operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoLTE(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs greater than operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoGT(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs greater than or equal to operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoGTE(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs equal to operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoEQ(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs not equal to operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoNE(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs bitwise AND operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoAnd(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs bitwise OR operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoOr(Interpreter* interp, Value* lhs, Value* rhs, Value** out);

/**
 * Performs bitwise XOR operation on two values.
 * @param interp The interpreter instance
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @param out Output parameter for the result (if not NULL, result is stored here)
 * @return Offset in constants array, or error flag (FLG_INVALID_OPERATION, FLG_NOTFOUND)
 */
int DoXor(Interpreter* interp, Value* lhs, Value* rhs, Value** out);


/**
 * Loads a function from the interpreter's functions array.
 * @param interp The interpreter instance
 * @param rootEnvObj The root environment object
 * @param envObj The local environment object
 * @param offset The offset of the function in the functions array
 * @param closure Whether to create a closure (clone) of the function
 * @param out Output parameter for the result (if not NULL, result is stored here)
 */
void DoLoadFunction(Interpreter* interp, Value* rootEnvObj, Value* envObj, int offset, bool closure, Value** out);

#endif