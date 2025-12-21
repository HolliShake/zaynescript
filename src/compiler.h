#include "./global.h"
#include "./operation.h"
#include "./parser.h"
#include "./value.h"
#include "./scope.h"

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

/**
 * @brief Compiles the parsed AST into bytecode
 * 
 * Takes the abstract syntax tree from the parser and compiles it into
 * bytecode instructions that can be executed by the interpreter.
 * 
 * @param compiler Pointer to the compiler instance containing the parser and interpreter
 * @return Pointer to a Value containing the compiled UserFunction, or NULL on compilation failure
 */
Value* Compile(Compiler* compiler);

#endif