#include "../class.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_OBJECT_H
#	define CORE_OBJECT_H


/**
 * @brief Creates the Object class
 *
 * This function initializes and returns the Object class, which
 * provides object functionalities to the interpreter.
 *
 * @param interpreter The interpreter instance to create the
 * class in
 * @return Value* Pointer to the Object class value
 */
Value* CreateObjectClass(Interpreter* interpreter);

/**
 * @brief Initializes and loads the Object module into the
 * interpreter.
 *
 * This function creates the Object module with all its built-in
 * functions and methods, making object operations available
 * within the interpreter environment.
 *
 * @param interpreter Pointer to the Interpreter instance where
 * the module will be loaded. Must not be NULL.
 *
 * @return Value* A pointer to the newly created Object module
 * Value object. Returns NULL if module creation fails.
 *
 * @note The returned Value should be managed by the
 * interpreter's memory management system.
 *
 * @see LoadCoreString, LoadCoreNumber (similar module loading
 * functions)
 */
Value* LoadCoreObject(Interpreter* interpreter);

#endif