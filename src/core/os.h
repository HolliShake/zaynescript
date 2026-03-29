/**
 * @file os.h
 * @brief Core OS module interface
 *
 * This header file defines the interface for loading the core OS module
 * which provides operating system functionality for the interpreter.
 */

#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_OS_H
#    define CORE_OS_H


/**
 * @brief Loads the core OS module
 *
 * @param  interpreter The interpreter instance to load the OS module into
 * @return Value* Pointer to the loaded OS module
 */
Value* LoadCoreOs(Interpreter* interpreter);

#endif /* CORE_OS_H */
