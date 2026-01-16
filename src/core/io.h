/**
 * @file io.h
 * @brief Core IO module interface
 *
 * This header file defines the interface for loading the core IO module
 * which provides input/output functionality for the interpreter.
 */

#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_IO_H
#define CORE_IO_H

/**
 * @brief Loads the core IO module
 * 
 * This function initializes and loads the core IO module, which provides
 * input/output operations for the interpreter.
 * 
 * @param interpeter The interpreter instance to load the IO module into
 * @return Value* Pointer to the loaded core IO module, or NULL on failure
 */
Value* LoadCoreIo(Interpreter* interpeter);

#endif /* IO_H */