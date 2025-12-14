#include "./global.h"


// AstName: Creates an AST node representing an identifier/name
// Parameters:
//   name     - String value of the identifier
//   position - Source code location information
// Returns: Pointer to newly allocated AST_NAME node
Ast* AstName(String name, Position position);

// AstInteger: Creates an AST node representing an integer literal
// Parameters:
//   value    - String representation of the integer value
//   position - Source code location information
// Returns: Pointer to newly allocated AST_INTEGER node
Ast* AstInteger(String value, Position position);

// AstNumber: Creates an AST node representing a floating-point number literal
// Parameters:
//   value    - String representation of the number value
//   position - Source code location information
// Returns: Pointer to newly allocated AST_NUMBER node
Ast* AstNumber(String value, Position position);

// AstString: Creates an AST node representing a string literal
// Parameters:
//   value    - String content of the literal
//   position - Source code location information
// Returns: Pointer to newly allocated AST_STRING node
Ast* AstString(String value, Position position);

// AstBool: Creates an AST node representing a boolean literal
// Parameters:
//   value    - Boolean value (true or false)
//   position - Source code location information
// Returns: Pointer to newly allocated AST_BOOL node
Ast* AstBool(bool value, Position position);

// AstNull: Creates an AST node representing a null literal
// Parameters:
//   position - Source code location information
// Returns: Pointer to newly allocated AST_NULL node
Ast* AstNull(Position position);

// AstBinary: Creates an AST node representing a binary operation
// Parameters:
//   type     - The type of binary operation (e.g., addition, multiplication)
//   lhs      - Pointer to the left-hand side operand AST node
//   rhs      - Pointer to the right-hand side operand AST node
//   position - Source code location information
// Returns: Pointer to newly allocated AST_BINARY node
Ast* AstBinary(AstType type, Ast* lhs, Ast* rhs, Position position);

// AstProgram: Creates an AST node representing the root program node
// Parameters:
//   children - Pointer to child AST nodes (statements/declarations)
//   position - Source code location information
// Returns: Pointer to newly allocated AST_PROGRAM node
Ast* AstProgram(Ast* children, Position position);