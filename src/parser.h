/**
 * @file parser.h
 * @brief Declares the recursive-descent parser that turns lexer tokens into an
 *        abstract syntax tree.
 *
 * The parser owns one-token lookahead, raises syntax errors with source spans,
 * and produces the module AST consumed by the compiler.
 */

#include "./astnode.h"
#include "./global.h"
#include "./keyword.h"
#include "./lexer.h"
#include "./position.h"

#ifndef PARSER_H
#	define PARSER_H

/**
 * @brief Allocates a parser and binds it to a lexer.
 *
 * The parser stores the lexer pointer and is expected to populate `Next`
 * through its internal accept/check helpers as parsing begins.
 *
 * @param lexer Lexer that supplies tokens for this parse.
 * @return Newly allocated parser instance.
 */
Parser* CreateParser(Lexer* lexer);

/**
 * @brief Parses the entire token stream into a top-level `AST_PROGRAM` tree.
 *
 * The parser consumes declarations, statements, and expressions until EOF and
 * throws on the first syntax error via the shared error-reporting helpers.
 *
 * @param parser Parser instance consuming the current lexer stream.
 * @return Root AST node for the compilation unit.
 */
Ast* Parse(Parser* parser);

/**
 * @brief Frees the parser wrapper without touching the lexer or produced AST.
 *
 * @param parser Parser returned by `CreateParser()`. Passing `NULL` is safe.
 */
void FreeParser(Parser* parser);


#endif