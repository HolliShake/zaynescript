/**
 * @file lexer.h
 * @brief Lexical analysis and tokenization interface.
 *
 * This module scans source text and converts it into a stream of
 * tokens consumable by the parser. It handles
 * character-by-character processing, recognizing keywords,
 * identifiers, literals, operators, comments, and whitespace.
 * Unicode input is represented as an array of Runes (UTF-32 code
 * points).
 */

#include "./global.h"
#include "./keyword.h"
#include "./position.h"

#ifndef TOKENIZER_H
#	define TOKENIZER_H

/**
 * @brief Creates a new lexer instance for tokenizing source
 * code.
 *
 * Allocates and initializes a Lexer that will process the
 * provided source data. The lexer tracks line and column
 * positions for accurate error reporting.
 *
 * @param path File path of the source being tokenized (used for
 * error reporting).
 * @param data Pointer to the source code as a null-terminated
 * array of Runes (Unicode code points).
 * @return Pointer to the newly created Lexer structure, or NULL
 * on allocation failure.
 */
Lexer* CreateLexer(String path, Rune* data);

/**
 * @brief Advances the lexer and returns the next token from the
 * source stream.
 *
 * Skips whitespace and comments, then classifies the next
 * sequence of characters as a keyword, identifier, numeric
 * literal, string literal, operator, or end-of-file token.
 *
 * @param lexer Pointer to the Lexer instance to advance.
 * @return A Token structure containing the token type, lexeme
 * string, and source position.
 */
Token NextToken(Lexer* lexer);

/**
 * @brief Frees all memory associated with a lexer instance.
 *
 * Deallocates the Lexer structure and any internally owned
 * resources. Does not free the source data array passed to
 * CreateLexer(); the caller is responsible for managing that
 * buffer.
 *
 * @param lexer Pointer to the Lexer instance to free.
 */
void FreeLexer(Lexer* lexer);

#endif