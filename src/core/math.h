/**
 * @file math.h
 * @brief Core Math module interface
 *
 * This file contains the interface for loading and initializing the core
 * Math module, which provides mathematical functions and constants.
 */

#include "../global.h"
#include "../value.h"

#ifndef CORE_MATH_H
#define CORE_MATH_H

/**
 * @brief Loads the core Math module
 *
 * Initializes and loads the core Math module into the interpreter,
 * registering all mathematical functions and constants.
 * 
 * @param interpeter The interpreter instance to load the module into
 * @return Value* Pointer to the loaded core Math module, or NULL on failure
 */
Value* LoadCoreMath(Interpreter* interpeter);

#endif /* MATH_H */
