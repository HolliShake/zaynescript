#include "./global.h"
#include "./operation.h"
#include "./parser.h"
#include "./value.h"

#ifndef COMPILER_H
#define COMPILER_H

/**
 * @brief Creates a new compiler instance
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param parser Pointer to the parser instance
 * @return Pointer to the newly created Compiler structure
 */
Compiler* CreateCompiler(Interpreter* interpreter, Parser* parser);

// Compile
void Compile(Compiler* compiler);

#endif