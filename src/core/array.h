#include "../class.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_ARRAY_H
#    define CORE_ARRAY_H

/**
 * @brief Creates the Array class
 *
 * This function initializes and returns the Array class, which provides
 * array functionalities to the interpreter.
 *
 * @param interpreter The interpreter instance to create the class in
 * @return Value* Pointer to the Array class value
 */
Value* CreateArrayClass(Interpreter* interpreter);

/**
 * @brief Initializes and loads the Array module into the interpreter.
 *
 * This function creates the Array module with all its built-in functions and
 * methods, making array operations available within the interpreter environment.
 *
 * @param interpreter Pointer to the Interpreter instance where the module will be loaded.
 *                    Must not be NULL.
 *
 * @return Value* A pointer to the newly created Array module Value object.
 *                Returns NULL if module creation fails.
 *
 * @note The returned Value should be managed by the interpreter's memory management system.
 *
 * @see LoadCoreString, LoadCoreNumber (similar module loading functions)
 */
Value* LoadCoreArray(Interpreter* interpreter);

#endif