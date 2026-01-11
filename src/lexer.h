/*
 * lexer.h - Lexical analyzer interface
 *
 * This header defines the interface for the lexical analyzer (lexer/tokenizer)
 * which converts source code text into a stream of tokens.
 */

#include "./global.h"
#include "./keyword.h"
#include "./position.h"

#ifndef TOKENIZER_H
#define TOKENIZER_H

/*
 * CreateLexer - Creates a new lexer instance for the given source file
 *
 * Parameters:
 *   path - The file path to tokenize
 *   data - Pointer to the source code data as runes
 *
 * Returns:
 *   A pointer to the newly created Lexer, or NULL on failure
 */
Lexer* CreateLexer(String path, Rune* data);

/*
 * NextToken - Retrieves the next token from the lexer
 *
 * Parameters:
 *   lexer - The lexer instance to read from
 *
 * Returns:
 *   The next Token from the source
 */
Token NextToken(Lexer* lexer);

/*
 * FreeLexer - Frees all memory associated with a lexer instance
 *
 * Parameters:
 *   lexer - The lexer to free
 *
 * Returns:
 *   None
 */
void FreeLexer(Lexer* lexer);

#endif