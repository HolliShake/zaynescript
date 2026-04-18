/**
 * @file blob.h
 * @brief Core Blob module interface.
 *
 * Declares creation of the built-in Blob class and loading of the
 * core Blob module for byte-buffer operations in the interpreter.
 */

#include "../array.h"
#include "../class.h"
#include "../function.h"
#include "../value.h"

#ifndef CORE_BLOB_H
#	define CORE_BLOB_H

/**
 * @brief Creates the Blob class
 *
 * This function initializes the Blob class with its methods and
 * properties. It is called during the loading of the core Blob
 * module.
 *
 * @param interpreter The interpreter instance to create the
 * Blob class in
 * @return Pointer to the created Blob class, or NULL on
 * failure
 */
Value* CreateBlobClass(Interpreter* interpreter);

/**
 * @brief Loads the core Blob module
 *
 * This function initializes and loads the core Blob module,
 * which provides blob functionality for the interpreter.
 *
 * @param interpreter The interpreter instance to load the Blob
 * module into
 * @return Pointer to the loaded core Blob module, or NULL on
 * failure
 */
Value* LoadCoreBlob(Interpreter* interpreter);

#endif
