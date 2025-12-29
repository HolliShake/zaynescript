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
 * AstMember - Creates an AST node representing a member access
 * @object: Pointer to the object AST node
 * @member: Pointer to the member AST node
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_MEMBER node
 */
Ast* AstMember(Ast* object, Ast* member, Position position);

/**
 * AstIndex - Creates an AST node representing an index access
 * @object: Pointer to the object AST node
 * @index: Pointer to the index AST node
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_INDEX node
 */
Ast* AstIndex(Ast* object, Ast* index, Position position);

/**
 * AstCall - Creates an AST node representing a function call
 * @object: Pointer to the object AST node
 * @arguments: Pointer to the arguments AST node
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_CALL node
 */
Ast* AstCall(Ast* object, Ast* arguments, Position position);

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
 * AstReturn - Creates an AST node representing a return statement
 * @expression: Pointer to the expression AST node
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_RETURN node
 */
Ast* AstReturn(Ast* expression, Position position);

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
 * AstImport - Creates an AST node representing an import statement
 * @imports: Pointer to AST node containing the list of imports
 * @moduleName: Pointer to AST node containing the module name
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_IMPORT node
 */
Ast* AstImport(Ast* imports, Ast* moduleName, Position position);

/**
 * AstVarDeclaration - Creates an AST node representing a variable declaration
 * @type: The type of the variable declaration
 * @declarations: Pointer to AST node containing the variable declarations
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_VAR_DECLARATION node
 */
Ast* AstVarDeclaration(AstType type, Ast* declarations, Position position);

/**
 * AstIf - Creates an AST node representing an if statement
 * @condition: Pointer to the condition AST node
 * @thenBranch: Pointer to the then branch AST node
 * @elseBranch: Pointer to the else branch AST node
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_IF node
 */
Ast* AstIf(Ast* condition, Ast* thenBranch, Ast* elseBranch, Position position);

/**
 * AstFor - Creates an AST node representing a for statement
 * @initializerConditionMutator: Pointer to the initializer, condition, and mutator AST node
 * @body: Pointer to the body AST node
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_FOR node
 */
Ast* AstFor(Ast* initializerConditionMutator, Ast* body, Position position);

/**
 * AstWhile - Creates an AST node representing a while statement
 * @condition: Pointer to the condition AST node
 * @body: Pointer to the body AST node
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_WHILE node
 */
Ast* AstWhile(Ast* condition, Ast* body, Position position);

/**
 * AstDoWhile - Creates an AST node representing a do while statement
 * @condition: Pointer to the condition AST node
 * @body: Pointer to the body AST node
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_DO_WHILE node
 */
Ast* AstDoWhile(Ast* condition, Ast* body, Position position);

/**
 * AstBlock - Creates an AST node representing a block statement
 * @statements: Pointer to child AST nodes (statements)
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_BLOCK node
 */
Ast* AstBlock(Ast* statements, Position position);

/**
 * AstProgram - Creates an AST node representing the root program node
 * @children: Pointer to child AST nodes (statements/declarations)
 * @position: Source code location information
 *
 * Return: Pointer to newly allocated AST_PROGRAM node
 */
Ast* AstProgram(Ast* children, Position position);

/**
 * FreeAst - Frees an AST node and all its children
 * @ast: Pointer to the AST node to free
 */
void FreeAst(Ast* ast);

#endif