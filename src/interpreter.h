#include "./array.h"
#include "./class.h"
#include "./decompiler.h"
#include "./environment.h"
#include "./function.h"
#include "./gc.h"
#include "./global.h"
#include "./operation.h"
#include "./parser.h"
#include "./value.h"

#ifndef INTERPRETER_H
#define INTERPRETER_H

/**
 * @brief Creates and initializes a new interpreter instance
 * 
 * @return Pointer to newly allocated Interpreter structure, or NULL on failure
 */
Interpreter* CreateInterpreter();

/**
 * @brief Executes the parsed program using the given interpreter
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param fnValue Pointer to the Value containing the UserFunction to execute
 */
void Interpret(Interpreter* interpreter, Value* fnValue/*UserFunction*/);

/**
 * @brief Frees the interpreter and all associated memory
 * 
 * @param interpreter Pointer to the interpreter instance to free
 */
void FreeInterpreter(Interpreter* interpreter);

#endif