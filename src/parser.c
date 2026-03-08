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

static String _GetTokenTypeName(TokenKind type) {
    switch (type) {
        case TK_KEY:
            return "keyword";
        case TK_IDN:
            return "identifier";
        case TK_INT:
            return "integer";
        case TK_NUM:
            return "number";
        case TK_STR:
            return "string";
        case TK_EOF:
            return "end of file";
        default:
            return "unknown";
    }
}

static int _CheckTokenV(Parser* parser, String value) {
    return strcmp(parser->Next.Value, value) == 0 && (
        parser->Next.Type == TK_IDN ||
        parser->Next.Type == TK_KEY ||
        parser->Next.Type == TK_SYM
    );
}

static int _CheckTokenT(Parser* parser, TokenKind type) {
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

static void _AcceptTokenT(Parser* parser, TokenKind type) {
    if (_CheckTokenT(parser, type)) {
        parser->Next = NextToken(parser->Lexer);
        return;
    }
    char message[256];
    snprintf(message, sizeof(message), "expected token type %s, got %s", _GetTokenTypeName(type), _GetTokenTypeName(parser->Next.Type));
    ThrowError(
        parser->Lexer->Path, 
        parser->Lexer->Data, 
        parser->Next.Position, 
        message
    );
}

static Ast* _Expression(Parser* parser);

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
        case TK_KEY: {
            if (strcmp(parser->Next.Value, KEY_TRUE) == 0 || strcmp(parser->Next.Value, KEY_FALSE) == 0) {
                node = AstBool(strcmp(parser->Next.Value, KEY_TRUE) == 0, parser->Next.Position);
                //NOTE: memory leak (ACCEPTT advances to the next token, discarding Value without freeing)
                ACCEPTT(TK_KEY);
            } else if (strcmp(parser->Next.Value, KEY_NULL) == 0) {
                node = AstNull(parser->Next.Position);
                //NOTE: memory leak (ACCEPTT advances to the next token, discarding Value without freeing)
                ACCEPTT(TK_KEY);
            } else if (strcmp(parser->Next.Value, KEY_THIS) == 0) {
                node = AstThis(parser->Next.Position);
                //NOTE: memory leak (ACCEPTT advances to the next token, discarding Value without freeing)
                ACCEPTT(TK_KEY);
            } else {
                ThrowError(
                    parser->Lexer->Path, 
                    parser->Lexer->Data, 
                    parser->Next.Position, 
                    "expected true, false, or null"
                ); 
            }
            break;
        }
        default:
            return NULL;
    }
    return node;
}

static Ast* _ListOfStatements(Parser* parser);
static Ast* _List(Parser* parser);
static Ast* _Object(Parser* parser);
static Ast* _FunctionExpression(Parser* parser);

static Ast* _Group(Parser* parser) {
    if (CHECKTV("[")) {
        return _List(parser);
    } else if (CHECKTV("{")) {
        return _Object(parser);
    } else if (CHECKTV("(")) {
        ACCEPTV_FREE("(");
        Ast* expr = _Expression(parser);
        if (expr == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                parser->Next.Position, 
                "expected an expression"
            );
        }
        ACCEPTV_FREE(")");
        return expr;
    } else if (CHECKTV(KEY_FN)) {
        return _FunctionExpression(parser);
    }
    return _Terminal(parser);
}

static Ast* _ListElement(Parser* parser) {
    if (CHECKTV("...")) {
        ACCEPTV_FREE("...");
        Ast* spreadValue = _Expression(parser);
        if (spreadValue == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                parser->Next.Position, 
                "expected an expression after spread operator"
            );
        }
        return AstSpread(
            spreadValue, 
            spreadValue->Position
        );
    }

    return _Expression(parser);
}

static Ast* _List(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE("[");
    
    Ast* head = NULL;
    Ast* tail = NULL;
    
    Ast* element = _ListElement(parser);
    if (element != NULL) {
        head = element;
        tail = element;
        
        while (CHECKTV(",")) {
            ACCEPTV_FREE(",");
            element = _ListElement(parser);
            if (element == NULL) {
                ThrowError(
                    parser->Lexer->Path, 
                    parser->Lexer->Data, 
                    tail->Position, 
                    "expected list element after comma"
                );
            }
            tail->Next = element;
            tail = element;
        }
    }
    
    ended = parser->Next.Position;
    ACCEPTV_FREE("]");
    
    return AstListLiteral(head, MergePositions(start, ended));
}

static Ast*_ObjectElement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast* element = NULL;
    if (CHECKTV("...")) {
        ACCEPTV_FREE("...");
        Ast* spreadValue = _Expression(parser);
        if (spreadValue == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                parser->Next.Position, 
                "expected an expression after spread operator"
            );
        }
        ended = spreadValue->Position;
        return AstSpread(
            spreadValue, 
            MergePositions(start, ended)
        );
    }
    element = _Terminal(parser);
    if (element == NULL) {
        return NULL;
    }
    if (element->Type != AST_NAME) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            element->Position, 
            "expected an identifier or name for object key"
        );
    }
    if (!CHECKTV(":")) {
        return element;
    }
    ACCEPTV_FREE(":");
    element->B = _Expression(parser);
    if (element->B == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            parser->Next.Position, 
            "expected an expression after ':'"
        );
    }
    ended = element->B->Position;
    return AstObjectKeyVal(
        element, 
        MergePositions(start, ended)
    );
}

static Ast* _Object(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE("{");

    Ast* head = NULL;
    Ast* tail = NULL;
    
    Ast* element = _ObjectElement(parser);
    if (element != NULL) {
        head = element;
        tail = element;
        
        while (CHECKTV(",")) {
            ACCEPTV_FREE(",");
            element = _ObjectElement(parser);
            if (element == NULL) {
                ThrowError(
                    parser->Lexer->Path, 
                    parser->Lexer->Data, 
                    tail->Position, 
                    "expected object element after comma"
                );
            }
            tail->Next = element;
            tail = element;
        }
    }
    
    ended = parser->Next.Position;
    ACCEPTV_FREE("}");
    
    return AstObjectLiteral(head, MergePositions(start, ended));
}

static Ast* _FunctionExpression(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast* parameters = NULL, *current = parameters, *body = NULL;
    ACCEPTV_FREE(KEY_FN);
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
    return AstFunction(NULL, parameters, body, MergePositions(start, ended));
}

static Ast* _Allocation(Parser* parser) {
    if (CHECKTV("new")) {
        Position start = parser->Next.Position, ended = start;
        ACCEPTV_FREE("new");
        Ast* cls = _Group(parser);
        if (cls == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                parser->Next.Position, 
                "expected a class"
            );
        }
        ACCEPTV_FREE("(");
        Ast* arguments = _ListOfExpressions(parser);
        ended = parser->Next.Position;
        ACCEPTV_FREE(")");
        return AstAllocation(
            cls, 
            arguments, 
            MergePositions(start, ended)
        );
    }
    return _Group(parser);
}

static Ast* _MemberOrCall(Parser* parser) {
    Ast* call = _Allocation(parser);
    if (call == NULL) {
        return NULL;
    }

    while (CHECKTV(".") || CHECKTV("[") || CHECKTV("(")) {
        if (CHECKTV(".")) {
            ACCEPTV_FREE(".");
            Ast* member = _Terminal(parser);
            if (member == NULL || member->Type != AST_NAME) {
                ThrowError(
                    parser->Lexer->Path, 
                    parser->Lexer->Data, 
                    call->Position, 
                    "expected a member name"
                );
            }
            call = AstMember(
                call, 
                member, 
                MergePositions(call->Position, member->Position)
            );
        } else if (CHECKTV("[")) {
            ACCEPTV_FREE("[");
            Ast* index = _Expression(parser);
            if (index == NULL) {
                ThrowError(
                    parser->Lexer->Path, 
                    parser->Lexer->Data, 
                    call->Position, 
                    "expected an expression"
                );
            }
            Position ended = parser->Next.Position;
            ACCEPTV_FREE("]");
            call = AstIndex(
                call, 
                index, 
                MergePositions(call->Position, ended)
            );
        } else if (CHECKTV("(")) {
            ACCEPTV_FREE("(");
            Ast* arguments = _ListOfExpressions(parser);
            Position ended = parser->Next.Position;
            ACCEPTV_FREE(")");
            call = AstCall(call, arguments, MergePositions(call->Position, ended));
        } else {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                call->Position, 
                "expected a member name, index, or call"
            );
        }
    }
    return call;
}

static Ast* _Postfix(Parser* parser) {
    Ast* node = _MemberOrCall(parser);
    if (node == NULL) {
        return NULL;
    }
    while (CHECKTV("++") || CHECKTV("--")) {
        String op = parser->Next.Value;
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
        ACCEPTT(TK_SYM);
        node = AstSingle(
            strcmp(op, "++") == 0 
                ? AST_POST_INC 
                : AST_POST_DEC,
            node, 
            MergePositions(node->Position, node->Position)
        );
        free(op);
    }
    return node;
}

static Ast* _Unary(Parser* parser) {
    String op = NULL;
    if (CHECKTV("+") || CHECKTV("-") || CHECKTV("!")) {
        op = parser->Next.Value;
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
        ACCEPTT(TK_SYM);
        Ast* operand = _Unary(parser);
        if (operand == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                parser->Next.Position, 
                "expected an expression"
            );
        }
        return AstSingle(
            strcmp(op, "+") == 0 
                ? AST_POSITIVE 
                : strcmp(op, "-") == 0 
                    ? AST_NEGATIVE 
                    : AST_LOGICAL_NOT,
            operand, 
            MergePositions(operand->Position, operand->Position)
        );
    } else if (CHECKTV("++") || CHECKTV("--")) {
        op = parser->Next.Value;
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
        ACCEPTT(TK_SYM);
        Ast* operand = _MemberOrCall(parser);
        if (operand == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                parser->Next.Position, 
                "expected an expression"
            );
        }
        return AstSingle(
            strcmp(op, "++") == 0 
                ? AST_PRE_INC 
                : AST_PRE_DEC,
            operand, 
            MergePositions(operand->Position, operand->Position)
        );
    }
    return _Postfix(parser);
}

static Ast* _Multiplicative(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Unary(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("*") || CHECKTV("/") || CHECKTV("%")) {
        // * | / | %
        op = parser->Next.Value;
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
        ACCEPTT(TK_SYM);

        rhs = _Unary(parser);

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
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
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
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
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
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
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
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
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
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
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
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
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

static Ast* _Assignment(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Logical(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("=")) {
        // =
        op = parser->Next.Value;
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
        ACCEPTT(TK_SYM);

        rhs = _Logical(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            AST_ASSIGN,
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _AugmentedMuliplicative(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Assignment(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("*=") || CHECKTV("/=") || CHECKTV("%=")) {
        // = | /= | %=
        op = parser->Next.Value;
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
        ACCEPTT(TK_SYM);

        rhs = _Assignment(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            (strcmp(op, "*=") == 0 
                ? AST_MUL_ASSIGN 
                : strcmp(op, "/=") == 0
                    ? AST_DIV_ASSIGN
                    : AST_MOD_ASSIGN),
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _AugmentedAddetive(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _AugmentedMuliplicative(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("+=") || CHECKTV("-=")) {
        // += | -=
        op = parser->Next.Value;
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
        ACCEPTT(TK_SYM);

        rhs = _AugmentedMuliplicative(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            (strcmp(op, "+=") == 0 
                ? AST_ADD_ASSIGN 
                : AST_SUB_ASSIGN),
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _AugmentedShift(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _AugmentedAddetive(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("<<=") || CHECKTV(">>=")) {
        // <<= | >>=
        op = parser->Next.Value;
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
        ACCEPTT(TK_SYM);

        rhs = _AugmentedAddetive(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            (strcmp(op, "<<=") == 0 
                ? AST_LSHFT_ASSIGN 
                : AST_RSHFT_ASSIGN),
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _AugmentedBitwise(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _AugmentedShift(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("&=") || CHECKTV("|=") || CHECKTV("^=")) {
        // &= | |= | ^=
        op = parser->Next.Value;
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
        ACCEPTT(TK_SYM);

        rhs = _AugmentedShift(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            (strcmp(op, "&=") == 0 
                ? AST_AND_ASSIGN 
                : strcmp(op, "|=") == 0
                    ? AST_OR_ASSIGN
                    : AST_XOR_ASSIGN),
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _Expression(Parser* parser) {
    return _AugmentedBitwise(parser);
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
                ThrowError(
                    parser->Lexer->Path, 
                    parser->Lexer->Data, 
                    tail->Position, 
                    "expected expression after comma"
                );
            }
            tail->Next = expression;
            tail = expression;
        }
    }
    
    return head;
}

static Ast* _Function(Parser* parser);
static Ast* _Statement(Parser* parser);

static Ast* _ClassMember(Parser* parser) {
    bool _static_ = false;
    Ast* node     = NULL;

    if (CHECKTV(KEY_STATIC)) {
        ACCEPTV_FREE(KEY_STATIC);
        _static_ = true;
    }

    if (CHECKTV(KEY_FN)) {
        node = _Function(parser);
        if (strcmp(node->A->Value, CONSTRUCTOR_NAME) == 0 && _static_) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                node->Position, 
                "constructor cannot be static"
            );
        }
    } else if (_static_ && CHECKTT(TK_IDN)) {
        node = _Assignment(parser);
        if (node->Type != AST_ASSIGN) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                node->Position, 
                "expected a method or function declaration"
            );
        }
        if (node->A->Type != AST_NAME) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                node->A->Position, 
                "expected an identifier or name for member"
            );
        }
        ACCEPTV_FREE(";");
    }

    if (node == NULL) {
        return NULL;
    }

    return AstClassMember(
        _static_,
        node,
        node->Position
    );
}

static Ast* _ListOfClassMembers(Parser* parser) {
    Ast* head = NULL;
    Ast* tail = NULL;
    
    Ast* member = _ClassMember(parser);
    if (member != NULL) {
        head = member;
        tail = member;
        
        while (true) {
            member = _ClassMember(parser);
            if (member == NULL) {
                break;
            }
            tail->Next = member;
            tail = member;
        }
    }
    
    return head;
}

static Ast* _Class(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast* className = NULL, *body = NULL;
    ACCEPTV_FREE(KEY_CLASS);
    className = _Terminal(parser);
    if (className == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a class name"
        );
    }
    if (className->Type != AST_NAME) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            className->Position, 
            "expected an identifier or name"
        );
    }
    Ast* superClass = NULL;
    if (CHECKTV("(")) {
        ACCEPTV_FREE("(");
        superClass = _Expression(parser);
        if (superClass == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                start, 
                "expected a superclass name"
            );
        }
        ACCEPTV_FREE(")");
    }
    ACCEPTV_FREE("{");
    body  = _ListOfClassMembers(parser);
    ended = parser->Next.Position;
    ACCEPTV_FREE("}");
    return AstClass(
        className,
        superClass,
        body,
        MergePositions(start, ended)
    );
}

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

static Ast* _ImportStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_IMPORT);
    Ast* current = NULL, *imports = current;
    if (CHECKTV("{")) {
        ACCEPTV_FREE("{");
        current = _ListOfExpressions(parser), imports = current;
        if (current == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                start, 
                "expected a list of imports"
            );
        }
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
        ACCEPTV_FREE("}");
        ACCEPTV_FREE(KEY_FROM);
    }
    Ast* moduleName = _Terminal(parser);
    if (moduleName == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a module name"
        );
    }
    if (moduleName->Type != AST_STR) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            moduleName->Position, 
            "expected a string or path"
        );
    }
    ACCEPTV_FREE(";");
    return AstImport(
        imports,
        moduleName, 
        MergePositions(start, ended)
    );
}

static Ast* _DeclarationList(Parser* parser);

static Ast* _InitializerConditionMutator(Parser* parser) {
    String op = NULL;
    Ast* lhs  = _Expression(parser), *rhs = NULL;
    
    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV(":=")) {
        if (lhs->Type != AST_NAME) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "expected an identifier or name"
            );
        }

        // =
        op = parser->Next.Value;
        //NOTE: memory leak (ACCEPTT advances to the next token, effectively discarding the current token's allocated Value string without freeing it)
        ACCEPTT(TK_SYM);

        rhs = _Expression(parser);
        
        if (rhs == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                lhs->Position, 
                "missing right operand"
            );
        }

        lhs = AstBinary(
            AST_SHORT_ASSIGN,
            lhs, 
            rhs, 
            MergePositions(lhs->Position, rhs->Position)
        );

        free(op);
    }

    return lhs;
}

static Ast* _VarStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_VAR);
    Ast* declarations = _DeclarationList(parser);
    ended = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstVarDeclaration(
        AST_VAR_DECLARATION,
        declarations,
        MergePositions(start, ended)
    );
}

static Ast* _ConstStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_CONST);
    Ast* declarations = _DeclarationList(parser);
    ended = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstVarDeclaration(
        AST_CONST_DECLARATION,
        declarations,
        MergePositions(start, ended)
    );
}

static Ast* _LocalStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_LOCAL);
    Ast* declarations = _DeclarationList(parser);
    ended = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstVarDeclaration(
        AST_LOCAL_DECLARATION,
        declarations,
        MergePositions(start, ended)
    );
}

static Ast* _DeclarationList(Parser* parser) {
    Ast* head = NULL;
    Ast* tail = NULL;
    
    do {
        Ast* dec = _Terminal(parser);

        if (dec == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                parser->Next.Position, 
                "expected a declaration"
            );
        }

        if (dec->Type != AST_NAME) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                dec->Position, 
                "expected an identifier or name"
            );
        }

        if (CHECKTV("=")) {
            ACCEPTV_FREE("=");
            Ast* value = _Expression(parser);
            if (value == NULL) {
                ThrowError(
                    parser->Lexer->Path, 
                    parser->Lexer->Data, 
                    dec->Position, 
                    "expected an expression"
                );
            }
            dec->B = value;
        }

        if (head == NULL) {
            head = dec;
            tail = dec;
        } else {
            tail->Next = dec;
            tail = dec;
        }

        if (!CHECKTV(",")) {
            break;
        }
        ACCEPTV_FREE(",");
    } while (1);
    
    return head;
}

static Ast* _IfStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast* initializerCondition = NULL, *thenBranch = NULL, *elseBranch = NULL;
    ACCEPTV_FREE(KEY_IF);
    ACCEPTV_FREE("(");
    initializerCondition = _InitializerConditionMutator(parser);
    if (initializerCondition == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a condition"
        );
    }
    if (CHECKTV(";")) {
        ACCEPTV_FREE(";");
        initializerCondition->Next = _Expression(parser);
        if (initializerCondition->Next == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                parser->Next.Position, 
                "expected a condition"
            );
        }
    }
    ACCEPTV_FREE(")");
    thenBranch = _Statement(parser);
    if (thenBranch == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a then branch"
        );
    }
    if (CHECKTV(KEY_ELSE)) {
        ACCEPTV_FREE(KEY_ELSE);
        elseBranch = _Statement(parser);
        if (elseBranch == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                start, 
                "expected an else branch"
            );
        }
        ended = parser->Next.Position;
    }
    
    return AstIf(
        initializerCondition,
        thenBranch, 
        elseBranch, 
        MergePositions(start, ended)
    );
}

static Ast* _ForStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast* initializerConditionMutator = NULL, *body = NULL;
    ACCEPTV_FREE(KEY_FOR);
    ACCEPTV_FREE("(");
    initializerConditionMutator = _InitializerConditionMutator(parser);
    ACCEPTV_FREE(";");
    if (initializerConditionMutator != NULL) {
        initializerConditionMutator->Next = _Expression(parser);
    }
    ACCEPTV_FREE(";");
    if (initializerConditionMutator != NULL && initializerConditionMutator->Next != NULL) {
        initializerConditionMutator->Next->Next = _Expression(parser);
    }
    ACCEPTV_FREE(")");
    body = _Statement(parser);
    if (body == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a body"
        );
    }
    ended = body->Position;
    return AstFor(
        initializerConditionMutator,
        body, 
        MergePositions(start, ended)
    );
}

static Ast* _WhileStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast* initializerConditionMutator = NULL, *body = NULL;
    ACCEPTV_FREE(KEY_WHILE);
    ACCEPTV_FREE("(");
    initializerConditionMutator = _InitializerConditionMutator(parser);
    if (initializerConditionMutator == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a condition"
        );
    }

    if (CHECKTV(";")) {
        // Condition?
        ACCEPTV_FREE(";");
        initializerConditionMutator->Next = _Expression(parser);
        if (initializerConditionMutator->Next == NULL) {
            ThrowError(
                parser->Lexer->Path, 
                parser->Lexer->Data, 
                parser->Next.Position, 
                "expected a condition"
            );
        }

        if (CHECKTV(";")) {
            // Mutator?
            ACCEPTV_FREE(";");
            initializerConditionMutator->Next->Next = _Expression(parser);
            if (initializerConditionMutator->Next->Next == NULL) {
                ThrowError(
                    parser->Lexer->Path, 
                    parser->Lexer->Data, 
                    parser->Next.Position, 
                    "expected a mutator"
                );
            }
        }
    }
    ACCEPTV_FREE(")");
    body = _Statement(parser);
    if (body == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a body"
        );
    }
    ended = body->Position;
    return AstWhile(
        initializerConditionMutator, 
        body, 
        MergePositions(start, ended)
    );
}

static Ast* _DoWhileStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast* body = NULL, *condition = NULL;
    ACCEPTV_FREE(KEY_DO);
    body = _Statement(parser);
    if (body == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a body"
        );
    }
    ACCEPTV_FREE("while");
    ACCEPTV_FREE("(");
    condition = _Expression(parser);
    if (condition == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a condition"
        );
    }
    ended = parser->Next.Position;
    ACCEPTV_FREE(")");
    return AstDoWhile(
        condition, 
        body, 
        MergePositions(start, ended)
    );
}

static Ast* _TryCatchStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast* tryBlock = NULL, *catchBlock = NULL, *errorName = NULL;
    ACCEPTV_FREE(KEY_TRY);
    tryBlock = _Statement(parser);
    if (tryBlock == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a try block"
        );
    }
    if (tryBlock->Type != AST_BLOCK) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            tryBlock->Position, 
            "expected a block statement"
        );
    }
    ACCEPTV_FREE(KEY_CATCH);
    ACCEPTV_FREE("(");
    errorName = _Terminal(parser);
    if (errorName == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected an error name"
        );
    }
    if (errorName->Type != AST_NAME) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            errorName->Position, 
            "expected an identifier or name"
        );
    }
    ACCEPTV_FREE(")");
    catchBlock = _Statement(parser);
    if (catchBlock == NULL) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            start, 
            "expected a catch block"
        );
    }
    if (catchBlock->Type != AST_BLOCK) {
        ThrowError(
            parser->Lexer->Path, 
            parser->Lexer->Data, 
            tryBlock->Position, 
            "expected a block statement"
        );
    }
    ended = parser->Next.Position;
    return AstTryCatch(
        tryBlock,
        errorName,
        catchBlock,
        MergePositions(start, ended)
    );
}

static Ast* _BlockStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast* statements = NULL;
    ACCEPTV_FREE("{");
    statements = _ListOfStatements(parser);
    ended = parser->Next.Position;
    ACCEPTV_FREE("}");
    return AstBlock(
        statements, 
        MergePositions(start, ended)
    );
}

static Ast* _ContinueStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_CONTINUE);
    ended = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstContinue(
        MergePositions(start, ended)
    );
}

static Ast* _BreakStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_BREAK);
    ended = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstBreak(
        MergePositions(start, ended)
    );
}

static Ast* _ReturnStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_RETURN);
    Ast* expression = _Expression(parser);
    ended = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstReturn(
        expression, 
        MergePositions(start, ended)
    );
}

static Ast* _ExpressionStatement(Parser* parser) {
    Position start  = parser->Next.Position, ended = start;
    Ast* expression = _Expression(parser);
    if (expression == NULL) {
        while (CHECKTV(";")) {
            ended = parser->Next.Position;
            ACCEPTV_FREE(";");
        }
        return NULL;
    } else {
        ended = parser->Next.Position;
        ACCEPTV_FREE(";");
    }
    return AstExpressionStatement(
        expression, 
        MergePositions(start, ended)
    );
}

static Ast* _Statement(Parser* parser) {
    if (CHECKTV(KEY_CLASS)) {
        return _Class(parser);
    } else if (CHECKTV(KEY_FN)) {
        return _Function(parser);
    } else if (CHECKTV(KEY_IMPORT)) {
        return _ImportStatement(parser);
    } else if (CHECKTV(KEY_VAR)) {
        return _VarStatement(parser);
    } else if (CHECKTV(KEY_CONST)) {
        return _ConstStatement(parser);
    } else if (CHECKTV(KEY_LOCAL)) {
        return _LocalStatement(parser);
    } else if (CHECKTV(KEY_IF)) {
        return _IfStatement(parser);
    } else if (CHECKTV(KEY_FOR)) {
        return _ForStatement(parser);
    } else if (CHECKTV(KEY_WHILE)) {
        return _WhileStatement(parser);
    } else if (CHECKTV(KEY_DO)) {
        return _DoWhileStatement(parser);
    } else if (CHECKTV(KEY_TRY)) {
        return _TryCatchStatement(parser);
    }else if (CHECKTV(KEY_CONTINUE)) {
        return _ContinueStatement(parser);
    } else if (CHECKTV(KEY_BREAK)) {
        return _BreakStatement(parser);
    } else if (CHECKTV(KEY_RETURN)) {
        return _ReturnStatement(parser);
    } else if (CHECKTV("{")) {
        return _BlockStatement(parser);
    }
    return _ExpressionStatement(parser);
}

static Ast* _ListOfStatements(Parser* parser) {
    Ast* head = NULL;
    Ast* tail = NULL;
    
    while (CHECKTT(TK_EOF) == false) {
        Ast* statement = _Statement(parser);
        if (statement == NULL) {
            break;
        }
        
        if (head == NULL) {
            head = statement;
            tail = statement;
        } else {
            tail->Next = statement;
            tail = statement;
        }
    }
    
    return head;
}

static Ast* _ParseProgram(Parser* parser) {
    Ast* ast = AstProgram(_ListOfStatements(parser), parser->Next.Position);
    //NOTE: memory leak (ACCEPTT advances to the next token, discarding Value without freeing)
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