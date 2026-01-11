/**
 * @file astnode.h
 * @brief Abstract Syntax Tree (AST) node creation and management interface
 *
 * This header defines the interface for creating and managing AST nodes that
 * represent the syntactic structure of parsed source code. Each function creates
 * a specific type of AST node for different language constructs.
 */

#include "./global.h"

#ifndef ASTNODE_H
#define ASTNODE_H

/**
 * @brief Creates an AST node representing an identifier/name
 * 
 * Allocates and initializes an AST_NAME node that represents a variable,
 * function, or other identifier in the source code.
 * 
 * @param[in] name String value of the identifier
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_NAME node, or NULL on allocation failure
 */
Ast* AstName(String name, Position position);

/**
 * @brief Creates an AST node representing an integer literal
 * 
 * Allocates and initializes an AST_INT node that represents an integer
 * constant in the source code.
 * 
 * @param[in] value String representation of the integer value
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_INT node, or NULL on allocation failure
 */
Ast* AstInteger(String value, Position position);

/**
 * @brief Creates an AST node representing a floating-point number literal
 * 
 * Allocates and initializes an AST_NUM node that represents a floating-point
 * constant in the source code.
 * 
 * @param[in] value String representation of the number value
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_NUM node, or NULL on allocation failure
 */
Ast* AstNumber(String value, Position position);

/**
 * @brief Creates an AST node representing a string literal
 * 
 * Allocates and initializes an AST_STR node that represents a string
 * constant in the source code.
 * 
 * @param[in] value String content of the literal
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_STR node, or NULL on allocation failure
 */
Ast* AstString(String value, Position position);

/**
 * @brief Creates an AST node representing a boolean literal
 * 
 * Allocates and initializes an AST_BOOL node that represents a boolean
 * constant (true or false) in the source code.
 * 
 * @param[in] value Boolean value (true or false)
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_BOOL node, or NULL on allocation failure
 */
Ast* AstBool(bool value, Position position);

/**
 * @brief Creates an AST node representing a null literal
 * 
 * Allocates and initializes an AST_NULL node that represents a null
 * constant in the source code.
 * 
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_NULL node, or NULL on allocation failure
 */
Ast* AstNull(Position position);

/**
 * @brief Creates an AST node representing a spread operator
 * 
 * Allocates and initializes an AST_SPREAD_OPERATOR node that represents
 * the spread operator (...) applied to an expression.
 * 
 * @param[in] expression Pointer to the expression AST node to spread
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_SPREAD_OPERATOR node, or NULL on allocation failure
 */
Ast* AstSpread(Ast* expression, Position position);

/**
 * @brief Creates an AST node representing a list literal
 * 
 * Allocates and initializes an AST_LIST_LITERAL node that represents
 * an array or list literal expression.
 * 
 * @param[in] elements Pointer to AST node containing the list elements
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_LIST_LITERAL node, or NULL on allocation failure
 */
Ast* AstListLiteral(Ast* elements, Position position);

/**
 * @brief Creates an AST node representing a key-value pair in an object
 * 
 * Allocates and initializes an AST_OBJECT_KEY_VAL node that represents
 * a single property definition in an object literal.
 * 
 * @param[in] key Pointer to the key AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_OBJECT_KEY_VAL node, or NULL on allocation failure
 */
Ast* AstObjectKeyVal(Ast* key, Position position);

/**
 * @brief Creates an AST node representing an object literal
 * 
 * Allocates and initializes an AST_OBJECT_LITERAL node that represents
 * an object literal expression with properties.
 * 
 * @param[in] properties Pointer to AST node containing the object properties
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_OBJECT_LITERAL node, or NULL on allocation failure
 */
Ast* AstObjectLiteral(Ast* properties, Position position);

/**
 * @brief Creates an AST node representing an allocation expression
 * 
 * Allocates and initializes an AST_ALLOCATION node that represents
 * a class instantiation (new) expression.
 * 
 * @param[in] cls Pointer to the class AST node
 * @param[in] arguments Pointer to the constructor arguments AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_ALLOCATION node, or NULL on allocation failure
 */
Ast* AstAllocation(Ast* cls, Ast* arguments, Position position);

/**
 * @brief Creates an AST node representing a member access
 * 
 * Allocates and initializes an AST_MEMBER node that represents
 * a property or method access using dot notation (object.member).
 * 
 * @param[in] object Pointer to the object AST node
 * @param[in] member Pointer to the member AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_MEMBER node, or NULL on allocation failure
 */
Ast* AstMember(Ast* object, Ast* member, Position position);

/**
 * @brief Creates an AST node representing an index access
 * 
 * Allocates and initializes an AST_INDEX node that represents
 * an array or object index access using bracket notation (object[index]).
 * 
 * @param[in] object Pointer to the object AST node
 * @param[in] index Pointer to the index AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_INDEX node, or NULL on allocation failure
 */
Ast* AstIndex(Ast* object, Ast* index, Position position);

/**
 * @brief Creates an AST node representing a function call
 * 
 * Allocates and initializes an AST_CALL node that represents
 * a function or method invocation.
 * 
 * @param[in] object Pointer to the callable object AST node
 * @param[in] arguments Pointer to the arguments AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_CALL node, or NULL on allocation failure
 */
Ast* AstCall(Ast* object, Ast* arguments, Position position);

/**
 * @brief Creates an AST node representing a unary or postfix operation
 * 
 * Allocates and initializes an AST_UNARY node that represents
 * a unary operation (e.g., negation, logical NOT, increment, decrement).
 * 
 * @param[in] type The type of unary operation (e.g., negation, logical NOT)
 * @param[in] operand Pointer to the operand AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_UNARY node, or NULL on allocation failure
 */
Ast* AstSingle(AstType type, Ast* operand, Position position);

/**
 * @brief Creates an AST node representing a binary operation
 * 
 * Allocates and initializes an AST_BINARY node that represents
 * a binary operation (e.g., addition, multiplication, comparison).
 * 
 * @param[in] type The type of binary operation (e.g., addition, multiplication)
 * @param[in] lhs Pointer to the left-hand side operand AST node
 * @param[in] rhs Pointer to the right-hand side operand AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_BINARY node, or NULL on allocation failure
 */
Ast* AstBinary(AstType type, Ast* lhs, Ast* rhs, Position position);

/**
 * @brief Creates an AST node representing a continue statement
 * 
 * Allocates and initializes an AST_CONTINUE node that represents
 * a continue statement in a loop.
 * 
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_CONTINUE node, or NULL on allocation failure
 */
Ast* AstContinue(Position position);

/**
 * @brief Creates an AST node representing a break statement
 * 
 * Allocates and initializes an AST_BREAK node that represents
 * a break statement in a loop or switch.
 * 
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_BREAK node, or NULL on allocation failure
 */
Ast* AstBreak(Position position);

/**
 * @brief Creates an AST node representing a return statement
 * 
 * Allocates and initializes an AST_RETURN node that represents
 * a return statement with an optional expression.
 * 
 * @param[in] expression Pointer to the expression AST node, or NULL for empty return
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_RETURN node, or NULL on allocation failure
 */
Ast* AstReturn(Ast* expression, Position position);

/**
 * @brief Creates an AST node representing an expression statement
 * 
 * Allocates and initializes an AST_EXPRESSION_STATEMENT node that represents
 * an expression used as a statement.
 * 
 * @param[in] expression Pointer to the expression AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_EXPRESSION_STATEMENT node, or NULL on allocation failure
 */
Ast* AstExpressionStatement(Ast* expression, Position position);

/**
 * @brief Creates an AST node representing a class member
 * 
 * Allocates and initializes an AST_CLASS_MEMBER node that represents
 * a member (property or method) definition within a class, which may be static.
 * 
 * @param[in] _static_ Boolean indicating if the member is static
 * @param[in] node Pointer to the member AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_CLASS_MEMBER node, or NULL on allocation failure
 */
Ast* AstClassMember(bool _static_, Ast* node, Position position);

/**
 * @brief Creates an AST node representing a class definition
 * 
 * Allocates and initializes an AST_CLASS node that represents
 * a class declaration with optional inheritance.
 * 
 * @param[in] name Pointer to AST node containing the class name
 * @param[in] super Pointer to AST node containing the superclass, or NULL if no inheritance
 * @param[in] body Pointer to AST node containing the class body
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_CLASS node, or NULL on allocation failure
 */
Ast* AstClass(Ast* name, Ast* super, Ast* body, Position position);

/**
 * @brief Creates an AST node representing a function definition
 * 
 * Allocates and initializes an AST_FUNCTION node that represents
 * a function declaration with parameters and body.
 * 
 * @param[in] fnName Pointer to AST node containing the function name
 * @param[in] parameters Pointer to AST node containing the function parameters
 * @param[in] body Pointer to AST node containing the function body
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_FUNCTION node, or NULL on allocation failure
 */
Ast* AstFunction(Ast* fnName, Ast* parameters, Ast* body, Position position);

/**
 * @brief Creates an AST node representing an import statement
 * 
 * Allocates and initializes an AST_IMPORT node that represents
 * a module import declaration.
 * 
 * @param[in] imports Pointer to AST node containing the list of imports
 * @param[in] moduleName Pointer to AST node containing the module name
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_IMPORT node, or NULL on allocation failure
 */
Ast* AstImport(Ast* imports, Ast* moduleName, Position position);

/**
 * @brief Creates an AST node representing a variable declaration
 * 
 * Allocates and initializes an AST_VAR_DECLARATION node that represents
 * one or more variable declarations (var, let, const).
 * 
 * @param[in] type The type of the variable declaration (e.g., var, let, const)
 * @param[in] declarations Pointer to AST node containing the variable declarations
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_VAR_DECLARATION node, or NULL on allocation failure
 */
Ast* AstVarDeclaration(AstType type, Ast* declarations, Position position);

/**
 * @brief Creates an AST node representing an if statement
 * 
 * Allocates and initializes an AST_IF node that represents
 * a conditional statement with optional else branch.
 * 
 * @param[in] condition Pointer to the condition AST node
 * @param[in] thenBranch Pointer to the then branch AST node
 * @param[in] elseBranch Pointer to the else branch AST node, or NULL if no else
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_IF node, or NULL on allocation failure
 */
Ast* AstIf(Ast* condition, Ast* thenBranch, Ast* elseBranch, Position position);

/**
 * @brief Creates an AST node representing a for statement
 * 
 * Allocates and initializes an AST_FOR node that represents
 * a for loop with initializer, condition, and mutator.
 * 
 * @param[in] initializerConditionMutator Pointer to the initializer, condition, and mutator AST node
 * @param[in] body Pointer to the body AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_FOR node, or NULL on allocation failure
 */
Ast* AstFor(Ast* initializerConditionMutator, Ast* body, Position position);

/**
 * @brief Creates an AST node representing a while statement
 * 
 * Allocates and initializes an AST_WHILE node that represents
 * a while loop with condition and body.
 * 
 * @param[in] condition Pointer to the condition AST node
 * @param[in] body Pointer to the body AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_WHILE node, or NULL on allocation failure
 */
Ast* AstWhile(Ast* condition, Ast* body, Position position);

/**
 * @brief Creates an AST node representing a do-while statement
 * 
 * Allocates and initializes an AST_DO_WHILE node that represents
 * a do-while loop with body and condition.
 * 
 * @param[in] condition Pointer to the condition AST node
 * @param[in] body Pointer to the body AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_DO_WHILE node, or NULL on allocation failure
 */
Ast* AstDoWhile(Ast* condition, Ast* body, Position position);

/**
 * @brief Creates an AST node representing a try-catch statement
 * 
 * Allocates and initializes an AST_TRY_CATCH node that represents
 * exception handling with try and catch blocks.
 * 
 * @param[in] tryBlock Pointer to the try block AST node
 * @param[in] errorName Pointer to the error name AST node for the catch clause
 * @param[in] catchBlock Pointer to the catch block AST node
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_TRY_CATCH node, or NULL on allocation failure
 */
Ast* AstTryCatch(Ast* tryBlock, Ast* errorName,  Ast* catchBlock, Position position);

/**
 * @brief Creates an AST node representing a block statement
 * 
 * Allocates and initializes an AST_BLOCK node that represents
 * a block of statements enclosed in braces.
 * 
 * @param[in] statements Pointer to child AST nodes (statements)
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_BLOCK node, or NULL on allocation failure
 */
Ast* AstBlock(Ast* statements, Position position);

/**
 * @brief Creates an AST node representing the root program node
 * 
 * Allocates and initializes an AST_PROGRAM node that represents
 * the top-level program containing all statements and declarations.
 * 
 * @param[in] children Pointer to child AST nodes (statements/declarations)
 * @param[in] position Source code location information
 * @return Pointer to newly allocated AST_PROGRAM node, or NULL on allocation failure
 */
Ast* AstProgram(Ast* children, Position position);

/**
 * @brief Frees an AST node and all its children
 * 
 * Recursively deallocates memory for an AST node and all of its
 * child nodes, preventing memory leaks.
 * 
 * @param[in,out] ast Pointer to the AST node to free
 */
void FreeAst(Ast* ast);

#endif /* ASTNODE_H */