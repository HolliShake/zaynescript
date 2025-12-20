#include "./global.h"
#include "./parser.h"
#include "./value.h"
#include "./function.h"
#include "./exceptionhandler.h"

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
 * @param parser Pointer to the parser containing the AST to interpret
 */
void Interpret(Interpreter* interpreter, Parser* parser);

#endif