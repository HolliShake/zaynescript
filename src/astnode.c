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
    Ast* ast   = InitAst(AST_NAME, position);
    ast->Value = name;
    return ast;
}


Ast* AstInteger(String value, Position position) {
    Ast* ast   = InitAst(AST_INT, position);
    ast->Value = value;
    return ast;
}

Ast* AstNumber(String value, Position position) {
    Ast* ast   = InitAst(AST_NUM, position);
    ast->Value = value;
    return ast;
}

Ast* AstString(String value, Position position) {
    Ast* ast   = InitAst(AST_STR, position);
    ast->Value = value;
    return ast;
}

Ast* AstBool(bool value, Position position) {
    Ast* ast   = InitAst(AST_BOOL, position);
    ast->Value = AllocateString(value ? "true" : "false");
    return ast;
}

Ast* AstNull(Position position) {
    Ast* ast = InitAst(AST_NULL, position);
    return ast;
}

Ast* AstThis(Position position) {
    Ast* ast = InitAst(AST_THIS, position);
    return ast;
}

Ast* AstSpread(Ast* expression, Position position) {
    Ast* ast = InitAst(AST_SPREAD, position);
    ast->A   = expression;
    return ast;
}

Ast* AstListLiteral(Ast* elements, Position position) {
    Ast* ast = InitAst(AST_LIST_LITERAL, position);
    ast->A   = elements;
    return ast;
}

Ast* AstObjectKeyVal(Ast* key, Position position) {
    Ast* ast = InitAst(AST_OBJECT_KEY_VAL, position);
    ast->A   = key;
    return ast;
}

Ast* AstObjectLiteral(Ast* properties, Position position) {
    Ast* ast = InitAst(AST_OBJECT_LITERAL, position);
    ast->A   = properties;
    return ast;
}

Ast* AstAllocation(Ast* cls, Ast* arguments, Position position) {
    Ast* ast = InitAst(AST_ALLOCATION, position);
    ast->A   = cls;
    ast->B   = arguments;
    return ast;
}

Ast* AstMember(Ast* object, Ast* member, Position position) {
    Ast* ast = InitAst(AST_MEMBER, position);
    ast->A   = object;
    ast->B   = member;
    return ast;
}

Ast* AstIndex(Ast* object, Ast* index, Position position) {
    Ast* ast = InitAst(AST_INDEX, position);
    ast->A   = object;
    ast->B   = index;
    return ast;
}

Ast* AstCall(Ast* object, Ast* arguments, Position position) {
    Ast* ast = InitAst(AST_CALL, position);
    ast->A   = object;
    ast->B   = arguments;
    return ast;
}

Ast* AstSingle(AstType type, Ast* operand, Position position) {
    Ast* ast = InitAst(type, position);
    ast->A   = operand;
    return ast;
}

Ast* AstBinary(AstType type, Ast* lhs, Ast* rhs, Position position) {
    Ast* ast = InitAst(type, position);
    ast->A   = lhs;
    ast->B   = rhs;
    return ast;
}

Ast* AstTernary(Ast* condition, Ast* trueBranch, Ast* falseBranch, Position position) {
    Ast* ast = InitAst(AST_TERNARY, position);
    ast->A   = condition;
    ast->B   = trueBranch;
    ast->C   = falseBranch;
    return ast;
}

Ast* AstContinue(Position position) {
    Ast* ast = InitAst(AST_CONTINUE, position);
    return ast;
}

Ast* AstBreak(Position position) {
    Ast* ast = InitAst(AST_BREAK, position);
    return ast;
}

Ast* AstReturn(Ast* expression, Position position) {
    Ast* ast = InitAst(AST_RETURN, position);
    ast->A = expression;
    return ast;
}

Ast* AstExpressionStatement(Ast* expression, Position position) {
    Ast* ast = InitAst(AST_EXPRESSION_STATEMENT, position);
    ast->A   = expression;
    return ast;
}

Ast* AstClassMember(bool _static_, Ast* node, Position position) {
    Ast* ast   = InitAst(AST_CLASS_MEMBER, position);
    ast->A     = node;
    ast->Value = (_static_) 
        ? AllocateString("static") 
        : AllocateString("instance");
    return ast;
}

Ast* AstClass(Ast* name, Ast* super, Ast* body, Position position) {
    Ast* ast = InitAst(AST_CLASS, position);
    ast->A   = name;
    ast->B   = super;
    ast->C   = body;
    return ast;
} 

Ast* AstFunction(Ast* fnName, Ast* parameters, Ast* body, Position position) {
    Ast* ast = InitAst(AST_FUNCTION, position);
    ast->A   = fnName;
    ast->B   = parameters;
    ast->C   = body;
    return ast;
}

Ast* AstImport(Ast* imports, Ast* moduleName, Position position) {
    Ast* ast = InitAst(AST_IMPORT, position);
    ast->A   = imports;
    ast->B   = moduleName;
    return ast;
}

Ast* AstVarDeclaration(AstType type, Ast* declarations, Position position) {
    Ast* ast = InitAst(type, position);
    ast->A   = declarations;
    return ast;
}

Ast* AstIf(Ast* condition, Ast* thenBranch, Ast* elseBranch, Position position) {
    Ast* ast = InitAst(AST_IF, position);
    ast->A   = condition;
    ast->B   = thenBranch;
    ast->C   = elseBranch;
    return ast;
}

Ast* AstFor(Ast* initializerConditionMutator, Ast* body, Position position) {
    Ast* ast = InitAst(AST_FOR, position);
    ast->A   = initializerConditionMutator;
    ast->B   = body;
    return ast;
}

Ast* AstWhile(Ast* condition, Ast* body, Position position) {
    Ast* ast = InitAst(AST_WHILE, position);
    ast->A   = condition;
    ast->B   = body;
    return ast;
}

Ast* AstDoWhile(Ast* condition, Ast* body, Position position) {
    Ast* ast = InitAst(AST_DO_WHILE, position);
    ast->A   = condition;
    ast->B   = body;
    return ast;
}

Ast* AstTryCatch(Ast* tryBlock, Ast* errorName,  Ast* catchBlock, Position position) {
    Ast* ast = InitAst(AST_TRY_CATCH, position);
    ast->A   = tryBlock;
    ast->B   = errorName;
    ast->C   = catchBlock;
    return ast;
}

Ast* AstBlock(Ast* statements, Position position) {
    Ast* ast = InitAst(AST_BLOCK, position);
    ast->A = statements;
    return ast;
}

Ast* AstProgram(Ast* body, Position position) {
    Ast* ast = InitAst(AST_PROGRAM, position);
    ast->A   = body;
    return ast;
}

void FreeAst(Ast* ast) {
    if (ast == NULL) {
        return;
    }
    if (ast->A != NULL) {
        FreeAst(ast->A);
    }
    if (ast->B != NULL) {
        FreeAst(ast->B);
    }
    if (ast->C != NULL) {
        FreeAst(ast->C);
    }
    if (ast->D != NULL) {
        FreeAst(ast->D);
    }
    if (ast->Next != NULL) {
        FreeAst(ast->Next);
    }
    if (ast->Value != NULL) {
        free(ast->Value);
    }
    free(ast);
}