/**
 * @file error.h
 * @brief Error type string constants for the interpreter
 *
 * Defines string constants used to identify different categories of runtime
 * errors throughout the interpreter.
 */

#include "global.h"

#ifndef ERROR_H
#    define ERROR_H

/** @brief Import resolution failure */
#    define IMPORT_ERROR "ImportError"
/** @brief Undefined variable or symbol reference */
#    define REFERENCE_ERROR "ReferenceError"
/** @brief Operation on incompatible type */
#    define TYPE_ERROR "TypeError"
/** @brief Array or string index out of bounds */
#    define INDEX_ERROR "IndexError"
/** @brief General runtime failure */
#    define RUNTIME_ERROR "RuntimeError"
/** @brief Wrong number or type of function arguments */
#    define ARGUMENT_ERROR "ArgumentError"
/** @brief Division or modulo by zero */
#    define ZERO_DIVISION_ERROR "ZeroDivisionError"
/** @brief Invalid attribute access on an object */
#    define ATTRIBUTE_ERROR "AttributeError"

#endif