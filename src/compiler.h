/**
 * @file compiler.h
 * @brief Declares the bytecode compiler that lowers LanguageX AST nodes into
 *        executable `UserFunction` Values.
 *
 * The compiler writes into the interpreter's constant/function tables and uses
 * parser metadata to preserve source locations for debugging and errors.
 */

#include "./global.h"
#include "./import.h"
#include "./keyword.h"
#include "./operation.h"
#include "./parser.h"
#include "./scope.h"
#include "./value.h"

#ifndef COMPILER_H
#	define COMPILER_H

/**
 * @brief Allocates compiler state for one parser/interpreter pair.
 *
 * The compiler starts with no module path cached; module initialization happens
 * later when actual compilation begins.
 *
 * @param interpreter Interpreter whose constant/function tables receive emitted
 *                    artifacts.
 * @param parser Parser that owns the AST source metadata for this compile.
 * @return Newly allocated compiler instance.
 */
Compiler* CreateCompiler(Interpreter* interpreter, Parser* parser);

/**
 * @brief Parses the compiler's bound parser and lowers the resulting AST into a
 *        compiled top-level function.
 *
 * Unlike `CompileAst()`, this helper owns the parse step and frees the AST
 * after code generation completes.
 *
 * @param compiler Compiler instance that owns parser/interpreter references.
 * @return Compiled `VLT_USER_FUNCTION` Value representing the module body.
 */
Value* Compile(Compiler* compiler);

/**
 * @brief Lowers an already-built AST into bytecode without taking ownership of
 *        the tree.
 *
 * This is the path used by import helpers and any caller that wants to manage
 * AST lifetime separately from code generation.
 *
 * @param compiler Compiler instance that owns parser/interpreter references.
 * @param programAst Root AST node to lower.
 * @return Compiled `VLT_USER_FUNCTION` Value representing `programAst`.
 */
Value* CompileAst(Compiler* compiler, Ast* programAst);


/**
 * @brief Frees the compiler wrapper and its cached module path string.
 *
 * The interpreter and parser supplied at construction remain owned by the
 * caller.
 *
 * @param compiler Compiler returned by `CreateCompiler()`.
 */
void FreeCompiler(Compiler* compiler);

#endif /* COMPILER_H */