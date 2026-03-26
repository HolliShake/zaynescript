/**
 * @file global.h
 * @brief Global definitions, types, and macros for the language interpreter.
 * 
 * This file contains the core data structures, type definitions, and constants
 * used throughout the interpreter and compiler.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

// Order patterns
#include "../utf/utf.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../libbf/libbf.h"
#include "../libbf/cutils.h"

// Platform-specific includes for aligned allocation
#if defined(_WIN32)
#include <malloc.h>
#endif

// Constants & Macros

#define GC_GROWTH_FACTOR 2

/**
 * @def GC_THRESHOLD
 * @brief The allocation threshold for triggering garbage collection.
 */
#define GC_THRESHOLD 1024

/**
 * @def VARARG
 * @brief Constant indicating variable number of arguments for functions.
 */
#define VARARG -1

/**
 * @def STACK_SIZE
 * @brief The maximum size of the interpreter's execution stack.
 */
#define STACK_SIZE 1500

/**
 * @def CONSTRUCTOR_NAME
 * @brief The reserved name for class constructor methods.
 */
#define CONSTRUCTOR_NAME "init"

#define PREC_INT 0

#define PREC_DBL 53

#define PREC_NUM(len) (uint64_t)len * LIMB_BITS

/**
 * @def Panic
 * @brief Prints an error message and terminates the program.
 * 
 * @param message The error message format string.
 * @param ... Additional arguments for the format string.
 */
#define Panic(message, ...) do { \
    fprintf(stderr, "[%s:%d]::Panic: ", __FILE__, __LINE__); \
    fprintf(stderr, message, ##__VA_ARGS__); \
    fprintf(stderr, "\n"); \
    exit(EXIT_FAILURE); \
} while(0)

// -----------------------------------------------------------------------------
// Basic Types
// -----------------------------------------------------------------------------

/**
 * @typedef String
 * @brief Type alias for C-style null-terminated character strings.
 */
typedef char* String;

/**
 * @typedef Rune
 * @brief Represents a Unicode code point (UTF-32 character).
 */
typedef int Rune;

// -----------------------------------------------------------------------------
// Generic Data Structures
// -----------------------------------------------------------------------------

/**
 * @struct array_struct
 * @brief Dynamic array structure for storing generic pointers.
 * 
 * This structure provides a resizable array implementation that can store
 * pointers to any type of data. It maintains both the current count of items
 * and the allocated capacity.
 */
typedef struct array_struct {
    void** Items;    /**< Pointer to the array of items */
    size_t Capacity; /**< Allocated capacity of the array */
    size_t Count;    /**< Current number of items in the array */
} Array;

/**
 * @struct hashnode_struct
 * @brief Node structure for hash map entries.
 * 
 * Represents a single entry in a hash map bucket. Uses chaining for collision
 * resolution through the Next pointer.
 */
typedef struct hashnode_struct HashNode;
struct hashnode_struct {
    String    Key;  /**< The key string for the entry */
    void*     Val;  /**< The value associated with the key */
    HashNode* Next; /**< Pointer to the next node in the bucket chain (collision handling) */
};

/**
 * @struct hashmap_struct
 * @brief Hash map structure for key-value storage.
 * 
 * Implements a hash table with separate chaining for collision resolution.
 * Keys are strings and values are generic pointers.
 */
typedef struct hashmap_struct {
    size_t    Size;    /**< Total number of buckets in the hash map */
    size_t    Count;   /**< Current number of entries in the hash map */
    HashNode* Buckets; /**< Array of hash node buckets */
} HashMap;

// -----------------------------------------------------------------------------
// Lexer & Parser (Frontend)
// -----------------------------------------------------------------------------

/**
 * @struct position_struct
 * @brief Tracks the location of a token or AST node in source code.
 * 
 * Stores both line and column information for the start and end positions
 * of a token or syntax element. All values are 1-based.
 */
typedef struct position_struct {
    int LineStart; /**< Starting line number (1-based) */
    int LineEnded; /**< Ending line number (1-based) */
    int ColmStart; /**< Starting column number (1-based) */
    int ColmEnded; /**< Ending column number (1-based) */
} Position;

/**
 * @enum token_kind_enum
 * @brief Enumeration of all possible token types in the language.
 * 
 * Defines the lexical token categories recognized by the lexer.
 */
typedef enum token_kind_enum {
    TK_KEY,  /**< Keyword token */
    TK_SYM,  /**< Symbol/Operator token */
    TK_IDN,  /**< Identifier token */
    TK_INT,  /**< Integer literal token */
    TK_BINT, /**< Big integer literal token */
    TK_NUM,  /**< Numeric (float) literal token */
    TK_BNUM, /**< Big numeric (float) literal token */
    TK_STR,  /**< String literal token */
    TK_EOF,  /**< End-of-file token */
} TokenKind;

/**
 * @struct token_struct
 * @brief Represents a single lexical token from source code.
 * 
 * Contains the token type, its string value, and its position in the source.
 */
typedef struct token_struct {
    TokenKind Type;     /**< The type of the token */
    String    Value;    /**< The string value of the token */
    Position  Position; /**< The location of the token in the source code */
} Token;

/**
 * @struct lexer_struct
 * @brief State machine for lexical analysis of source code.
 * 
 * Maintains the current position in the source file and provides
 * tokenization functionality.
 */
typedef struct lexer_struct {
    String Path; /**< Path to the source file being lexed */
    Rune*  Data; /**< The source code content as runes */
    int    Line; /**< Current line number */
    int    Colm; /**< Current column number */
    int    Indx; /**< Current index in the Data array */
} Lexer;

/**
 * @struct parser_struct
 * @brief State machine for syntactic analysis of tokenized source code.
 * 
 * Consumes tokens from the lexer and builds an abstract syntax tree.
 */
typedef struct parser_struct {
    Lexer* Lexer; /**< Pointer to the lexer instance */
    Token  Next;  /**< The next token to be consumed (lookahead) */
} Parser;

// -----------------------------------------------------------------------------
// Abstract Syntax Tree (AST)
// -----------------------------------------------------------------------------

/**
 * @enum ast_type_enum
 * @brief Enumeration of all possible AST node types.
 * 
 * Defines the complete set of syntax tree node types representing
 * all language constructs including statements, expressions, literals,
 * and operators.
 */
typedef enum ast_type_enum {
    AST_PROGRAM,              /**< Root program node */
    AST_EXPR,                 /**< Expression statement */
    AST_CONTINUE,             /**< Continue statement */
    AST_BREAK,                /**< Break statement */
    AST_RETURN,               /**< Return statement */
    AST_FUNCTION,             /**< Function declaration */
    AST_IMPORT,               /**< Import statement */
    AST_VAR_DECLARATION,      /**< Variable declaration (var) */
    AST_CONST_DECLARATION,    /**< Constant declaration (const) */
    AST_LOCAL_DECLARATION,    /**< Local declaration (local) */
    AST_EMPTY_STATEMENT,      /**<  Empty statement (;;) */
    AST_CLASS,                /**< Class declaration */
    AST_CLASS_MEMBER,         /**< Class member definition */
    AST_EXPRESSION_STATEMENT, /**< Statement wrapping an expression */
    AST_IF,                   /**< If statement */
    AST_FOR,                  /**< For loop statement */
    AST_WHILE,                /**< While loop statement */
    AST_DO_WHILE,             /**< Do-while loop statement */
    AST_BLOCK,                /**< Block of statements */
    AST_TRY_CATCH,            /**< Try-catch block */
    // Literals
    AST_NAME,           /**< Identifier/Name */
    AST_INT,            /**< Integer literal */
    AST_BINT,           /**< Big integer literal */
    AST_NUM,            /**< Number literal */
    AST_BNUM,           /**< Big numeric (float) literal */
    AST_STR,            /**< String literal */
    AST_BOOL,           /**< Boolean literal */
    AST_NULL,           /**< Null literal */
    AST_THIS,           /**< This keyword */
    AST_SPREAD,         /**< Spread operator (...) */
    AST_LIST_LITERAL,   /**< List literal ([...]) */
    AST_OBJECT_KEY_VAL, /**< Object key-value pair */
    AST_OBJECT_LITERAL, /**< Object literal ({...}) */
    AST_ALLOCATION,     /**< Allocation expression (new Class(...)) */
    AST_MEMBER,         /**< Member access (obj.prop) */
    AST_INDEX,          /**< Index access (obj[index]) */
    AST_CALL,           /**< Function call */
    // Unary Operators
    AST_POST_INC,    /**< Post-increment (i++) */
    AST_POST_DEC,    /**< Post-decrement (i--) */
    AST_POSITIVE,    /**< Unary plus (+) */
    AST_NEGATIVE,    /**< Unary minus (-) */
    AST_LOGICAL_NOT, /**< Logical NOT (!) */
    AST_PRE_INC,     /**< Pre-increment (++i) */
    AST_PRE_DEC,     /**< Pre-decrement (--i) */
    AST_AWAIT,       /**< Await expression (await promise) */
    // Binary Operators
    AST_MUL,          /**< Multiplication (*) */
    AST_DIV,          /**< Division (/) */
    AST_MOD,          /**< Modulo (%) */
    AST_ADD,          /**< Addition (+) */
    AST_SUB,          /**< Subtraction (-) */
    AST_LSHFT,        /**< Left shift (<<) */
    AST_RSHFT,        /**< Right shift (>>) */
    AST_LT,           /**< Less than (<) */
    AST_LTE,          /**< Less than or equal (<=) */
    AST_GT,           /**< Greater than (>) */
    AST_GTE,          /**< Greater than or equal (>=) */
    AST_EQ,           /**< Equality (==) */
    AST_NE,           /**< Inequality (!=) */
    AST_AND,          /**< Bitwise AND (&) */
    AST_OR,           /**< Bitwise OR (|) */
    AST_XOR,          /**< Bitwise XOR (^) */
    AST_LAND,         /**< Logical AND (&&) */
    AST_LOR,          /**< Logical OR (||) */
    AST_TERNARY,      /**< Ternary operator (condition ? true : false) */
    AST_ASSIGN,       /**< Assignment (=) */
    AST_SHORT_ASSIGN, /**< Short assignment (e.g. +=, -=) - specific type determined by operation */
    AST_MUL_ASSIGN,   /**< Multiply assignment (*=) */
    AST_DIV_ASSIGN,   /**< Divide assignment (/=) */
    AST_MOD_ASSIGN,   /**< Modulo assignment (%=) */
    AST_ADD_ASSIGN,   /**< Add assignment (+=) */
    AST_SUB_ASSIGN,   /**< Subtract assignment (-=) */
    AST_LSHFT_ASSIGN, /**< Left shift assignment (<<=) */
    AST_RSHFT_ASSIGN, /**< Right shift assignment (>>=) */
    AST_AND_ASSIGN,   /**< Bitwise AND assignment (&=) */
    AST_OR_ASSIGN,    /**< Bitwise OR assignment (|=) */
    AST_XOR_ASSIGN,   /**< Bitwise XOR assignment (^=) */
} AstType;

/**
 * @struct ast_struct
 * @brief Abstract Syntax Tree node representing parsed source code structure.
 * 
 * A flexible tree node structure that can represent any language construct.
 * The meaning of child nodes (A, B, C, D) depends on the Type field.
 * Nodes can be chained using the Next pointer for lists and sequences.
 */
typedef struct ast_struct Ast;
struct ast_struct {
    AstType  Type;     /**< The type of the AST node */
    Position Position; /**< The position in the source code */
    String   Value;    /**< String value (for identifiers, literals, etc.) */
    bool     Flag;     /**< Generic boolean flag for various uses (e.g. async functions) */
    Ast*     A;        /**< First child node (usage depends on Type) */
    Ast*     B;        /**< Second child node (usage depends on Type) */
    Ast*     C;        /**< Third child node (usage depends on Type) */
    Ast*     D;        /**< Fourth child node (usage depends on Type) */
    Ast*     Next;     /**< Next sibling node (for lists/blocks) */
};

// -----------------------------------------------------------------------------
// Runtime Values & Type System
// -----------------------------------------------------------------------------

/**
 * @enum value_type_enum
 * @brief Enumeration of all possible value types in the interpreter.
 * 
 * Defines the runtime type system including primitives, collections,
 * functions, and objects.
 */
typedef enum value_type_enum {
    VLT_ERROR,          /**< Error value */
    VLT_INT,            /**< Integer value */
    VLT_BINT,           /**< Big integer value */
    VLT_NUM,            /**< Number value */
    VLT_BNUM,           /**< Big numeric (float) value */
    VLT_STR,            /**< String value */
    VLT_BOOL,           /**< Boolean value */
    VLT_NULL,           /**< Null value */
    VLT_PROMISE,        /**< Promise value */
    VLT_ARRAY,          /**< Array value */
    VLT_OBJECT,         /**< Object value */
    VLT_CLASS,          /**< Class definition value */
    VLT_CLASS_INSTANCE, /**< Class instance value */
    VLT_USER_FUNCTION,  /**< User-defined function value */
    VLT_NATV_FUNCTION,  /**< Native function value */
    VLT_ENVIRONMENT,    /**< Environment value */
} ValueType;

/**
 * @struct value_struct
 * @brief Runtime value structure for the interpreter.
 * 
 * Tagged union representing any runtime value. Uses a discriminated union
 * to store different value types efficiently. Includes garbage collection
 * metadata (Next pointer and Marked flag).
 */
typedef struct value_struct Value;
struct value_struct {
    ValueType  Type; /**< The type of the value */
    union value_union {
        int    I32;    /**< 32-bit integer value */
        double Num;    /**< Double-precision floating point number */
        void*  Opaque; /**< Pointer to heap-allocated data (String, Function, Class, etc.) */
    } Value;
    // GC
    Value* Next;   /**< Next value in the GC tracking list */
    int    Marked; /**< GC mark flag (0 = unmarked, 1 = marked) */
};

// -----------------------------------------------------------------------------
// Bytecode & Execution
// -----------------------------------------------------------------------------

/**
 * @enum opcode_enum
 * @brief Enumeration of all possible opcode types for the virtual machine.
 * 
 * Defines the complete instruction set for the bytecode interpreter.
 * Instructions are organized by category: loading, array/object operations,
 * function operations, arithmetic, comparison, control flow, and stack
 * manipulation.
 */
typedef enum opcode_enum {
    OP_IMPORT_CORE,                  /**< Import core library */
    OP_LOAD_CAPTURE,                 /**< Load a captured variable */
    OP_LOAD_NAME,                    /**< Load a variable by name */
    OP_LOAD_LOCAL,                   /**< Load a local variable */
    OP_LOAD_CONST,                   /**< Load a constant value (takes 4 byte arg) */
    OP_LOAD_BOOL,                    /**< Load a boolean value (takes 4 byte arg) */
    OP_LOAD_NULL,                    /**< Load null value */
    OP_LOAD_STRING,                  /**< Load a string literal */
    OP_ARRAY_EXTEND,                 /**< Extend an array */
    OP_ARRAY_PUSH,                   /**< Push to an array */
    OP_ARRAY_MAKE,                   /**< Create a new array */
    OP_OBJECT_EXTEND,                /**< Extend an object */
    OP_OBJECT_PLUCK_ATTRIBUTE,       /**< Extract attribute from object */
    OP_OBJECT_MAKE,                  /**< Create a new object */
    OP_CLASS_EXTEND,                 /**< Extend a class */
    OP_CLASS_MAKE,                   /**< Create a new class */
    OP_CLASS_DEFINE_STATIC_MEMBER,   /**< Define a static class member */
    OP_CLASS_DEFINE_INSTANCE_MEMBER, /**< Define an instance class member */
    OP_SET_INDEX,                    /**< Set value at index */
    OP_GET_INDEX,                    /**< Get value at index */
    OP_LOAD_FUNCTION_CLOSURE,        /**< Load a function closure */
    OP_LOAD_FUNCTION,                /**< Load a function */
    OP_CALL_CTOR,                    /**< Call a constructor */
    OP_CALL,                         /**< Call a function */
    OP_CALL_METHOD,                  /**< Call a method */
    OP_NOT,                          /**< Logical NOT */
    OP_POS,                          /**< Unary plus */
    OP_NEG,                          /**< Unary minus */
    OP_AWAIT,                        /**< Await a promise */
    OP_GET_AWAITED_VALUE,            /**< Get the awaited value */
    OP_MUL,                          /**< Multiply */
    OP_DIV,                          /**< Divide */
    OP_MOD,                          /**< Modulo */
    OP_INC,                          /**< Increment */
    OP_POSTINC,                      /**< Post-increment */
    OP_ADD,                          /**< Add */
    OP_DEC,                          /**< Decrement */
    OP_POSTDEC,                      /**< Post-decrement */
    OP_SUB,                          /**< Subtract */
    OP_LSHFT,                        /**< Left shift */
    OP_RSHFT,                        /**< Right shift */
    OP_LT,                           /**< Less than */
    OP_LTE,                          /**< Less than or equal */
    OP_GT,                           /**< Greater than */
    OP_GTE,                          /**< Greater than or equal */
    OP_EQ,                           /**< Equality */
    OP_NE,                           /**< Inequality */
    OP_AND,                          /**< Bitwise AND */
    OP_OR,                           /**< Bitwise OR */
    OP_XOR,                          /**< Bitwise XOR */
    OP_STORE_CAPTURE,                /**< Store to captured variable */
    OP_STORE_NAME,                   /**< Store to named variable */
    OP_STORE_LOCAL,                  /**< Store to local variable */
    OP_DUPTOP,                       /**< Duplicate top of stack */
    OP_DUP2,                         /**< Duplicate top 2 stack items */
    OP_POPTOP,                       /**< Pop top of stack */
    OP_ROT2,                         /**< Rotate top 2 stack items */
    OP_ROT3,                         /**< Rotate top 3 stack items */
    OP_ROT4,                         /**< Rotate top 4 stack items */
    OP_SETUP_TRY,                    /**< Setup try-catch block */
    OP_POP_TRY,                      /**< Pop try block */
    OP_POPN_TRY,                     /**< Pop N items from try stack */
    OP_ENTER_SCOPE,                  /**< Enter a new scope */
    OP_EXIT_SCOPE,                   /**< Exit current scope */
    OP_EXITN_SCOPE,                  /**< Exit N scopes */
    OP_JUMP_IF_FALSE_OR_POP,         /**< Jump if false or pop */
    OP_JUMP_IF_TRUE_OR_POP,          /**< Jump if true or pop */
    OP_POP_JUMP_IF_FALSE,            /**< Pop and jump if false */
    OP_POP_JUMP_IF_TRUE,             /**< Pop and jump if true */
    OP_JUMP,                         /**< Unconditional jump */
    OP_ABSOLUTE_JUMP,                /**< Absolute jump */
    OP_RETURN                        /**< Return from function */
} OpcodeEnum;

// -----------------------------------------------------------------------------
// Runtime Structures (Functions, Scope, Environment)
// -----------------------------------------------------------------------------

/**
 * @struct capture_meta_struct
 * @brief Metadata for captured variables in closures.
 * 
 * Describes how a variable from an outer scope is captured by a closure,
 * including whether it's global and the source/destination indices.
 */
typedef struct capture_meta_struct {
    int  Depth;    /**< Depth of the captured variable's scope relative to the closure */
    int  Src;      /**< Source index */
    int  Dst;      /**< Destination index */
} CaptureMeta;

/**
 * @struct envcell_struct
 * @brief Represents a cell in the environment (variable storage).
 * 
 * A single variable slot in an environment. Tracks whether the variable
 * has been captured by a closure to enable proper lifetime management.
 */
typedef struct envcell_struct {
    Value* Value;      /**< Pointer to the value stored in the cell */
    bool   IsCaptured; /**< True if this cell is captured by a closure */
    int    RefCount;   /**< Reference count for captured variables (optional, can be used for optimizations) */
} EnvCell;

/**
 * @struct environment_struct
 * @brief Represents an execution environment (scope).
 * 
 * Contains the local variables for a function or block scope, along with
 * a reference to the parent environment for lexical scoping.
 */
typedef struct environment_struct {
    Value*    Parent; /**< Pointer to the parent environment */
    EnvCell** Locals; /**< Array of local variable cells */
    int       LocalC; /**< Count of local variables */
} Environment;

/**
 * @struct line_info_struct
 * @brief Stores source location information for debugging and error reporting.
 * 
 * Associates bytecode or AST nodes with their original source file and line
 * number, enabling meaningful error messages and stack traces.
 */
typedef struct line_info_struct {
    String Path; /**< Path to the source file */
    int    Pc;   /**< Program counter */
    int    Line; /**< Line number in the source file */
} LineInfo;

/**
 * @struct user_function_struct
 * @brief Represents a user-defined function in the interpreter.
 * 
 * Contains the compiled bytecode, parameter information, local variable
 * count, and closure captures for a function defined in the source language.
 */
typedef struct user_function_struct {
    Value*       Scope;        /**< The environment where the function was defined */
    String       Name;         /**< Function name (Nullable) */
    bool         Async;        /**< True if function is asynchronous */
    uint8_t*     Codes;        /**< Bytecode instructions */
    size_t       CodeC;        /**< Size of bytecode */
    LineInfo*    Lines;        /**< Line information for each instruction */
    size_t       LineC;        /**< Count of line information */
    int          Argc;         /**< Argument count */
    int          LocalC;       /**< Local variable count */
    CaptureMeta* CaptureMetas; /**< Array of capture metadata */
    int          CaptureC;     /**< Count of captured variables */
    struct envcell_struct** Captures; /**< Array of captured environment cells */
} UserFunction;

/**
 * @struct symbol_struct
 * @brief Represents a symbol table entry in the compiler.
 * 
 * Tracks information about a variable or constant during compilation,
 * including its scope, mutability, and memory location.
 */
typedef struct symbol_struct {
    bool IsGlobal;    /**< True if symbol is global */
    bool IsLocalToFn; /**< True if symbol is local to current function */
    bool IsConstant;  /**< True if symbol is a constant */
    int  Offset;      /**< Memory offset or index */
} Symbol;

/**
 * @enum scope_type_enum
 * @brief Enumeration of all possible scope types.
 * 
 * Defines the different kinds of scopes that can exist during compilation,
 * which affects variable resolution and control flow handling.
 */
typedef enum scope_type_enum {
    SCOPE_GLOBAL,           /**< Global scope */
    SCOPE_CLASS ,           /**< Class scope */
    SCOPE_FUNCTION,         /**< Function body scope */
    SCOPE_BLOCK,            /**< Generic block scope */
    SCOPE_NEW,              /**< Scope for new block with temporary variable such as loop or catch */
    SCOPE_TRY_BLOCK,        /**< Try block scope */
    SCOPE_LOOP,             /**< Loop scope */
} ScopeType;

/**
 * @struct scope_struct
 * @brief Represents a compilation scope.
 * 
 * Maintains symbol tables and control flow information during compilation.
 * Scopes are nested and linked through the Parent pointer. Loop scopes
 * track jump locations for break and continue statements.
 */
typedef struct scope_struct Scope;
struct scope_struct {
    ScopeType Type;          /**< Type of the scope */
    Scope*    Parent;        /**< Parent scope */
    HashMap*  Symbols;       /**< Symbol table for this scope */
    HashMap*  Captures;      /**< Captured variables in this scope */
    // FN
    bool      Returned;      /**< True if function returns in this scope */
    // Loop
    int*      ContinueJumps; /**< Array of jump locations for continue statements */
    int       ContinueJumpC; /**< Count of continue jumps */
    int*      BreakJumps;    /**< Array of jump locations for break statements */
    int       BreakJumpC;    /**< Count of break jumps */
};

// -----------------------------------------------------------------------------
// Classes, Modules & Native Functions
// -----------------------------------------------------------------------------

/**
 * @struct user_class_struct
 * @brief Represents a user-defined class in the interpreter.
 * 
 * Contains the class name, optional base class for inheritance, and
 * separate hash maps for static and instance members.
 */
typedef struct user_class_struct {
    String   Name;            /**< Class name */
    Value*   Base;            /**< Base class (must be Class or Nullable) */
    HashMap* StaticMembers;   /**< Static members map */
    HashMap* InstanceMembers; /**< Instance members map */
} Class;

/**
 * @struct class_instance_struct
 * @brief Represents an instance of a user-defined class.
 * 
 * Links to the class prototype and maintains instance-specific member values.
 */
typedef struct class_instance_struct {
    Value*   Proto;   /**< Prototype class (must be Class) */
    HashMap* Members; /**< Instance members map */
} ClassInstance;

/**
 * @struct interpreter_struct
 * @brief Forward declaration of Interpreter structure for NativeFunctionCallback typedef.
 */
typedef struct interpreter_struct Interpreter;

/**
 * @typedef NativeFunctionCallback
 * @brief Function pointer type for native functions.
 * 
 * Native functions are implemented in C and callable from the interpreted
 * language. They receive the interpreter context, argument count, and
 * argument array, and return a Value pointer.
 * 
 * @param interpreter Pointer to the interpreter instance.
 * @param argc Number of arguments.
 * @param arguments Array of argument values.
 * @return Value* The return value of the function.
 */
typedef Value* (*NativeFunctionCallback)(Interpreter* interpreter, int argc, Value** arguments);

/**
 * @struct native_function_struct
 * @brief Represents a native (C-implemented) function metadata.
 * 
 * Wraps a C function pointer with metadata about the function's name
 * and expected argument count.
 */
typedef struct native_function_struct {
    String         Name;    /**< Function name */
    int            Argc;    /**< Expected argument count */
    NativeFunctionCallback FuncPtr; /**< Pointer to the C function */
} NativeFunction;

/**
 * @struct module_function_struct
 * @brief Represents a function or value exported by a native module.
 * 
 * Native modules can export both functions and constant values. This
 * structure describes a single export with its name and either a function
 * pointer or a value pointer.
 */
typedef struct module_function_struct {
    const String    Name;      /**< Export name */
    int             Argc;      /**< Argument count (for functions) */
    NativeFunctionCallback  CFunction; /**< C function pointer (if is NativeFunctionCallback) */
    Value*          Value;     /**< Exported value (if not function) */
} ModuleFunction;

// -----------------------------------------------------------------------------
// Main State Machines (Interpreter & Compiler)
// -----------------------------------------------------------------------------

/**
 * @struct interpreter_struct
 * @brief Main interpreter state structure containing execution context.
 * 
 * The central runtime state including the execution stack, garbage collector
 * root, constant pool, function table, and singleton values for true, false,
 * and null. Also maintains exception handler stack for try-catch blocks.
 */
struct interpreter_struct {
    bf_context_t BfContext;                              /**< Context for the libbf library (memory management, etc.) */
    HashMap*     Imports;                                /**< Imports map */
    Value*       Array;                                  /**< Built-in Array class */
    Value*       Promise;                                /**< Built-in Promise class */
    Value*       True;                                   /**< Singleton 'true' value */
    Value*       False;                                  /**< Singleton 'false' value */
    Value*       Null;                                   /**< Singleton 'null' value */
    Value*       GcRoot;                                 /**< Root of the Garbage Collector object graph */
    Value*       RootEnv;                                /**< Root environment of the current program */
    Value*       CallEnv;                                /**< Current execution environment */
    int          Allocated;                              /**< Total allocated bytes since last GC */
    Value**      Constants;                              /**< Array of constant values */
    int          ConstantC;                              /**< Count of constants */
    Value**      Functions;                              /**< Array of function definitions */
    int          FunctionC;                              /**< Count of functions */
    Value*       Stacks[STACK_SIZE];                     /**< Execution stack */
    int          StackC;                                 /**< Stack pointer/count */
    Value*       Envs[STACK_SIZE];                       /**< Environment stack for variable scopes */
    int          EnvC;                                   /**< Environment stack pointer */
    int          ExceptionHandlerStacks[STACK_SIZE];     /**< Stack for exception handlers */
    int          ExceptionHandlerStackC;                 /**< Exception handler stack pointer */
    int          GcThreshold;                            /**< Threshold for triggering garbage collection */
    Value*       TaskQueue[STACK_SIZE];                  /**< Queue for pending tasks (e.g. resolved promises) */
    int          TaskQueueHead;                          /**< Head index (next item to dequeue) */
    int          TaskQueueC;                             /**< Count of pending tasks in the task queue */
};

/**
 * @struct compiler_struct
 * @brief Compiler state structure holding parser and interpreter references.
 * 
 * Links the compilation process to both the parser (for reading source code)
 * and the interpreter (for registering constants and functions).
 */
typedef struct compiler_struct {
    Interpreter* Interpreter; /**< Pointer to the interpreter */
    String       ModulePath;  /**< Path to the module */
    Parser*      Parser;      /**< Pointer to the parser */
} Compiler;

/**
 * @enum StateMachineState
 * @brief Enumeration representing the possible states of a state machine.
 * 
 * @var PENDING
 *      State indicating that the operation is still in progress.
 * @var COMPLETE
 *      State indicating that the operation has completed successfully.
 * @var REJECTED
 *      State indicating that the operation has failed or been rejected.
 */
typedef enum state_machine_state_enum {
    PENDING,
    FULFILLED,
    REJECTED,
} StateMachineState;

/**
 * @struct StateMachine
 * @brief Represents a state machine for managing asynchronous operations.
 * 
 * @var StateMachine::State
 *      The current state of the state machine (PENDING, COMPLETE, or REJECTED).
 * @var StateMachine::WaitFor
 *     Pointer to a value that the state machine is waiting on (e.g. a promise).
 * @var StateMachine::Value
 *      Pointer to the resulting value produced by the state machine operation.
 * @var StateMachine::Then
 *      Pointer to a callback value to be executed upon successful completion.
 * @var StateMachine::Catch
 *      Pointer to a callback value to be executed upon rejection or error.
 * @var StateMachine::Function
 *      Pointer to the primary function or operation being executed by the state machine.
 * @var StateMachine::Ip
 *      Instruction pointer; tracks the execution position within the state machine.
 */
typedef struct state_machine_struct {
    StateMachineState State;
    bool              IsCallback;
    Value*            CallEnv;
    Value*            WaitFor;
    Value*            Value;
    Value*            Then;
    Value*            Catch;
    Value*            Function;
    size_t            Ip;
    Value**           WaitList;
    size_t            WaitListC;
} StateMachine;

// -----------------------------------------------------------------------------
// Memory Allocation & Utilities
// -----------------------------------------------------------------------------

/**
 * @def Allocate
 * @brief Wrapper macro for memory allocation with file/line tracking.
 * 
 * Provides debugging information by recording the source location of
 * each allocation.
 * 
 * @param size Size in bytes to allocate.
 */
#define Allocate(size) _Allocate(__FILE__, __LINE__, size)

/**
 * @def Callocate
 * @brief Wrapper macro for zero-initialized memory allocation with tracking.
 * 
 * Allocates memory and initializes it to zero, with debugging information.
 * 
 * @param count Number of elements.
 * @param size Size of each element.
 */
#define Callocate(count, size) _Callocate(__FILE__, __LINE__, count, size)

/**
 * @def Reallocate
 * @brief Wrapper macro for memory reallocation with tracking.
 * 
 * Resizes an existing memory block, with debugging information.
 * 
 * @param ptr Pointer to existing memory block.
 * @param size New size in bytes.
 */
#define Reallocate(ptr, size) _Reallocate(__FILE__, __LINE__, ptr, size)

/**
 * @brief Internal allocation function.
 * 
 * Allocates memory and tracks the allocation location for debugging.
 * Terminates the program if allocation fails.
 * 
 * @param file Source file name calling allocation.
 * @param line Line number calling allocation.
 * @param size Size in bytes.
 * @return Pointer to allocated memory.
 */
void* _Allocate(String file, int line, size_t size);

/**
 * @brief Internal zero-initialized allocation function.
 * 
 * Allocates memory, initializes it to zero, and tracks the allocation
 * location for debugging. Terminates the program if allocation fails.
 * 
 * @param file Source file name calling allocation.
 * @param line Line number calling allocation.
 * @param count Number of elements.
 * @param size Size of each element.
 * @return Pointer to allocated memory.
 */
void* _Callocate(String file, int line, size_t count, size_t size);

/**
 * @brief Internal reallocation function.
 * 
 * Resizes an existing memory block and tracks the allocation location
 * for debugging. Terminates the program if reallocation fails.
 * 
 * @param file Source file name calling allocation.
 * @param line Line number calling allocation.
 * @param ptr Pointer to existing memory.
 * @param size New size in bytes.
 * @return Pointer to reallocated memory.
 */
void* _Reallocate(String file, int line, void* ptr, size_t size);

// String Utilities

/**
 * @brief Duplicates a C string using the custom allocator.
 * 
 * Creates a new copy of the given string on the heap using the tracked
 * allocation system.
 * 
 * @param str The string to duplicate.
 * @return A new copy of the string.
 */
String AllocateString(String str);

/**
 * @brief Converts a UTF-8 string to an array of Runes (UTF-32).
 * 
 * Decodes a UTF-8 encoded string into an array of Unicode code points.
 * The returned array is null-terminated.
 * 
 * @param str The UTF-8 string.
 * @return Pointer to the array of Runes.
 */
Rune* StringToRunes(String str);

/**
 * @brief Converts an array of Runes back to a UTF-8 string.
 * 
 * Encodes an array of Unicode code points into a UTF-8 string.
 * 
 * @param runes The array of Runes.
 * @return A new UTF-8 string.
 */
String RunesStrToString(Rune* runes);

/**
 * @brief Checks if a string starts with a given prefix.
 * 
 * Performs a case-sensitive prefix comparison.
 * 
 * @param str The string to check.
 * @param prefix The prefix to look for.
 * @return true if str starts with prefix, false otherwise.
 */
bool StringStartsWith(String str, String prefix);

// Type Coercion

/**
 * @brief Coerces a value to a 32-bit integer.
 * 
 * Converts a runtime value to an integer, performing type conversion
 * as necessary. Behavior depends on the value's type.
 * 
 * @param value The value to coerce.
 * @return The integer representation.
 */
int CoerceToI32(Value* value);

/**
 * @brief Coerces a value to a 64-bit integer (long).
 * 
 * Converts a runtime value to a long integer, performing type conversion
 * as necessary. Behavior depends on the value's type.
 * 
 * @param value The value to coerce.
 * @return The long integer representation.
 */
long CoerceToI64(Value* value);

/**
 * @brief Coerces a value to a double-precision number.
 * 
 * Converts a runtime value to a double, performing type conversion
 * as necessary. Behavior depends on the value's type.
 * 
 * @param value The value to coerce.
 * @return The double representation.
 */
double CoerceToNum(Value* value);

/**
 * @brief Coerces a value to a big integer (bf_t).
 * 
 * Converts a runtime value to a big integer, performing type conversion
 * as necessary. Behavior depends on the value's type. Uses the libbf
 * library for big integer representation.
 * 
 * @param interp Pointer to the interpreter instance.
 * @param value The value to coerce.
 * @return Pointer to the big integer representation.
 */
bf_t* CoerceToBitField(Interpreter* interp, Value* value);

/**
 * @brief Gets the precession (number of decimal places) for a big numeric value.
 * 
 * @param value The big numeric value.
 * @return The precession.
 */
limb_t BFPrecession(Value* value);

/**
 * @brief Coerces a value to a boolean.
 * 
 * Converts a runtime value to a boolean, following standard truthiness
 * rules. Behavior depends on the value's type.
 * 
 * @param value The value to coerce.
 * @return The boolean representation.
 */
bool CoerceToBool(Value* value);

/**
 * @brief Coerces a value to an Environment.
 * 
 * Extracts the Environment pointer from a value. The value must be
 * of type VLT_ENVIRONMENT.
 * 
 * @param value The value to coerce.
 * @return Pointer to the Environment.
 */
Environment* CoerceToEnvironment(Value* value);

/**
 * @brief Coerces a value to a HashMap (object).
 * 
 * Extracts the HashMap pointer from a value. The value must be
 * of type VLT_OBJECT.
 * 
 * @param value The value to coerce.
 * @return Pointer to the HashMap.
 */
HashMap* CoerceToHashMap(Value* value);

/**
 * @brief Coerces a value to an Array.
 * 
 * Extracts the Array pointer from a value. The value must be
 * of type VLT_ARRAY.
 * 
 * @param value The value to coerce.
 * @return Pointer to the Array.
 */
Array* CoerceToArray(Value* value);

/**
 * @brief Coerces a value to a UserFunction.
 * 
 * Extracts the UserFunction pointer from a value. The value must be
 * of type VLT_USER_FUNCTION.
 * 
 * @param value The value to coerce.
 * @return Pointer to the UserFunction.
 */
UserFunction* CoerceToUserFunction(Value* value);

/**
 * @brief Coerces a value to a NativeFunction.
 * 
 * Extracts the NativeFunction pointer from a value. The value must be
 * of type VLT_NATV_FUNCTION.
 * 
 * @param value The value to coerce.
 * @return Pointer to the NativeFunction.
 */
NativeFunction* CoerceToNativeFunction(Value* value);

/**
 * @brief Coerces a value to a Class.
 * 
 * Extracts the Class pointer from a value. The value must be
 * of type VLT_CLASS.
 * 
 * @param value The value to coerce.
 * @return Pointer to the Class.
 */
Class* CoerceToUserClass(Value* value);

/**
 * @brief Coerces a value to a ClassInstance.
 * 
 * Extracts the ClassInstance pointer from a value. The value must be
 * of type VLT_CLASS_INSTANCE.
 * 
 * @param value The value to coerce.
 * @return Pointer to the ClassInstance.
 */
ClassInstance* CoerceToClassInstance(Value* value);

/**
 * @brief Coerces a value to a StateMachine.
 * 
 * Extracts the StateMachine pointer from a value. The value must be
 * of type VLT_CLASS_INSTANCE and its prototype must be the built-in
 * StateMachine class.
 * 
 * @param value The value to coerce.
 * @return Pointer to the StateMachine.
 */
StateMachine* CoerceToStateMachine(Value* value);

// Error Handling

/**
 * @brief Formats an error message with location information.
 * 
 * Creates a formatted error message that includes the source file path,
 * line number, column number, and a snippet of the source code where
 * the error occurred.
 * 
 * @param path File path where error occurred.
 * @param runes Source code runes.
 * @param position Token position.
 * @param message Error message.
 * @return Formatted error string.
 */
String GetErrorLine(String path, Rune* runes, Position position, String message);

/**
 * @brief Throws a runtime error (prints and exits).
 * 
 * Prints a formatted error message with source location information
 * and terminates the program.
 * 
 * @param path File path where error occurred.
 * @param runes Source code runes.
 * @param position Token position.
 * @param message Error message.
 */
void ThrowError(String path, Rune* runes, Position position, String message);

/**
 * @brief Formats a string with variable arguments.
 * 
 * Similar to sprintf but allocates a new string to hold the result.
 * Uses the tracked allocation system.
 * 
 * @param format The format string.
 * @param ... Variable arguments.
 * @return Formatted string.
 */
String FormatString(String format, ...);

/**
 * @brief Converts a big integer value to a string.
 * 
 * Uses the libbf library to convert a big integer (bf_t) to its string
 * representation. The returned string is allocated using the tracked
 * allocation system.
 * 
 * @param value Pointer to the big integer value.
 * @return String representation of the big integer.
 */
String BFIntToString(bf_t* value);

/**
 * @brief Converts a big integer value to a string.
 * 
 * Uses the libbf library to convert a big integer (bf_t) to its string
 * representation. The returned string is allocated using the tracked
 * allocation system.
 * 
 * @param value Pointer to the big integer value.
 * @return String representation of the big integer.
 */
String BFNumToString(bf_t* value);

#endif
