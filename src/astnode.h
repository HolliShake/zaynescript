/**
 * @file astnode.h
 * @brief Factories allocating AST_ prefixed node types, retaining source syntax offsets for diagnostics.
 *
 * Each factory function creates one concrete `AstType` shape while preserving
 * source positions so later compiler and runtime diagnostics can point back to
 * the original program text.
 */

#include "./global.h"

#ifndef ASTNODE_H
#	define ASTNODE_H

/**
 * @brief Creates an AST node representing an identifier/name.
 *
 * Constructs \1 referencing \2
 *
 * @param name String value of the identifier.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstName(String name, Position position);

/**
 * @brief Creates an AST node representing an integer literal.
 *
 * Constructs \1 referencing \2
 *
 * @param value String representation of the integer value.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstInteger(String value, Position position);

/**
 * @brief Creates an AST node representing a big integer literal.
 *
 * Constructs \1 referencing \2
 *
 * @param value String representation of the big integer value.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstBigInteger(String value, Position position);

/**
 * @brief Creates an AST node representing a floating-point
 * number literal.
 *
 * Constructs \1 referencing \2
 *
 * @param value String representation of the number value.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstNumber(String value, Position position);

/**
 * @brief Creates an AST node representing a big number literal.
 *
 * Constructs \1 referencing \2
 *
 * @param value String representation of the big number value.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_BIGNUMBER node, or NULL
 * on allocation failure.
 */
Ast* AstBigNumber(String value, Position position);

/**
 * @brief Creates an AST node representing a string literal.
 *
 * Constructs \1 referencing \2
 *
 * @param value String content of the literal.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstString(String value, Position position);

/**
 * @brief Creates an AST node representing a boolean literal.
 *
 * Constructs \1 referencing \2
 *
 * @param value Boolean value (true or false).
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstBool(bool value, Position position);

/**
 * @brief Creates an AST node representing a null literal.
 *
 * Constructs \1 referencing \2
 *
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstNull(Position position);

/**
 * @brief Creates an AST node representing the 'this' keyword.
 *
 * Allocates and initializes an AST_THIS node that represents the
 * 'this' keyword in the source code, typically used within class
 * methods to refer to the current instance.
 *
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstThis(Position position);

/**
 * @brief Creates an AST node representing the 'base' keyword.
 *
 * Allocates and initializes an AST_BASE node that represents the
 * 'base' keyword in the source code, typically used within class
 * methods to refer to the base class instance.
 *
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstBase(Position position);

/**
 * @brief Creates an AST node representing a spread operator.
 *
 * Constructs \1 referencing \2
 *
 * @param expression Pointer to the expression AST node to
 * spread.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_SPREAD_OPERATOR node,
 * or NULL on allocation failure.
 */
Ast* AstSpread(Ast* expression, Position position);

/**
 * @brief Creates an AST node representing a list literal.
 *
 * Constructs \1 referencing \2
 *
 * @param elements Pointer to AST node containing the list
 * elements.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_LIST_LITERAL node, or
 * NULL on allocation failure.
 */
Ast* AstListLiteral(Ast* elements, Position position);

/**
 * @brief Creates an AST node representing a key-value pair in an
 * object.
 *
 * Constructs \1 referencing \2
 *
 * @param key Pointer to the key AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_OBJECT_KEY_VAL node, or
 * NULL on allocation failure.
 */
Ast* AstObjectKeyVal(Ast* key, Position position);

/**
 * @brief Creates an AST node representing an object literal.
 *
 * Constructs \1 referencing \2
 *
 * @param properties Pointer to AST node containing the object
 * properties.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_OBJECT_LITERAL node, or
 * NULL on allocation failure.
 */
Ast* AstObjectLiteral(Ast* properties, Position position);

/**
 * @brief Creates an AST node representing an allocation
 * expression.
 *
 * Constructs \1 referencing \2
 *
 * @param cls Pointer to the class AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_ALLOCATION node, or
 * NULL on allocation failure.
 */
Ast* AstAllocation(Ast* cls, Position position);

/**
 * @brief Creates an AST node representing a member access.
 *
 * Constructs \1 referencing \2
 * (object.member).
 *
 * @param object Pointer to the object AST node.
 * @param member Pointer to the member AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstMember(Ast* object, Ast* member, Position position);

/**
 * @brief Creates an AST node representing an index access.
 *
 * Constructs \1 referencing \2
 * (object[index]).
 *
 * @param object Pointer to the object AST node.
 * @param index Pointer to the index AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstIndex(Ast* object, Ast* index, Position position);

/**
 * @brief Creates an AST node representing a function call.
 *
 * Constructs \1 referencing \2
 *
 * @param object Pointer to the callable object AST node.
 * @param arguments Pointer to the arguments AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstCall(Ast* object, Ast* arguments, Position position);

/**
 * @brief Creates an AST node representing a unary or postfix
 * operation.
 *
 * Constructs \1 referencing \2
 * decrement).
 *
 * @param type The type of unary operation (e.g., negation,
 * logical NOT).
 * @param operand Pointer to the operand AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstSingle(AstType type, Ast* operand, Position position);

/**
 * @brief Creates an AST node representing a binary operation.
 *
 * Constructs \1 referencing \2
 * comparison).
 *
 * @param type The type of binary operation (e.g., addition,
 * multiplication).
 * @param lhs Pointer to the left-hand side operand AST node.
 * @param rhs Pointer to the right-hand side operand AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstBinary(AstType type, Ast* lhs, Ast* rhs, Position position);

/**
 * @brief Creates an AST node representing a ternary conditional
 * operation.
 *
 * Constructs \1 referencing \2
 * elseBranch).
 *
 * @param condition Pointer to the condition AST node.
 * @param thenBranch Pointer to the then-branch AST node.
 * @param elseBranch Pointer to the else-branch AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_TERNARY node, or NULL
 * on allocation failure.
 */
Ast* AstTernary(Ast*	 condition,
				Ast*	 thenBranch,
				Ast*	 elseBranch,
				Position position);

/**
 * @brief Creates an AST node representing a raise statement.
 *
 * Constructs \1 referencing \2
 *
 * @param expression Pointer to the expression AST node whose
 * value is raised.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstRaise(Ast* expression, Position position);

/**
 * @brief Creates an AST node representing an assert statement.
 *
 * Constructs \1 referencing \2
 * value, the fallback expression is raised as an error.
 *
 * @param condition Pointer to the condition AST node to
 * evaluate.
 * @param fallback Pointer to the expression AST node raised when
 * the condition is falsy, or NULL for a default assertion error.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstAssert(Ast* condition, Ast* fallback, Position position);

/**
 * @brief Creates an AST node representing a continue statement.
 *
 * Constructs \1 referencing \2
 *
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_CONTINUE node, or NULL
 * on allocation failure.
 */
Ast* AstContinue(Position position);

/**
 * @brief Creates an AST node representing a break statement.
 *
 * Constructs \1 referencing \2
 *
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstBreak(Position position);

/**
 * @brief Creates an AST node representing a return statement.
 *
 * Constructs \1 referencing \2
 *
 * @param expression Pointer to the expression AST node, or NULL
 * for empty return.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstReturn(Ast* expression, Position position);

/**
 * @brief Creates an AST node representing an expression
 * statement.
 *
 * Allocates and initializes an AST_EXPRESSION_STATEMENT node
 * that represents an expression used as a statement.
 *
 * @param expression Pointer to the expression AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_EXPRESSION_STATEMENT
 * node, or NULL on allocation failure.
 */
Ast* AstExpressionStatement(Ast* expression, Position position);

/**
 * @brief Creates an AST node representing a class member.
 *
 * Constructs \1 referencing \2
 * class, which may be static.
 *
 * @param _static_ Boolean indicating if the member is static.
 * @param node Pointer to the member AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_CLASS_MEMBER node, or
 * NULL on allocation failure.
 */
Ast* AstClassMember(bool _static_, Ast* node, Position position);

/**
 * @brief Creates an AST node representing a class definition.
 *
 * Constructs \1 referencing \2
 *
 * @param name Pointer to AST node containing the class name.
 * @param super Pointer to AST node containing the superclass, or
 * NULL if no inheritance.
 * @param body Pointer to AST node containing the class body.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstClass(Ast* name, Ast* super, Ast* body, Position position);

/**
 * @brief Creates an AST node representing a function definition.
 *
 * Constructs \1 referencing \2
 *
 * @param fnName Pointer to AST node containing the function
 * name.
 * @param parameters Pointer to AST node containing the function
 * parameters.
 * @param body Pointer to AST node containing the function body.
 * @param async Boolean indicating if the function is
 * asynchronous.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_FUNCTION node, or NULL
 * on allocation failure.
 */
Ast* AstFunction(Ast*	  fnName,
				 Ast*	  parameters,
				 Ast*	  body,
				 bool	  async,
				 Position position);

/**
 * @brief Creates an AST node representing an immediate function.
 *
 * Constructs \1 referencing \2
 *
 * @param fnName Pointer to AST node containing the function name.
 * @param parameters Pointer to AST node containing the function parameters.
 * @param body Pointer to AST node containing the function body.
 * @param async Boolean indicating if the function is asynchronous.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstImmediateFunction(Ast*	   fnName,
						  Ast*	   parameters,
						  Ast*	   body,
						  bool	   async,
						  Position position);

/**
 * @brief Creates an AST node representing an import statement.
 *
 * Constructs \1 referencing \2
 *
 * @param imports Pointer to AST node containing the list of
 * imports.
 * @param moduleName Pointer to AST node containing the module
 * name.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstImport(Ast* imports, Ast* moduleName, Position position);

/**
 * @brief Creates an AST node representing a variable
 * declaration.
 *
 * Constructs \1 referencing \2
 * const).
 *
 * @param type The type of the variable declaration (e.g., var,
 * local, const).
 * @param declarations Pointer to AST node containing the
 * variable declarations.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_VAR_DECLARATION node,
 * or NULL on allocation failure.
 */
Ast* AstVarDeclaration(AstType type, Ast* declarations, Position position);

/**
 * @brief Creates an AST node representing an empty statement
 * (noop).
 *
 * Constructs \1 referencing \2
 *
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_EMPTY_STMNT node, or
 * NULL on allocation failure.
 */
Ast* AstEmptyStatement(Position position);

/**
 * @brief Creates an AST node representing an if statement.
 *
 * Constructs \1 referencing \2
 *
 * @param condition Pointer to the condition AST node.
 * @param thenBranch Pointer to the then branch AST node.
 * @param elseBranch Pointer to the else branch AST node, or NULL
 * if no else.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstIf(Ast* condition, Ast* thenBranch, Ast* elseBranch, Position position);

/**
 * @brief Creates an AST node representing a switch statement.
 *
 * Constructs \1 referencing \2
 *
 * @param expression Pointer to the switch expression AST node.
 * @param cases Pointer to AST node containing the list of cases.
 * @param defaultCase Pointer to the default case AST node, or
 * NULL if no default.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstSwitch(Ast*		expression,
			   Ast*		cases,
			   Ast*		defaultCase,
			   Position position);

/**
 * @brief Creates an AST node representing a switch case.
 *
 * Constructs \1 referencing \2
 * the case value and body.
 *
 * @param value Pointer to the case value AST node.
 * @param body Pointer to the case body AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_SWITCH_CASE node, or
 * NULL on allocation failure.
 */
Ast* AstSwitchCase(Ast* value, Ast* body, Position position);

/**
 * @brief Creates an AST node representing a for statement.
 *
 * Constructs \1 referencing \2
 *
 * @param initializerConditionMutator Pointer to the initializer,
 * condition, and mutator AST node.
 * @param body Pointer to the body AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstFor(Ast* initializerConditionMutator, Ast* body, Position position);

/**
 * @brief Creates an AST node representing a while statement.
 *
 * Constructs \1 referencing \2
 *
 * @param condition Pointer to the condition AST node.
 * @param body Pointer to the body AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstWhile(Ast* condition, Ast* body, Position position);

/**
 * @brief Creates an AST node representing a do-while statement.
 *
 * Constructs \1 referencing \2
 *
 * @param condition Pointer to the condition AST node.
 * @param body Pointer to the body AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_DO_WHILE node, or NULL
 * on allocation failure.
 */
Ast* AstDoWhile(Ast* condition, Ast* body, Position position);

/**
 * @brief Creates an AST node representing a try-catch statement.
 *
 * Constructs \1 referencing \2
 *
 * @param tryBlock Pointer to the try block AST node.
 * @param errorName Pointer to the error name AST node for the
 * catch clause.
 * @param catchBlock Pointer to the catch block AST node.
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_TRY_CATCH node, or NULL
 * on allocation failure.
 */
Ast* AstTryCatch(Ast*	  tryBlock,
				 Ast*	  errorName,
				 Ast*	  catchBlock,
				 Position position);

/**
 * @brief Creates an AST node representing a block statement.
 *
 * Constructs \1 referencing \2
 *
 * @param statements Pointer to child AST nodes (statements).
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Fresh \1 container allocated from global memory, halting securely on failure.
 */
Ast* AstBlock(Ast* statements, Position position);

/**
 * @brief Creates an AST node representing the root program node.
 *
 * Constructs \1 referencing \2
 * declarations.
 *
 * @param children Pointer to child AST nodes
 * (statements/declarations).
 * @param position Lexical start coordinates strictly attached for debugging.
 * @return Pointer to newly allocated AST_PROGRAM node, or NULL
 * on allocation failure.
 */
Ast* AstProgram(Ast* children, Position position);

/**
 * @brief Recursively frees an AST subtree and every owned string or child node
 *        reachable from it.
 *
 * This is the parser/compiler teardown path used after a module has either been
 * compiled or discarded.
 *
 * @param ast Root AST node to release. Passing `NULL` is safe.
 */
void FreeAst(Ast* ast);

#endif /* ASTNODE_H */
