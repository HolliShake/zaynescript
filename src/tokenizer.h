#include "./global.h"
#include "./keyword.h"
#include "./position.h"

#ifndef TOKENIZER_H
#define TOKENIZER_H

/**
 * Creates a new tokenizer instance for the given source file.
 * 
 * @param path The file path to tokenize
 * @return A pointer to the newly created Tokenizer, or NULL on failure
 */
Tokenizer* CreateTokenizer(String path, Rune* data);

/**
 * Retrieves the next token from the source string.
 * 
 * @param source The source string to tokenize
 * @return The next Token from the source
 */
Token NextToken(Tokenizer* tokenizer);

/**
 * Frees all memory associated with a tokenizer instance.
 * 
 * @param tokenizer The tokenizer to free
 */
void FreeTokenizer(Tokenizer* tokenizer);

#endif