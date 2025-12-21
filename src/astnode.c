#include "./astnode.h"


static Ast* InitAst(AstType type, Position position) {
    Ast* ast       = Allocate(sizeof(Ast));
    ast->Type      = type;
    ast->Position  = position;
    ast->Value     = NULL;
    ast->A         = NULL;
    ast->B         = NULL;
    ast->C         = NULL;
    ast->D         = NULL;
    ast->Next      = NULL;
    return ast;
}

Ast* AstName(String name, Position position) {
    Ast* ast = InitAst(AST_NAME, position);
    ast->Value = name;
    return ast;
}


Ast* AstInteger(String value, Position position) {
    Ast* ast = InitAst(AST_INT, position);
    ast->Value = value;
    return ast;
}

Ast* AstNumber(String value, Position position) {
    Ast* ast = InitAst(AST_NUM, position);
    ast->Value = value;
    return ast;
}

Ast* AstString(String value, Position position) {
    Ast* ast = InitAst(AST_STR, position);
    ast->Value = value;
    return ast;
}

Ast* AstBool(bool value, Position position) {
    Ast* ast = InitAst(AST_BOOL, position);
    ast->Value = value ? "true" : "false";
    return ast;
}

Ast* AstNull(Position position) {
    Ast* ast = InitAst(AST_NULL, position);
    return ast;
}

Ast* AstBinary(AstType type, Ast* lhs, Ast* rhs, Position position) {
    Ast* ast = InitAst(type, position);
    ast->A = lhs;
    ast->B = rhs;
    return ast;
}

Ast* AstExpressionStatement(Ast* expression, Position position) {
    Ast* ast = InitAst(AST_EXPRESSION_STATEMENT, position);
    ast->A = expression;
    return ast;
}

Ast* AstFunction(Ast* fnName, Ast* parameters, Ast* body, Position position) {
    Ast* ast = InitAst(AST_FUNCTION, position);
    ast->A   = fnName;
    ast->B   = parameters;
    ast->C   = body;
    return ast;
}

Ast* AstProgram(Ast* body, Position position) {
    Ast* ast = InitAst(AST_PROGRAM, position);
    ast->A   = body;
    return ast;
}