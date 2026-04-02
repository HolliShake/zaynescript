/**
 * @file parser.h
 * @brief Syntax analysis and Abstract Syntax Tree construction interface.
 *
 * This module consumes the token stream produced by the lexer and constructs
 * an Abstract Syntax Tree (AST) that represents the hierarchical structure
 * of the program. The parser validates syntax against the language grammar
 * and reports errors for any invalid constructs.
 */

#include "./astnode.h"
#include "./global.h"
#include "./keyword.h"
#include "./lexer.h"
#include "./position.h"

#ifndef PARSER_H
#	define PARSER_H

/**
 * @brief Creates a new parser instance.
 *
 * Allocates and initializes a Parser that will consume tokens from the
 * provided lexer. Reads the first lookahead token upon creation.
 *
 * @param lexer Pointer to the Lexer that will supply the token stream.
 * @return Pointer to the newly created Parser structure, or NULL on
 * allocation failure.
 */
Parser* CreateParser(Lexer* lexer);

/**
 * @brief Parses the token stream and constructs an Abstract Syntax Tree.
 *
 * Performs syntax analysis on the token stream from the parser's lexer,
 * building an AST that represents the full program structure. Validates
 * that the token sequence conforms to the language grammar and reports
 * any syntax errors encountered.
 *
 * The resulting AST root node (AST_PROGRAM) can be passed to the compiler
 * for semantic analysis and code generation.
 *
 * @param parser Pointer to the Parser instance to use.
 * @return Pointer to the root Ast node of the parsed program, or NULL on
 * parse failure.
 */
Ast* Parse(Parser* parser);

/**
 * @brief Frees all memory associated with a parser instance.
 *
 * Deallocates the Parser structure and its internal resources. Does not
 * free the Lexer or any AST nodes produced by Parse(); those must be
 * freed separately by the caller.
 *
 * @param parser Pointer to the Parser instance to free.
 */
void FreeParser(Parser* parser);


#endif