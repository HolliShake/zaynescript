#include "./function.h"
#include "./global.h"
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
 */
void Interpret(Interpreter* interpreter);

#endif