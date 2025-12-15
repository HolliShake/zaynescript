#include "./parser.h"

Parser* CreateParser(Lexer* lexer) {
    Parser* parser = Allocate(sizeof(Parser));
    parser->Lexer = lexer;
    return parser;
}

#define CHECKTV(v) _CheckTokenV(parser, v)

#define CHECKTT(t) _CheckTokenT(parser, t)

#define ACCEPTV(v) _AcceptTokenV(parser, v)

#define ACCEPTV_FREE(v) { \
    String value = parser->Next.Value; \
    _AcceptTokenV(parser, v); \
    free(value); \
} \

#define ACCEPTT(t) _AcceptTokenT(parser, t)

static int _CheckTokenV(Parser* parser, String value) {
    return strcmp(parser->Next.Value, value) == 0 && (
        parser->Next.Type == TK_IDN ||
        parser->Next.Type == TK_KEY ||
        parser->Next.Type == TK_SYM
    );
}

static int _CheckTokenT(Parser* parser, TokenType type) {
    return parser->Next.Type == type;
}

static void _AcceptTokenV(Parser* parser, String value) {
    if (_CheckTokenV(parser, value)) {
        parser->Next = NextToken(parser->Lexer);
        return;
    }
    char message[256];
    snprintf(message, sizeof(message), "expected token value %s, got %s", value, parser->Next.Value);
    ThrowError(
        parser->Lexer->Path, 
        parser->Lexer->Data, 
        parser->Next.Position, 
        message
    );
}

static void _AcceptTokenT(Parser* parser, TokenType type) {
    if (_CheckTokenT(parser, type)) {
        parser->Next = NextToken(parser->Lexer);
        return;
    }
    char message[256];
    snprintf(message, sizeof(message), "expected token type %d, got %d", type, parser->Next.Type);
    ThrowError(
        parser->Lexer->Path, 
        parser->Lexer->Data, 
        parser->Next.Position, 
        message
    );
}

static Ast* _ListOfExpressions(Parser* parser);

static Ast* _Terminal(Parser* parser) {
    Ast* node = NULL;
    switch (parser->Next.Type) {
        case TK_IDN: {
            node = AstName(
                parser->Next.Value, 
                parser->Next.Position
            );
            ACCEPTT(TK_IDN);
            break;
        }
        case TK_INT: {
            node = AstInteger(
                parser->Next.Value, 
                parser->Next.Position
            );
            ACCEPTT(TK_INT);
            break;
        }
        case TK_NUM: {
            node = AstNumber(
                parser->Next.Value, 
                parser->Next.Position
            );
            ACCEPTT(TK_NUM);
            break;
        }
        case TK_STR: {
            node = AstString(
                parser->Next.Value, 
                parser->Next.Position
            );
            ACCEPTT(TK_STR);
            break;
        }
        default:
            return NULL;
    }
    return node;
}

static Ast* _Multiplicative(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Terminal(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("*") || CHECKTV("/") || CHECKTV("%")) {
        // * | / | %
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _Terminal(parser);

        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            op[0] == '*' ? AST_MUL : op[0] == '/' ? AST_DIV : AST_MOD,
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _Addetive(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Multiplicative(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("+") || CHECKTV("-")) {
        // + | -
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _Multiplicative(parser);

        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            op[0] == '+' ? AST_ADD : AST_SUB,
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _Shift(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Addetive(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("<<") || CHECKTV(">>")) {
        // << | >>
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _Addetive(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            strcmp(op, "<<") == 0 ? AST_LSHFT : AST_RSHFT,
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _Relational(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Shift(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("<") || CHECKTV("<=") || CHECKTV(">") || CHECKTV(">=")) {
        // < | <= | > | >=
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _Shift(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            strcmp(op, "<") == 0 
                ? AST_LT 
                : strcmp(op, "<=") == 0
                    ? AST_LTE
                    : strcmp(op, ">") == 0
                        ? AST_GT
                        : AST_GTE,
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _Equality(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Relational(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("==") || CHECKTV("!=")) {
        // == | !=
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _Relational(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            strcmp(op, "==") == 0 
                ? AST_EQ 
                : AST_NE,
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _Bitwise(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Equality(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("&") || CHECKTV("|") || CHECKTV("^")) {
        // & | | | ^
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _Equality(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            strcmp(op, "&") == 0 
                ? AST_AND 
                : strcmp(op, "|") == 0
                    ? AST_OR
                    : AST_XOR,
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _Logical(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Bitwise(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("&&") || CHECKTV("||")) {
        // && | ||
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _Bitwise(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            strcmp(op, "&&") == 0 
                ? AST_LAND 
                : AST_LOR,
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _Expression(Parser* parser) {
    return _Logical(parser);
}

static Ast* _ListOfExpressions(Parser* parser) {
    Ast* head = NULL;
    Ast* tail = NULL;
    
    Ast* expression = _Expression(parser);
    if (expression != NULL) {
        head = expression;
        tail = expression;
        
        while (CHECKTV(",")) {
            ACCEPTV_FREE(",");
            expression = _Expression(parser);
            if (expression == NULL) {
                break;
            }
            tail->Next = expression;
            tail = expression;
        }
    }
    
    return head;
}

static Ast* _ListOfStatements(Parser* parser);

static Ast* _Function(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast* fnName = NULL, *parameters = NULL, *current = parameters, *body = NULL;
    ACCEPTV_FREE(KEY_FN);
    fnName = _Terminal(parser);
    if (fnName == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a function name"
        );
    }
    if (fnName->Type != AST_NAME) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            fnName->Position, 
            "expected an identifier or name"
        );
    }
    ACCEPTV_FREE("(");
    current = parameters = _ListOfExpressions(parser);
    while (current != NULL) {
        if (current->Type != AST_NAME) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                current->Position, 
                "expected an identifier or name"
            );
        }
        current = current->Next;
    }
    ACCEPTV_FREE(")");
    ACCEPTV_FREE("{");
    body  = _ListOfStatements(parser);
    ended = parser->Next.Position;
    ACCEPTV_FREE("}");
    return AstFunction(
        fnName,
        parameters,
        body,
        MergePositions(start, ended)
    );
}

static Ast* _Statement(Parser* parser) {
    if (CHECKTV(KEY_FN)) {
        return _Function(parser);
    }
    return _Expression(parser);
}

static Ast* _ListOfStatements(Parser* parser) {
    Ast* head = NULL;
    Ast* tail = NULL;
    
    Ast* statement = _Statement(parser);
    if (statement != NULL) {
        head = statement;
        tail = statement;
        
        while (CHECKTT(TK_EOF) == false) {
            statement = _Statement(parser);
            if (statement == NULL) {
                break;
            }
            tail->Next = statement;
            tail = statement;
        }
    }
    
    return head;
}

static Ast* _ParseProgram(Parser* parser) {
    Ast* ast = AstProgram(_ListOfStatements(parser), parser->Next.Position);
    ACCEPTT(TK_EOF);
    return ast;
}

Ast* Parse(Parser* parser) {
    parser->Next = NextToken(parser->Lexer);
    return _ParseProgram(parser);
}

void FreeParser(Parser* parser) {
    free(parser);
}