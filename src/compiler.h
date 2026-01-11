/**
 * @file compiler.h
 * @brief Compiler interface for converting parsed AST into bytecode
 *
 * This header defines the compiler interface that transforms abstract syntax
 * trees (AST) produced by the parser into executable bytecode instructions
 * for the interpreter.
 */

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
 * Allocates and initializes a new Compiler structure that will be used
 * to compile parsed source code into bytecode.
 * 
 * @param[in] interpreter Pointer to the interpreter instance
 * @param[in] parser Pointer to the parser instance
 * @return Pointer to the newly created Compiler structure, or NULL on allocation failure
 */
Compiler* CreateCompiler(Interpreter* interpreter, Parser* parser);

/**
 * @brief Compiles the parsed AST into bytecode
 * 
 * Takes the abstract syntax tree from the parser and compiles it into
 * bytecode instructions that can be executed by the interpreter. This
 * function performs semantic analysis, optimization, and code generation.
 * 
 * @param[in,out] compiler Pointer to the compiler instance containing the parser and interpreter
 * @return Pointer to a Value containing the compiled UserFunction on success, or NULL on compilation failure
 */
Value* Compile(Compiler* compiler);

#endif /* COMPILER_H */