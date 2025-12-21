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
#include <math.h>
#include <limits.h>

// Platform-specific includes for aligned allocation
#if defined(_WIN32)
#include <malloc.h>
#endif

#define GC_THRESHOLD 1000

/**
 * @typedef String
 * @brief Type alias for C-style null-terminated character strings
 */
typedef char* String;

/**
 * @typedef Rune
 * @brief Represents a Unicode code point (UTF-32 character)
 */
typedef int Rune;

/**
 * @struct position_struct
 * @brief Tracks the location of a token in source code
 * 
 * @var position_struct::lineStart
 * Starting line number (1-indexed)
 * @var position_struct::lineEnded
 * Ending line number (1-indexed)
 * @var position_struct::colmStart
 * Starting column number (1-indexed)
 * @var position_struct::colmEnded
 * Ending column number (1-indexed)
 */
typedef struct position_struct {
    int lineStart;
    int lineEnded;
    int colmStart;
    int colmEnded;
} Position;

/**
 * @enum token_type_enum
 * @brief Enumeration of all possible token types in the language
 * 
 * @var token_type_enum::TK_KEY
 * Keyword token (e.g., if, while, fn)
 * @var token_type_enum::TK_SYM
 * Symbol/operator token (e.g., +, ==, &&)
 * @var token_type_enum::TK_IDN
 * Identifier token (variable/function names)
 * @var token_type_enum::TK_INT
 * Integer literal token
 * @var token_type_enum::TK_NUM
 * Floating-point number literal token
 * @var token_type_enum::TK_STR
 * String literal token
 * @var token_type_enum::TK_EOF
 * End-of-file marker
 */
typedef enum token_type_enum {
    TK_KEY,
    TK_SYM,
    TK_IDN,
    TK_INT,
    TK_NUM,
    TK_STR,
    TK_EOF,
} TokenType;

/**
 * @struct token_struct
 * @brief Represents a single lexical token from source code
 * 
 * @var token_struct::Type
 * The type of token (keyword, symbol, identifier, etc.)
 * @var token_struct::Value
 * String representation of the token's value
 * @var token_struct::Position
 * Source code location information for the token
 */
typedef struct token_struct {
    TokenType Type;
    char*     Value;
    Position  Position;
} Token;

/**
 * @struct lexer_struct
 * @brief State machine for lexical analysis of source code
 * 
 * @var lexer_struct::Path
 * File path of the source being tokenized
 * @var lexer_struct::Data
 * Array of Unicode runes representing the source code
 * @var lexer_struct::Line
 * Current line number (1-indexed)
 * @var lexer_struct::Colm
 * Current column number (1-indexed)
 * @var lexer_struct::Indx
 * Current index in the Data array
 */
typedef struct lexer_struct {
    String Path;
    Rune*  Data;
    int    Line;
    int    Colm;
    int    Indx;
} Lexer;

/**
 * @struct parser_struct
 * @brief State machine for syntactic analysis of tokenized source code
 * 
 * @var parser_struct::Lexer
 * Pointer to the lexer providing the token stream
 * @var parser_struct::Next
 * The next token to be processed (lookahead token)
 */
typedef struct parser_struct {
    Lexer* Lexer;
    Token  Next;
} Parser;

/**
 * @enum ast_type_enum
 * @brief Enumeration of all possible AST node types
 */
typedef enum ast_type_enum {
    AST_PROGRAM,
    AST_EXPR,
    AST_FUNCTION,
    AST_CLASS,
    AST_EXPRESSION_STATEMENT,
    // 
    AST_NAME,
    AST_INT,
    AST_NUM,
    AST_STR,
    AST_BOOL,
    AST_NULL,
    // 
    AST_MUL,
    AST_DIV,
    AST_MOD,
    AST_ADD,
    AST_SUB,
    AST_LSHFT,
    AST_RSHFT,
    AST_LT,
    AST_LTE,
    AST_GT,
    AST_GTE,
    AST_EQ,
    AST_NE,
    AST_AND,
    AST_OR,
    AST_XOR,
    AST_LAND,
    AST_LOR
} AstType;

/**
 * @struct ast_struct
 * @brief Abstract Syntax Tree node representing parsed source code structure
 * 
 * @var ast_struct::Type
 * The type of AST node (program, expression, statement, etc.)
 * @var ast_struct::Position
 * Source code location information for this node
 * @var ast_struct::Value
 * String value associated with this node (e.g., identifier name, literal value)
 * @var ast_struct::A
 * First child node pointer (usage varies by node type)
 * @var ast_struct::B
 * Second child node pointer (usage varies by node type)
 * @var ast_struct::C
 * Third child node pointer (usage varies by node type)
 * @var ast_struct::D
 * Fourth child node pointer (usage varies by node type)
 * @var ast_struct::Next
 * Pointer to next AST node in a linked list
 */
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

/**
 * @enum opcode_enum
 * @brief Enumeration of all possible opcode types
 */
typedef enum opcode_enum {
    #include "./opcode.h"
} OpcodeEnum;

/**
 * @struct user_function_struct
 * @brief Represents a user-defined function in the interpreter
 * 
 * @var user_function_struct::Name
 * Optional name of the function (nullable)
 * @var user_function_struct::Codes
 * Array of bytecode instructions representing the compiled function
 * @var user_function_struct::CodeC
 * Number of bytecode instructions in the Codes array
 * @var user_function_struct::Argc
 * Number of arguments the function accepts
 */
typedef struct user_function_struct {
    String   Name; // Nullable
    uint8_t* Codes;
    int      CodeC;
    int      Argc;
} UserFunction;

/**
 * @enum value_type_enum
 * @brief Enumeration of all possible value types in the interpreter
 */
typedef enum value_type_enum {
    VT_INT,
    VT_NUM,
    VT_STR,
    VT_BOOL,
    VT_NULL,
    VT_ARRAY,
    VT_OBJECT,
    VT_CLASS,
    VT_USER_FUNCTION,
    VT_NATV_FUNCTION
} ValueType;

/**
 * @struct value_struct
 * @brief Runtime value structure for the interpreter
 * 
 * @var value_struct::Type
 * The type of value (integer, number, string, boolean, null, user function, native function, class)
 * @var value_struct::Value
 * Union holding the actual value data
 * @var value_struct::Next
 * Pointer to next Value in garbage collector's linked list
 * @var value_struct::Marked
 * Garbage collection mark flag (0 = unmarked, non-zero = marked)
 */
typedef struct value_struct Value;
typedef struct value_struct {
    ValueType  Type;
    union value_union {
        int    I32;    /**< 32-bit integer value */
        double Num;    /**< Double-precision floating point number */
        void*  Opaque; /**< Pointer to heap-allocated data (String, Function, Class, etc.) */
    } Value;
    // GC
    Value* Next;
    int    Marked;
} Value;

/**
 * @brief Forward declaration of HashNode structure
 */
typedef struct hashnode_struct HashNode;

/**
 * @brief Node structure for hash map entries
 * @brief Represents a single key-value pair in the hash map with chaining support
 * Represents a single key-value pair in the hash map with chaining support
 * for collision resolution.
 */
typedef struct hashnode_struct {
    String    Key;
    void*     Val;
    HashNode* Next;
 } HashNode;

 /**
 * @brief Hash map structure for key-value storage
 * 
 * Implements a hash table with separate chaining for collision resolution.
 */
typedef struct hashmap_struct {
    size_t    size;    /**< Total number of buckets in the hash map */
    size_t    count;   /**< Current number of entries in the hash map */
    HashNode* buckets; /**< Array of hash node buckets */
} HashMap;

/**
 * @struct interpreter_struct
 * @brief Main interpreter state structure
 * 
 * Holds the runtime state of the interpreter including singleton values,
 * garbage collection root, and constant pool.
 * 
 * @var interpreter_struct::True
 * Pointer to the singleton boolean true value
 * @var interpreter_struct::False
 * Pointer to the singleton boolean false value
 * @var interpreter_struct::Null
 * Pointer to the singleton null value
 * @var interpreter_struct::GcRoot
 * Root pointer for the garbage collector's linked list of values
 * @var interpreter_struct::Allocated
 * Total number of bytes allocated by the interpreter
 * @var interpreter_struct::Constants
 * Array of pointers to constant values used by the bytecode
 * @var interpreter_struct::ConstantC
 * Number of constants in the Constants array
 */
 typedef struct interpreter_struct {
    Value*   True;
    Value*   False;
    Value*   Null;
    Value*   GcRoot;
    int      Allocated;
    Value**  Constants;
    int      ConstantC;
} Interpreter;

/**
 * @struct compiler_struct
 * @brief Compiler state structure
 * 
 * @var compiler_struct::Interpreter
 * Pointer to the interpreter instance
 * @var compiler_struct::Parser
 * Pointer to the parser instance
 */
typedef struct compiler_struct {
    Interpreter* Interpreter;
    Parser*      Parser;
} Compiler;

/**
 * @typedef NativeFunction
 * @brief Function pointer type for native functions
 * 
 * @param interpreter Pointer to the interpreter instance
 * @param argc Number of arguments passed to the function
 * @param arguments Array of pointers to the arguments passed to the function
 * @return Pointer to the result of the function
 */
typedef Value* (*NativeFunction)(Interpreter* interpreter, int argc, Value** arguments);

/**
 * @def Allocate
 * @brief Wrapper macro for memory allocation with file/line tracking
 * 
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
#define Allocate(size) _Allocate(__FILE__, __LINE__, size)

/**
 * @def Reallocate
 * @brief Wrapper macro for memory reallocation with file/line tracking
 * 
 * @param ptr Pointer to the memory to reallocate
 * @param size Number of bytes to reallocate
 * @return Pointer to reallocated memory, or NULL on failure
 */
#define Reallocate(ptr, size) _Reallocate(__FILE__, __LINE__, ptr, size)

/**
 * @brief Internal allocation function called by Allocate macro
 * 
 * @param file Source file name where allocation was requested
 * @param line Line number where allocation was requested
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* _Allocate(String file, int line, size_t size);

/**
 * @brief Internal reallocation function called by Reallocate macro
 * 
 * @param file Source file name where reallocation was requested
 * @param line Line number where reallocation was requested
 * @param ptr Pointer to the memory to reallocate
 * @param size Number of bytes to reallocate
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* _Reallocate(String file, int line, void* ptr, size_t size);

/**
 * @def AllocateString
 * @brief Wrapper macro for allocating a string with file/line tracking
 * 
 * @param str String to allocate
 * @return Pointer to allocated string, or NULL on failure
 */
String AllocateString(String str);

/**
 * @brief Converts a C string to an array of Unicode runes
 * 
 * @param str Null-terminated C string to convert
 * @return Dynamically allocated array of Rune values, or NULL on failure
 */
Rune* StringToRunes(String str);

/**
 * @brief Coerces a value to a 32-bit integer
 * 
 * @param value Pointer to the Value to coerce
 * @return The value converted to a 32-bit integer
 */
int CoerceToI32(Value* value);

/**
 * @brief Coerces a value to a 64-bit integer
 * 
 * @param value Pointer to the Value to coerce
 * @return The value converted to a 64-bit integer (long)
 */
long CoerceToI64(Value* value);

/**
 * @brief Coerces a value to a double-precision floating point number
 * 
 * @param value Pointer to the Value to coerce
 * @return The value converted to a double
 */
double CoerceToNum(Value* value);

/**
 * @brief Generates a formatted error message with source code context
 * 
 * @param path File path where the error occurred
 * @param runes Array of Unicode runes representing the entire source file
 * @param position Position information (line and column) of the error
 * @param message Error message to display
 * @return Dynamically allocated formatted error string with line numbers and context
 */
String GetErrorLine(String path, Rune* runes, Position position, String message);

/**
 * @brief Throws an error with formatted message and source code context
 * 
 * @param path File path where the error occurred
 * @param runes Array of Unicode runes representing the entire source file
 * @param position Position information (line and column) of the error
 * @param message Error message to display
 */
void ThrowError(String path, Rune* runes, Position position, String message);

#endif
