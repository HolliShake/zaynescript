#ifndef GLOBAL_H
#define GLOBAL_H

// Order patters
#include "../utf/utf.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform-specific includes for aligned allocation
#if defined(_WIN32)
#include <malloc.h>
#endif

// String: Type alias for C-style null-terminated character strings
typedef char* String;

// Rune: Represents a Unicode code point (UTF-32 character)
typedef int Rune;

// Position: Tracks the location of a token in source code
// Fields:
//   lineStart - Starting line number (1-indexed)
//   lineEnded - Ending line number (1-indexed)
//   colmStart - Starting column number (1-indexed)
//   colmEnded - Ending column number (1-indexed)
typedef struct position_struct {
    int lineStart;
    int lineEnded;
    int colmStart;
    int colmEnded;
} Position;

// TokenType: Enumeration of all possible token types in the language
// Values:
//   TK_KEY - Keyword token (e.g., if, while, fn)
//   TK_SYM - Symbol/operator token (e.g., +, ==, &&)
//   TK_IDN - Identifier token (variable/function names)
//   TK_INT - Integer literal token
//   TK_NUM - Floating-point number literal token
//   TK_STR - String literal token
//   TK_EOF - End-of-file marker
typedef enum token_type_enum {
    TK_KEY,
    TK_SYM,
    TK_IDN,
    TK_INT,
    TK_NUM,
    TK_STR,
    TK_EOF,
} TokenType;

// Token: Represents a single lexical token from source code
// Fields:
//   Type     - The type of token (keyword, symbol, identifier, etc.)
//   Value    - String representation of the token's value
//   Position - Source code location information for the token
typedef struct token_struct {
    TokenType Type;
    char*     Value;
    Position  Position;
} Token;

// Tokenizer: State machine for lexical analysis of source code
// Fields:
//   Path - File path of the source being tokenized
//   Data - Array of Unicode runes representing the source code
//   Line - Current line number (1-indexed)
//   Colm - Current column number (1-indexed)
//   Indx - Current index in the Data array
typedef struct tokenizer_struct {
    String Path;
    Rune*  Data;
    int    Line;
    int    Colm;
    int    Indx;
} Tokenizer;

// Parser: State machine for syntactic analysis of tokenized source code
// Fields:
//   Tokenizer - Pointer to the tokenizer providing the token stream
//   Next      - The next token to be processed (lookahead token)
typedef struct parser_struct {
    Tokenizer* Tokenizer;
    Token      Next;
} Parser;

typedef enum ast_type_enum {
    AST_PROGRAM,
    AST_EXPR,
    AST_STMT,
    AST_DECL,
    AST_TYPE,
    AST_FUNC,
    AST_CLASS,
    // 
    AST_NAME,
    AST_INTEGER,
    AST_NUMBER,
    AST_STRING,
    AST_BOOL,
    AST_NULL,
    // 
    AST_MUL,
    AST_DIV,
    AST_MOD,
} AstType;

// Ast: Abstract Syntax Tree node representing parsed source code structure
// Fields:
//   Type     - The type of AST node (program, expression, statement, etc.)
//   Position - Source code location information for this node
//   Value    - String value associated with this node (e.g., identifier name, literal value)
//   A        - First child node pointer (usage varies by node type)
//   B        - Second child node pointer (usage varies by node type)
//   C        - Third child node pointer (usage varies by node type)
//   D        - Fourth child node pointer (usage varies by node type)
typedef struct ast_struct Ast;
typedef struct ast_struct {
    AstType  Type;
    Position Position;
    String   Value;
    Ast*     A;
    Ast*     B;
    Ast*     C;
    Ast*     D;
    Ast*     Next;
} Ast;

// Allocate: Wrapper macro for memory allocation with file/line tracking
// Usage: ptr = Allocate(size_in_bytes)
// Returns: Pointer to allocated memory, or NULL on failure
#define Allocate(size) _Allocate(__FILE__, __LINE__, size)

// _Allocate: Internal allocation function called by Allocate macro
// Parameters:
//   file - Source file name where allocation was requested
//   line - Line number where allocation was requested
//   size - Number of bytes to allocate
// Returns: Pointer to allocated memory, or NULL on failure
void* _Allocate(String file, int line, size_t size);

// StringToRunes: Converts a C string to an array of Unicode runes
// Parameters:
//   str - Null-terminated C string to convert
// Returns: Dynamically allocated array of Rune values, or NULL on failure
Rune* StringToRunes(String str);

// GetErrorLine: Generates a formatted error message with source code context
// Parameters:
//   path     - File path where the error occurred
//   runes    - Array of Unicode runes representing the entire source file
//   position - Position information (line and column) of the error
//   message  - Error message to display
// Returns: Dynamically allocated formatted error string with line numbers and context
String GetErrorLine(String path, Rune* runes, Position position, String message);

// ThrowError: Throws an error with formatted message and source code context
// Parameters:
//   path     - File path where the error occurred
//   runes    - Array of Unicode runes representing the entire source file
//   position - Position information (line and column) of the error
//   message  - Error message to display
// Returns: void
void ThrowError(String path, Rune* runes, Position position, String message);

#endif