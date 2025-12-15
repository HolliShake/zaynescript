#include "./astnode.h"
#include "./global.h"
#include "./keyword.h"
#include "./position.h"
#include "./lexer.h"

#ifndef PARSER_H
#define PARSER_H

/**
 * @brief Initializes a new parser instance with the given lexer
 * 
 * @param lexer Pointer to the lexer that will provide the token stream
 * @return Parser* Pointer to newly allocated Parser structure
 */
Parser* CreateParser(Lexer* lexer);

/**
 * @brief Parses the token stream and constructs an Abstract Syntax Tree
 * 
 * @param parser Pointer to the parser instance to use for parsing
 * @return Ast* Pointer to the root Ast node representing the parsed program
 */
Ast* Parse(Parser* parser);

/**
 * @brief Frees all memory associated with a parser instance
 * 
 * @param parser Pointer to the parser instance to be freed
 */
void FreeParser(Parser* parser);


#endif