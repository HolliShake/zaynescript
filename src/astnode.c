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
    Ast* ast = InitAst(AST_INTEGER, position);
    ast->Value = value;
    return ast;
}

Ast* AstNumber(String value, Position position) {
    Ast* ast = InitAst(AST_NUMBER, position);
    ast->Value = value;
    return ast;
}

Ast* AstString(String value, Position position) {
    Ast* ast = InitAst(AST_STRING, position);
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

Ast* AstFunction(Ast* fnName, Ast* parameters, Ast* body, Position position) {
    Ast* ast = InitAst(AST_FUNCTION, position);
    ast->A   = fnName;
    ast->B   = parameters;
    ast->C   = body;
    return ast;
}

Ast* AstProgram(Ast* children, Position position) {
    Ast* ast = InitAst(AST_PROGRAM, position);
    ast->A   = children;
    return ast;
}