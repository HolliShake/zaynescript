/**
 * @file lexer.h
 * @brief Declares the tokenizer that turns rune-based source buffers into the
 *        token stream consumed by the parser.
 *
 * The lexer tracks line and column positions, recognizes keywords and literal
 * forms used by LanguageX, and preserves source-path information for parse and
 * runtime diagnostics.
 */

#include "./global.h"
#include "./keyword.h"
#include "./position.h"

#ifndef TOKENIZER_H
#	define TOKENIZER_H

/**
 * @brief Allocates a lexer positioned at the start of a rune buffer.
 *
 * The returned lexer stores the supplied path for diagnostics and begins at
 * line 1, column 1, ready for `NextToken()`.
 *
 * @param path Logical source path reported in lexer and parser errors.
 * @param data NUL-terminated rune buffer to tokenize; the caller retains
 *             ownership of the buffer.
 * @return Newly allocated lexer instance.
 */
Lexer* CreateLexer(String path, Rune* data);

/**
 * @brief Scans and returns the next token from the input stream.
 *
 * Whitespace and comments are skipped before token classification. Returned
 * tokens carry a freshly allocated lexeme string plus source position data.
 *
 * @param lexer Lexer whose cursor should advance.
 * @return Next token in source order, including `TK_EOF` at end of input.
 */
Token NextToken(Lexer* lexer);

/**
 * @brief Frees the lexer wrapper itself.
 *
 * The implementation does not release `lexer->Path` or `lexer->Data`; callers
 * must manage those buffers separately.
 *
 * @param lexer Lexer returned by `CreateLexer()`. Passing `NULL` is safe.
 */
void FreeLexer(Lexer* lexer);

#endif