#include "./global.h"
#include "./keyword.h"
#include "./position.h"

#ifndef TOKENIZER_H
#define TOKENIZER_H

/*
 * lexer.h - Lexical analysis and tokenization
 *
 * This module provides functionality for lexical analysis (tokenization) of source
 * code. The lexer scans through source text and converts it into a stream of tokens
 * that can be consumed by the parser. It handles character-by-character processing,
 * recognizing keywords, identifiers, literals, operators, and other language elements.
 */

/**
 * CreateLexer - Creates a new lexer instance for tokenizing source code
 *
 * Allocates and initializes a Lexer structure that will process the provided source
 * code data. The lexer maintains state during tokenization, including the current
 * position in the source, line and column tracking for error reporting, and lookahead
 * capabilities.
 *
 * @param path The file path of the source being tokenized (used for error reporting)
 * @param data Pointer to the source code data as an array of runes (Unicode code points)
 *
 * @return Pointer to the newly created Lexer structure, or NULL on allocation failure
 */
Lexer* CreateLexer(String path, Rune* data);

/**
 * NextToken - Retrieves the next token from the source code stream
 *
 * Advances the lexer's position in the source code and returns the next token.
 * The lexer performs character-level analysis to identify token boundaries and
 * classify tokens according to the language grammar. This function handles:
 * - Whitespace and comment skipping
 * - Keyword and identifier recognition
 * - Literal value parsing (numbers, strings, etc.)
 * - Operator and punctuation recognition
 * - Error token generation for invalid sequences
 *
 * @param lexer Pointer to the lexer instance to read from
 *
 * @return The next Token structure containing the token type, lexeme, and position information
 */
Token NextToken(Lexer* lexer);

/**
 * FreeLexer - Frees all memory associated with a lexer instance
 *
 * Deallocates the Lexer structure and any internal resources it holds.
 * This does not free the source data passed to CreateLexer(); that must
 * be managed separately by the caller.
 *
 * @param lexer Pointer to the lexer instance to be freed
 *
 * @return None
 */
void FreeLexer(Lexer* lexer);

#endif