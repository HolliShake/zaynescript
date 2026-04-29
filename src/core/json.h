/**
 * @file json.h
 * @brief Definitions and interfaces for json.h.
 */

#include "../error.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_JSON_H
#	define CORE_JSON_H

/**
 * Loads the JSON core module into the interpreter.
 * @param interp The interpreter to load the module into.
 * @return The value of the loaded module.
 */
Value* LoadCoreJson(Interpreter* interp);

#endif