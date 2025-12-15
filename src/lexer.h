#include "./global.h"
#include "./keyword.h"
#include "./position.h"

#ifndef TOKENIZER_H
#define TOKENIZER_H

/**
 * Creates a new lexer instance for the given source file.
 * 
 * @param path The file path to tokenize
 * @return A pointer to the newly created Lexer, or NULL on failure
 */
Lexer* CreateLexer(String path, Rune* data);

/**
 * Retrieves the next token from the source string.
 * 
 * @param source The source string to tokenize
 * @return The next Token from the source
 */
Token NextToken(Lexer* lexer);

/**
 * Frees all memory associated with a lexer instance.
 * 
 * @param lexer The lexer to free
 */
void FreeLexer(Lexer* lexer);

#endif