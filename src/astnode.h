#include "./global.h"

#ifndef ASTNODE_H
#define ASTNODE_H

/**
 * AstName - Creates an AST node representing an identifier/name
 * @name: String value of the identifier
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_NAME node
 */
Ast* AstName(String name, Position position);

/**
 * AstInteger - Creates an AST node representing an integer literal
 * @value: String representation of the integer value
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_INT node
 */
Ast* AstInteger(String value, Position position);

/**
 * AstNumber - Creates an AST node representing a floating-point number literal
 * @value: String representation of the number value
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_NUM node
 */
Ast* AstNumber(String value, Position position);

/**
 * AstString - Creates an AST node representing a string literal
 * @value: String content of the literal
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_STR node
 */
Ast* AstString(String value, Position position);

/**
 * AstBool - Creates an AST node representing a boolean literal
 * @value: Boolean value (true or false)
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_BOOL node
 */
Ast* AstBool(bool value, Position position);

/**
 * AstNull - Creates an AST node representing a null literal
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_NULL node
 */
Ast* AstNull(Position position);

/**
 * AstBinary - Creates an AST node representing a binary operation
 * @type: The type of binary operation (e.g., addition, multiplication)
 * @lhs: Pointer to the left-hand side operand AST node
 * @rhs: Pointer to the right-hand side operand AST node
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_BINARY node
 */
Ast* AstBinary(AstType type, Ast* lhs, Ast* rhs, Position position);

/**
 * AstExpressionStatement - Creates an AST node representing an expression statement
 * @expression: Pointer to the expression AST node
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_EXPRESSION_STATEMENT node
 */
Ast* AstExpressionStatement(Ast* expression, Position position);

/**
 * AstFunction - Creates an AST node representing a function definition
 * @fnName: Pointer to AST node containing the function name
 * @parameters: Pointer to AST node containing the function parameters
 * @body: Pointer to AST node containing the function body
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_FUNCTION node
 */
Ast* AstFunction(Ast* fnName, Ast* parameters, Ast* body, Position position);

/**
 * AstProgram - Creates an AST node representing the root program node
 * @children: Pointer to child AST nodes (statements/declarations)
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_PROGRAM node
 */
Ast* AstProgram(Ast* children, Position position);

#endif