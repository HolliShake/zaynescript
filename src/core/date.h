/**
 * @file date.h
 * @brief Core Date module interface
 *
 * This header file defines the interface for loading the core Date module
 * which provides date and time functionality for the interpreter.
 */

#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_DATE_H
#define CORE_DATE_H

/**
 * @brief Loads the core Date module
 * 
 * This function initializes and loads the core Date module, which provides
 * date and time functionalities for the interpreter.
 * 
 * @param  interpreter The interpreter instance to load the Date module into
 * @return Value* Pointer to the loaded core Date module, or NULL on failure
 */
Value* LoadCoreDate(Interpreter*  interpreter);

#endif