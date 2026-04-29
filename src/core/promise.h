#ifndef CORE_PROMISE_H
#define CORE_PROMISE_H

/**
 * @file promise.h
 * @brief Declares the built-in Promise class loader used by core modules.
 *
 * This module wires the script-visible Promise methods into the interpreter's
 * class system and exposes the loader used by `import core.promise` style
 * bootstrap paths.
 */

#include "../class.h"
#include "../environment.h"
#include "../error.h"
#include "../function.h"
#include "../global.h"
#include "../statemachine.h"
#include "../value.h"

/**
 * @brief Builds the interpreter's singleton `Promise` class object.
 *
 * Registers the native `.then()` and `.error()` methods against a fresh class
 * whose base is the built-in `Object` class. The returned Value is usually
 * cached on `interpreter->Promise` and reused across the lifetime of the VM.
 *
 * @param interpreter Interpreter that owns the class object, method metadata,
 *                    and any allocations performed while creating it.
 * @return Heap-managed class Value ready to store in the core module table.
 */
Value* CreatePromiseClass(Interpreter* interpreter);

/**
 * @brief Returns the core module object that exports the Promise class.
 *
 * Ensures the interpreter has a Promise singleton, creates a plain object
 * module wrapper, and binds that singleton under the `Promise` property for
 * importers.
 *
 * @param interpreter Interpreter whose cached Promise class should be exposed
 *                    through a module object.
 * @return Object Value containing the `Promise` export.
 */
Value* LoadCorePromise(Interpreter* interpreter);

#endif