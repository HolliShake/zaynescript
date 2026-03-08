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
 * Creates an AST node representing an identifier/name.
 * 
 * Allocates and initializes an AST_NAME node that represents a variable,
 * function, or other identifier in the source code.
 * 
 * @param name String value of the identifier.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_NAME node, or NULL on allocation failure.
 */
Ast* AstName(String name, Position position);

/**
 * Creates an AST node representing an integer literal.
 * 
 * Allocates and initializes an AST_INT node that represents an integer
 * constant in the source code.
 * 
 * @param value String representation of the integer value.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_INT node, or NULL on allocation failure.
 */
Ast* AstInteger(String value, Position position);

/**
 * Creates an AST node representing a floating-point number literal.
 * 
 * Allocates and initializes an AST_NUM node that represents a floating-point
 * constant in the source code.
 * 
 * @param value String representation of the number value.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_NUM node, or NULL on allocation failure.
 */
Ast* AstNumber(String value, Position position);

/**
 * Creates an AST node representing a string literal.
 * 
 * Allocates and initializes an AST_STR node that represents a string
 * constant in the source code.
 * 
 * @param value String content of the literal.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_STR node, or NULL on allocation failure.
 */
Ast* AstString(String value, Position position);

/**
 * Creates an AST node representing a boolean literal.
 * 
 * Allocates and initializes an AST_BOOL node that represents a boolean
 * constant (true or false) in the source code.
 * 
 * @param value Boolean value (true or false).
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_BOOL node, or NULL on allocation failure.
 */
Ast* AstBool(bool value, Position position);

/**
 * Creates an AST node representing a null literal.
 * 
 * Allocates and initializes an AST_NULL node that represents a null
 * constant in the source code.
 * 
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_NULL node, or NULL on allocation failure.
 */
Ast* AstNull(Position position);

/**
 * Creates an AST node representing the 'this' keyword.
 * 
 * Allocates and initializes an AST_THIS node that represents the 'this'
 * keyword in the source code, typically used within class methods to refer
 * to the current instance.
 * 
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_THIS node, or NULL on allocation failure.
 */
Ast* AstThis(Position position);

/**
 * Creates an AST node representing a spread operator.
 * 
 * Allocates and initializes an AST_SPREAD_OPERATOR node that represents
 * the spread operator (...) applied to an expression.
 * 
 * @param expression Pointer to the expression AST node to spread.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_SPREAD_OPERATOR node, or NULL on allocation failure.
 */
Ast* AstSpread(Ast* expression, Position position);

/**
 * Creates an AST node representing a list literal.
 * 
 * Allocates and initializes an AST_LIST_LITERAL node that represents
 * an array or list literal expression.
 * 
 * @param elements Pointer to AST node containing the list elements.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_LIST_LITERAL node, or NULL on allocation failure.
 */
Ast* AstListLiteral(Ast* elements, Position position);

/**
 * Creates an AST node representing a key-value pair in an object.
 * 
 * Allocates and initializes an AST_OBJECT_KEY_VAL node that represents
 * a single property definition in an object literal.
 * 
 * @param key Pointer to the key AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_OBJECT_KEY_VAL node, or NULL on allocation failure.
 */
Ast* AstObjectKeyVal(Ast* key, Position position);

/**
 * Creates an AST node representing an object literal.
 * 
 * Allocates and initializes an AST_OBJECT_LITERAL node that represents
 * an object literal expression with properties.
 * 
 * @param properties Pointer to AST node containing the object properties.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_OBJECT_LITERAL node, or NULL on allocation failure.
 */
Ast* AstObjectLiteral(Ast* properties, Position position);

/**
 * Creates an AST node representing an allocation expression.
 * 
 * Allocates and initializes an AST_ALLOCATION node that represents
 * a class instantiation (new) expression.
 * 
 * @param cls Pointer to the class AST node.
 * @param arguments Pointer to the constructor arguments AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_ALLOCATION node, or NULL on allocation failure.
 */
Ast* AstAllocation(Ast* cls, Ast* arguments, Position position);

/**
 * Creates an AST node representing a member access.
 * 
 * Allocates and initializes an AST_MEMBER node that represents
 * a property or method access using dot notation (object.member).
 * 
 * @param object Pointer to the object AST node.
 * @param member Pointer to the member AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_MEMBER node, or NULL on allocation failure.
 */
Ast* AstMember(Ast* object, Ast* member, Position position);

/**
 * Creates an AST node representing an index access.
 * 
 * Allocates and initializes an AST_INDEX node that represents
 * an array or object index access using bracket notation (object[index]).
 * 
 * @param object Pointer to the object AST node.
 * @param index Pointer to the index AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_INDEX node, or NULL on allocation failure.
 */
Ast* AstIndex(Ast* object, Ast* index, Position position);

/**
 * Creates an AST node representing a function call.
 * 
 * Allocates and initializes an AST_CALL node that represents
 * a function or method invocation.
 * 
 * @param object Pointer to the callable object AST node.
 * @param arguments Pointer to the arguments AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_CALL node, or NULL on allocation failure.
 */
Ast* AstCall(Ast* object, Ast* arguments, Position position);

/**
 * Creates an AST node representing a unary or postfix operation.
 * 
 * Allocates and initializes an AST_UNARY node that represents
 * a unary operation (e.g., negation, logical NOT, increment, decrement).
 * 
 * @param type The type of unary operation (e.g., negation, logical NOT).
 * @param operand Pointer to the operand AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_UNARY node, or NULL on allocation failure.
 */
Ast* AstSingle(AstType type, Ast* operand, Position position);

/**
 * Creates an AST node representing a binary operation.
 * 
 * Allocates and initializes an AST_BINARY node that represents
 * a binary operation (e.g., addition, multiplication, comparison).
 * 
 * @param type The type of binary operation (e.g., addition, multiplication).
 * @param lhs Pointer to the left-hand side operand AST node.
 * @param rhs Pointer to the right-hand side operand AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_BINARY node, or NULL on allocation failure.
 */
Ast* AstBinary(AstType type, Ast* lhs, Ast* rhs, Position position);

/**
 * Creates an AST node representing a continue statement.
 * 
 * Allocates and initializes an AST_CONTINUE node that represents
 * a continue statement in a loop.
 * 
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_CONTINUE node, or NULL on allocation failure.
 */
Ast* AstContinue(Position position);

/**
 * Creates an AST node representing a break statement.
 * 
 * Allocates and initializes an AST_BREAK node that represents
 * a break statement in a loop or switch.
 * 
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_BREAK node, or NULL on allocation failure.
 */
Ast* AstBreak(Position position);

/**
 * Creates an AST node representing a return statement.
 * 
 * Allocates and initializes an AST_RETURN node that represents
 * a return statement with an optional expression.
 * 
 * @param expression Pointer to the expression AST node, or NULL for empty return.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_RETURN node, or NULL on allocation failure.
 */
Ast* AstReturn(Ast* expression, Position position);

/**
 * Creates an AST node representing an expression statement.
 * 
 * Allocates and initializes an AST_EXPRESSION_STATEMENT node that represents
 * an expression used as a statement.
 * 
 * @param expression Pointer to the expression AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_EXPRESSION_STATEMENT node, or NULL on allocation failure.
 */
Ast* AstExpressionStatement(Ast* expression, Position position);

/**
 * Creates an AST node representing a class member.
 * 
 * Allocates and initializes an AST_CLASS_MEMBER node that represents
 * a member (property or method) definition within a class, which may be static.
 * 
 * @param _static_ Boolean indicating if the member is static.
 * @param node Pointer to the member AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_CLASS_MEMBER node, or NULL on allocation failure.
 */
Ast* AstClassMember(bool _static_, Ast* node, Position position);

/**
 * Creates an AST node representing a class definition.
 * 
 * Allocates and initializes an AST_CLASS node that represents
 * a class declaration with optional inheritance.
 * 
 * @param name Pointer to AST node containing the class name.
 * @param super Pointer to AST node containing the superclass, or NULL if no inheritance.
 * @param body Pointer to AST node containing the class body.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_CLASS node, or NULL on allocation failure.
 */
Ast* AstClass(Ast* name, Ast* super, Ast* body, Position position);

/**
 * Creates an AST node representing a function definition.
 * 
 * Allocates and initializes an AST_FUNCTION node that represents
 * a function declaration with parameters and body.
 * 
 * @param fnName Pointer to AST node containing the function name.
 * @param parameters Pointer to AST node containing the function parameters.
 * @param body Pointer to AST node containing the function body.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_FUNCTION node, or NULL on allocation failure.
 */
Ast* AstFunction(Ast* fnName, Ast* parameters, Ast* body, Position position);

/**
 * Creates an AST node representing an import statement.
 * 
 * Allocates and initializes an AST_IMPORT node that represents
 * a module import declaration.
 * 
 * @param imports Pointer to AST node containing the list of imports.
 * @param moduleName Pointer to AST node containing the module name.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_IMPORT node, or NULL on allocation failure.
 */
Ast* AstImport(Ast* imports, Ast* moduleName, Position position);

/**
 * Creates an AST node representing a variable declaration.
 * 
 * Allocates and initializes an AST_VAR_DECLARATION node that represents
 * one or more variable declarations (var, local, const).
 * 
 * @param type The type of the variable declaration (e.g., var, local, const).
 * @param declarations Pointer to AST node containing the variable declarations.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_VAR_DECLARATION node, or NULL on allocation failure.
 */
Ast* AstVarDeclaration(AstType type, Ast* declarations, Position position);

/**
 * Creates an AST node representing an if statement.
 * 
 * Allocates and initializes an AST_IF node that represents
 * a conditional statement with optional else branch.
 * 
 * @param condition Pointer to the condition AST node.
 * @param thenBranch Pointer to the then branch AST node.
 * @param elseBranch Pointer to the else branch AST node, or NULL if no else.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_IF node, or NULL on allocation failure.
 */
Ast* AstIf(Ast* condition, Ast* thenBranch, Ast* elseBranch, Position position);

/**
 * Creates an AST node representing a for statement.
 * 
 * Allocates and initializes an AST_FOR node that represents
 * a for loop with initializer, condition, and mutator.
 * 
 * @param initializerConditionMutator Pointer to the initializer, condition, and mutator AST node.
 * @param body Pointer to the body AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_FOR node, or NULL on allocation failure.
 */
Ast* AstFor(Ast* initializerConditionMutator, Ast* body, Position position);

/**
 * Creates an AST node representing a while statement.
 * 
 * Allocates and initializes an AST_WHILE node that represents
 * a while loop with condition and body.
 * 
 * @param condition Pointer to the condition AST node.
 * @param body Pointer to the body AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_WHILE node, or NULL on allocation failure.
 */
Ast* AstWhile(Ast* condition, Ast* body, Position position);

/**
 * Creates an AST node representing a do-while statement.
 * 
 * Allocates and initializes an AST_DO_WHILE node that represents
 * a do-while loop with body and condition.
 * 
 * @param condition Pointer to the condition AST node.
 * @param body Pointer to the body AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_DO_WHILE node, or NULL on allocation failure.
 */
Ast* AstDoWhile(Ast* condition, Ast* body, Position position);

/**
 * Creates an AST node representing a try-catch statement.
 * 
 * Allocates and initializes an AST_TRY_CATCH node that represents
 * exception handling with try and catch blocks.
 * 
 * @param tryBlock Pointer to the try block AST node.
 * @param errorName Pointer to the error name AST node for the catch clause.
 * @param catchBlock Pointer to the catch block AST node.
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_TRY_CATCH node, or NULL on allocation failure.
 */
Ast* AstTryCatch(Ast* tryBlock, Ast* errorName,  Ast* catchBlock, Position position);

/**
 * Creates an AST node representing a block statement.
 * 
 * Allocates and initializes an AST_BLOCK node that represents
 * a block of statements enclosed in braces.
 * 
 * @param statements Pointer to child AST nodes (statements).
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_BLOCK node, or NULL on allocation failure.
 */
Ast* AstBlock(Ast* statements, Position position);

/**
 * Creates an AST node representing the root program node.
 * 
 * Allocates and initializes an AST_PROGRAM node that represents
 * the top-level program containing all statements and declarations.
 * 
 * @param children Pointer to child AST nodes (statements/declarations).
 * @param position Source code location information.
 * @return Pointer to newly allocated AST_PROGRAM node, or NULL on allocation failure.
 */
Ast* AstProgram(Ast* children, Position position);

/**
 * Frees an AST node and all its children.
 * 
 * Recursively deallocates memory for an AST node and all of its
 * child nodes, preventing memory leaks.
 * 
 * @param ast Pointer to the AST node to free.
 */
void FreeAst(Ast* ast);

#endif /* ASTNODE_H */