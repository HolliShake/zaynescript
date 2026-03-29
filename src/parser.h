#include "./astnode.h"
#include "./global.h"
#include "./keyword.h"
#include "./lexer.h"
#include "./position.h"

#ifndef PARSER_H
#    define PARSER_H

/*
 * parser.h - Syntax analysis and Abstract Syntax Tree construction
 *
 * This module provides functionality for parsing a stream of tokens into an
 * Abstract Syntax Tree (AST). The parser performs syntax analysis according
 * to the language grammar, constructing a hierarchical representation of the
 * program structure that can be used for semantic analysis and code generation.
 */

/*
 * CreateParser - Creates a new parser instance
 *
 * Allocates and initializes a Parser structure that will consume tokens from
 * the provided lexer. The parser maintains state during the parsing process,
 * including the current token position and error tracking.
 *
 * Parameters:
 *   lexer - Pointer to the lexer that will provide the token stream
 *
 * Returns:
 *   Pointer to the newly created Parser structure, or NULL on allocation failure
 */
Parser* CreateParser(Lexer* lexer);

/*
 * Parse - Parses the token stream and constructs an Abstract Syntax Tree
 *
 * Performs syntax analysis on the token stream provided by the parser's lexer,
 * constructing an AST that represents the program structure. The parser validates
 * that the token sequence conforms to the language grammar and reports syntax
 * errors if invalid constructs are encountered.
 *
 * The resulting AST can be traversed for semantic analysis, optimization, and
 * code generation.
 *
 * Parameters:
 *   parser - Pointer to the parser instance to use for parsing
 *
 * Returns:
 *   Pointer to the root Ast node representing the parsed program, or NULL on parse failure
 */
Ast* Parse(Parser* parser);

/*
 * FreeParser - Frees all memory associated with a parser instance
 *
 * Deallocates the Parser structure and any internal resources it holds.
 * This does not free the lexer or the AST produced by Parse(); those must
 * be freed separately by the caller.
 *
 * Parameters:
 *   parser - Pointer to the parser instance to be freed
 *
 * Returns:
 *   None
 */
void FreeParser(Parser* parser);


#endif