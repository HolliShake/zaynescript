/**
 * @file compiler.h
 * @brief Compiler interface for converting parsed AST into bytecode
 *
 * This header defines the compiler interface that transforms abstract syntax
 * trees (AST) produced by the parser into executable bytecode instructions
 * for the interpreter.
 */

#include "./global.h"
#include "./keyword.h"
#include "./operation.h"
#include "./parser.h"
#include "./scope.h"
#include "./value.h"

#ifndef COMPILER_H
#    define COMPILER_H

/**
 * Creates a new compiler instance.
 *
 * Allocates and initializes a new Compiler structure that will be used
 * to compile parsed source code into bytecode.
 *
 * @param interpreter Pointer to the interpreter instance.
 * @param parser Pointer to the parser instance.
 * @return Pointer to the newly created Compiler structure, or NULL on allocation failure.
 */
Compiler* CreateCompiler(Interpreter* interpreter, Parser* parser);

/**
 * Compiles the parsed AST into bytecode.
 *
 * Takes the abstract syntax tree from the parser and compiles it into
 * bytecode instructions that can be executed by the interpreter. This
 * function performs semantic analysis, optimization, and code generation.
 *
 * @param compiler Pointer to the compiler instance containing the parser and interpreter.
 * @return Pointer to a Value containing the compiled UserFunction on success, or NULL on
 * compilation failure.
 */
Value* Compile(Compiler* compiler);

/**
 * Compiles a given AST into bytecode without freeing the AST.
 *
 * This function is similar to Compile(), but it does not free the AST after compilation.
 * It can be used when the caller needs to retain access to the AST for further processing
 * or analysis after compilation.
 *
 * @param compiler Pointer to the compiler instance containing the parser and interpreter.
 * @param programAst Pointer to the root AST node to compile.
 * @return Pointer to a Value containing the compiled UserFunction on success, or NULL on
 * compilation failure.
 */
Value* CompileAst(Compiler* compiler, Ast* programAst);


/**
 * Frees the compiler instance and any associated resources.
 * Cleans up memory allocated for the compiler and its internal structures.
 * @param compiler Pointer to the compiler instance to free.
 */
void FreeCompiler(Compiler* compiler);

#endif /* COMPILER_H */