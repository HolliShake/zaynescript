/**
 * @file interpreter.h
 * @brief Interpreter module for executing parsed programs
 *
 * This module provides the core interpreter functionality for executing
 * parsed programs. It manages the interpreter state, execution context,
 * and provides the main interpretation loop.
 */

#include "./array.h"
#include "./class.h"
#include "./decompiler.h"
#include "./environment.h"
#include "./error.h"
#include "./function.h"
#include "./gc.h"
#include "./global.h"
#include "./operation.h"
#include "./parser.h"
#include "./value.h"

#ifndef INTERPRETER_H
#define INTERPRETER_H

/**
 * Creates and initializes a new interpreter instance.
 *
 * Allocates and initializes a new interpreter with default settings.
 * The interpreter manages execution state, call stack, and runtime
 * environment for program execution.
 * 
 * @return Pointer to newly allocated Interpreter structure, or NULL on failure.
 *
 * @note The caller is responsible for freeing the interpreter using
 *       FreeInterpreter() when done.
 *
 * @see FreeInterpreter()
 */
Interpreter* CreateInterpreter();

/**
 * Executes the parsed program using the given interpreter.
 *
 * Interprets and executes the user function contained in the provided
 * Value structure. This is the main entry point for program execution.
 * 
 * @param interpreter Pointer to the interpreter instance.
 * @param fnValue     Pointer to the Value containing the UserFunction to execute.
 *
 * @pre interpreter must be a valid, non-NULL pointer to an initialized Interpreter.
 * @pre fnValue must be a valid, non-NULL pointer to a Value containing a UserFunction.
 *
 * @note This function may modify the interpreter's internal state.
 *
 * @see CreateInterpreter()
 */
void Interpret(Interpreter* interpreter, Value* fnValue/*UserFunction*/);

/**
 * Frees the interpreter and all associated memory.
 *
 * Deallocates the interpreter instance and releases all associated
 * resources including execution stack, environment, and any other
 * dynamically allocated memory.
 * 
 * @param interpreter Pointer to the interpreter instance to free.
 *
 * @note After calling this function, the interpreter pointer becomes invalid
 *       and should not be used.
 * @note Passing NULL is safe and results in no operation.
 *
 * @see CreateInterpreter()
 */
void FreeInterpreter(Interpreter* interpreter);

#endif /* INTERPRETER_H */