#include "./ast.h"
#include "./global.h"
#include "./keyword.h"
#include "./position.h"
#include "./tokenizer.h"


#ifndef PARSER_H
#define PARSER_H

// CreateParser: Initializes a new parser instance with the given tokenizer
// Parameters:
//   tokenizer - Pointer to the tokenizer that will provide the token stream
// Returns: Pointer to newly allocated Parser structure
Parser* CreateParser(Tokenizer* tokenizer);

// Parse: Parses the token stream and constructs an Abstract Syntax Tree
// Parameters:
//   parser - Pointer to the parser instance to use for parsing
// Returns: Pointer to the root Ast node representing the parsed program
Ast* Parse(Parser* parser);

// FreeParser: Frees all memory associated with a parser instance
// Parameters:
//   parser - Pointer to the parser instance to be freed
// Returns: void
void FreeParser(Parser* parser);


#endif