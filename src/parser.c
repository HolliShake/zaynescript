#include "./parser.h"

Parser* CreateParser(Lexer* lexer) {
    Parser* parser = Allocate(sizeof(Parser));
    parser->Lexer  = lexer;
    return parser;
}

#define CHECKTV(v) _CheckTokenV(parser, v)

#define CHECKTT(t) _CheckTokenT(parser, t)

#define ACCEPTV(v) _AcceptTokenV(parser, v)

#define ACCEPTV_FREE(v)                                                                            \
    {                                                                                              \
        String value = parser->Next.Value;                                                         \
        _AcceptTokenV(parser, v);                                                                  \
        free(value);                                                                               \
    }

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
    return strcmp(parser->Next.Value, value) == 0
           && (parser->Next.Type == TK_IDN || parser->Next.Type == TK_KEY
               || parser->Next.Type == TK_SYM);
}

static int _CheckTokenT(Parser* parser, TokenKind type) {
    return parser->Next.Type == type;
}

static void _AcceptTokenV(Parser* parser, String value) {
    if (parser->Next.Type == TK_EOF)
        return;
    if (_CheckTokenV(parser, value)) {
        parser->Next = NextToken(parser->Lexer);
        return;
    }
    char message[256];
    snprintf(message,
             sizeof(message),
             "expected token value %s, got %s",
             value,
             parser->Next.Value);
    ThrowError(parser->Lexer->Path, parser->Lexer->Data, parser->Next.Position, message);
}

static void _AcceptTokenT(Parser* parser, TokenKind type) {
    if (parser->Next.Type == TK_EOF)
        return;
    if (_CheckTokenT(parser, type)) {
        parser->Next = NextToken(parser->Lexer);
        return;
    }
    char message[256];
    snprintf(message,
             sizeof(message),
             "expected token type %s, got %s",
             _GetTokenTypeName(type),
             _GetTokenTypeName(parser->Next.Type));
    ThrowError(parser->Lexer->Path, parser->Lexer->Data, parser->Next.Position, message);
}

static Ast* _ParseExpression(Parser* parser);

static Ast* _ParseListOfExpressions(Parser* parser);

static Ast* _ParseTerminal(Parser* parser) {
    Ast* node = NULL;
    switch (parser->Next.Type) {
        case TK_IDN:
            {
                node = AstName(parser->Next.Value, parser->Next.Position);
                ACCEPTT(TK_IDN);
                break;
            }
        case TK_INT:
            {
                node = AstInteger(parser->Next.Value, parser->Next.Position);
                ACCEPTT(TK_INT);
                break;
            }
        case TK_BINT:
            {
                node = AstBigInteger(parser->Next.Value, parser->Next.Position);
                ACCEPTT(TK_BINT);
                break;
            }
        case TK_NUM:
            {
                node = AstNumber(parser->Next.Value, parser->Next.Position);
                ACCEPTT(TK_NUM);
                break;
            }
        case TK_BNUM:
            {
                node = AstBigNumber(parser->Next.Value, parser->Next.Position);
                ACCEPTT(TK_BNUM);
                break;
            }
        case TK_STR:
            {
                node = AstString(parser->Next.Value, parser->Next.Position);
                ACCEPTT(TK_STR);
                break;
            }
        case TK_KEY:
            {
                String key = parser->Next.Value;
                if (strcmp(key, KEY_TRUE) == 0 || strcmp(key, KEY_FALSE) == 0) {
                    node =
                        AstBool(strcmp(parser->Next.Value, KEY_TRUE) == 0, parser->Next.Position);
                    ACCEPTT(TK_KEY);
                    free(key);
                } else if (strcmp(key, KEY_NULL) == 0) {
                    node = AstNull(parser->Next.Position);
                    ACCEPTT(TK_KEY);
                    free(key);
                } else if (strcmp(key, KEY_THIS) == 0) {
                    node = AstThis(parser->Next.Position);
                    ACCEPTT(TK_KEY);
                    free(key);
                } else {
                    ThrowError(parser->Lexer->Path,
                               parser->Lexer->Data,
                               parser->Next.Position,
                               "expected true, false, or null");
                }
                break;
            }
        default:
            return NULL;
    }
    return node;
}

static Ast* _ParseListOfStatements(Parser* parser);
static Ast* _ParseList(Parser* parser);
static Ast* _ParseObject(Parser* parser);
static Ast* _ParseFunctionExpression(Parser* parser);

static Ast* _ParseGroup(Parser* parser) {
    if (CHECKTV("[")) {
        return _ParseList(parser);
    } else if (CHECKTV("{")) {
        return _ParseObject(parser);
    } else if (CHECKTV("(")) {
        ACCEPTV_FREE("(");
        Ast* expr = _ParseExpression(parser);
        if (expr == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected an expression");
        }
        ACCEPTV_FREE(")");
        return expr;
    } else if (CHECKTV(KEY_FN)) {
        return _ParseFunctionExpression(parser);
    }
    return _ParseTerminal(parser);
}

static Ast* _ListElement(Parser* parser) {
    if (CHECKTV("...")) {
        ACCEPTV_FREE("...");
        Ast* spreadValue = _ParseExpression(parser);
        if (spreadValue == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected an expression after spread operator");
        }
        return AstSpread(spreadValue, spreadValue->Position);
    }

    return _ParseExpression(parser);
}

static Ast* _ParseList(Parser* parser) {
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
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           tail->Position,
                           "expected list element after comma");
            }
            tail->Next = element;
            tail       = element;
        }
    }

    ended = parser->Next.Position;
    ACCEPTV_FREE("]");

    return AstListLiteral(head, MergePositions(start, ended));
}

static Ast* _ParseObjectElement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast*     element = NULL;
    if (CHECKTV("...")) {
        ACCEPTV_FREE("...");
        Ast* spreadValue = _ParseExpression(parser);
        if (spreadValue == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected an expression after spread operator");
        }
        ended = spreadValue->Position;
        return AstSpread(spreadValue, MergePositions(start, ended));
    }
    element = _ParseTerminal(parser);
    if (element == NULL) {
        return NULL;
    }
    if (element->Type != AST_NAME) {
        ThrowError(parser->Lexer->Path,
                   parser->Lexer->Data,
                   element->Position,
                   "expected an identifier or name for object key");
    }
    if (!CHECKTV(":")) {
        return element;
    }
    ACCEPTV_FREE(":");
    element->B = _ParseExpression(parser);
    if (element->B == NULL) {
        ThrowError(parser->Lexer->Path,
                   parser->Lexer->Data,
                   parser->Next.Position,
                   "expected an expression after ':'");
    }
    ended = element->B->Position;
    return AstObjectKeyVal(element, MergePositions(start, ended));
}

static Ast* _ParseObject(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE("{");

    Ast* head = NULL;
    Ast* tail = NULL;

    Ast* element = _ParseObjectElement(parser);
    if (element != NULL) {
        head = element;
        tail = element;

        while (CHECKTV(",")) {
            ACCEPTV_FREE(",");
            element = _ParseObjectElement(parser);
            if (element == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           tail->Position,
                           "expected object element after comma");
            }
            tail->Next = element;
            tail       = element;
        }
    }

    ended = parser->Next.Position;
    ACCEPTV_FREE("}");

    return AstObjectLiteral(head, MergePositions(start, ended));
}

static Ast* _ParseFunctionExpression(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast *    parameters = NULL, *current = parameters, *body = NULL;
    ACCEPTV_FREE(KEY_FN);
    ACCEPTV_FREE("(");
    current = parameters = _ParseListOfExpressions(parser);
    while (current != NULL) {
        if (current->Type != AST_NAME) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       current->Position,
                       "expected an identifier or name");
        }
        current = current->Next;
    }
    ACCEPTV_FREE(")");
    bool async = CHECKTV(KEY_ASYNC);
    if (async) {
        ACCEPTV_FREE(KEY_ASYNC);
    }
    ACCEPTV_FREE("{");
    body  = _ParseListOfStatements(parser);
    ended = parser->Next.Position;
    ACCEPTV_FREE("}");
    return AstFunction(NULL, parameters, body, async, MergePositions(start, ended));
}

static Ast* _ParseAllocation(Parser* parser) {
    if (CHECKTV(KEY_NEW)) {
        Position start = parser->Next.Position, ended = start;
        ACCEPTV_FREE(KEY_NEW);

        Ast* cls = _ParseGroup(parser);

        if (cls == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected a class");
        }

        ACCEPTV_FREE("(");
        Ast* arguments = _ParseListOfExpressions(parser);
        ended          = parser->Next.Position;
        ACCEPTV_FREE(")");

        return AstAllocation(cls, arguments, MergePositions(start, ended));
    }
    return _ParseGroup(parser);
}

static Ast* _ParseMemberOrCall(Parser* parser) {
    Ast* call = _ParseAllocation(parser);
    if (call == NULL) {
        return NULL;
    }

    while (CHECKTV(".") || CHECKTV("[") || CHECKTV("(")) {
        if (CHECKTV(".")) {
            ACCEPTV_FREE(".");

            Ast* member = _ParseTerminal(parser);

            if (member == NULL || member->Type != AST_NAME) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           call->Position,
                           "expected a member name");
            }

            call = AstMember(call, member, MergePositions(call->Position, member->Position));
        } else if (CHECKTV("[")) {
            ACCEPTV_FREE("[");

            Ast* index = _ParseExpression(parser);

            if (index == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           call->Position,
                           "expected an expression");
            }

            Position ended = parser->Next.Position;
            ACCEPTV_FREE("]");

            call = AstIndex(call, index, MergePositions(call->Position, ended));
        } else if (CHECKTV("(")) {
            ACCEPTV_FREE("(");
            Ast*     arguments = _ParseListOfExpressions(parser);
            Position ended     = parser->Next.Position;
            ACCEPTV_FREE(")");

            call = AstCall(call, arguments, MergePositions(call->Position, ended));
        } else {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       call->Position,
                       "expected a member name, index, or call");
        }
    }

    return call;
}

static Ast* _ParsePostfix(Parser* parser) {
    Ast* node = _ParseMemberOrCall(parser);
    if (node == NULL) {
        return NULL;
    }

    while (CHECKTV("++") || CHECKTV("--")) {
        String op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        node = AstSingle(strcmp(op, "++") == 0 ? AST_POST_INC : AST_POST_DEC,
                         node,
                         MergePositions(node->Position, node->Position));

        free(op);
    }

    return node;
}

static Ast* _ParseUnary(Parser* parser) {
    String op   = NULL;
    Ast*   node = NULL;
    if (CHECKTV("+") || CHECKTV("-") || CHECKTV("!")) {
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        Ast* operand = _ParseUnary(parser);

        if (operand == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected an expression");
        }

        node = AstSingle(strcmp(op, "+") == 0   ? AST_POSITIVE
                         : strcmp(op, "-") == 0 ? AST_NEGATIVE
                                                : AST_LOGICAL_NOT,
                         operand,
                         MergePositions(operand->Position, operand->Position));

        free(op);

        return node;
    } else if (CHECKTV("++") || CHECKTV("--")) {
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        Ast* operand = _ParseUnary(parser);

        if (operand == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected an expression");
        }

        node = AstSingle(strcmp(op, "++") == 0 ? AST_PRE_INC : AST_PRE_DEC,
                         operand,
                         MergePositions(operand->Position, operand->Position));

        free(op);

        return node;
    } else if (CHECKTV(KEY_AWAIT)) {
        ACCEPTV_FREE(KEY_AWAIT);

        Ast* operand = _ParseUnary(parser);

        if (operand == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected an expression");
        }

        if (operand->Type != AST_CALL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       operand->Position,
                       "await can only be applied to function calls");
        }

        return AstSingle(AST_AWAIT, operand, MergePositions(operand->Position, operand->Position));
    }

    return _ParsePostfix(parser);
}

static Ast* _ParseMultiplicative(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseUnary(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("*") || CHECKTV("/") || CHECKTV("%")) {
        // * | / | %
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseUnary(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary(strcmp(op, "*") == 0   ? AST_MUL
                        : strcmp(op, "/") == 0 ? AST_DIV
                                               : AST_MOD,
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseAddetive(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseMultiplicative(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("+") || CHECKTV("-")) {
        // + | -
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseMultiplicative(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary(strcmp(op, "+") == 0 ? AST_ADD : AST_SUB,
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseShift(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseAddetive(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("<<") || CHECKTV(">>")) {
        // << | >>
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseAddetive(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary(strcmp(op, "<<") == 0 ? AST_LSHFT : AST_RSHFT,
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseRelational(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseShift(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("<") || CHECKTV("<=") || CHECKTV(">") || CHECKTV(">=")) {
        // < | <= | > | >=
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseShift(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary(strcmp(op, "<") == 0    ? AST_LT
                        : strcmp(op, "<=") == 0 ? AST_LTE
                        : strcmp(op, ">") == 0  ? AST_GT
                                                : AST_GTE,
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseEquality(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseRelational(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("==") || CHECKTV("!=")) {
        // == | !=
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseRelational(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary(strcmp(op, "==") == 0 ? AST_EQ : AST_NE,
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseBitwise(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseEquality(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("&") || CHECKTV("|") || CHECKTV("^")) {
        // & | | | ^
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseEquality(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary(strcmp(op, "&") == 0   ? AST_AND
                        : strcmp(op, "|") == 0 ? AST_OR
                                               : AST_XOR,
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseLogical(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseBitwise(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("&&") || CHECKTV("||")) {
        // && | ||
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseBitwise(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary(strcmp(op, "&&") == 0 ? AST_LAND : AST_LOR,
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseSwitchExpression(Parser* parser) {
    Position start = parser->Next.Position, ended = start;

    Ast* node = _ParseLogical(parser);

    if (node == NULL) {
        return NULL;
    }

    if (!CHECKTV(KEY_SWITCH)) {
        return node;
    }

    ACCEPTV_FREE(KEY_SWITCH);
    ACCEPTV_FREE("{");
    Ast *cases = NULL, *current = cases, *defaultCase = NULL;

    while (CHECKTV(KEY_CASE) || CHECKTV(KEY_DEFAULT)) {
        if (CHECKTV(KEY_CASE)) {
            ACCEPTV_FREE(KEY_CASE);
            Ast* caseValue = _ParseListOfExpressions(parser);
            if (caseValue == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           parser->Next.Position,
                           "expected an expression after 'case'");
            }
            ACCEPTV_FREE("=>");
            Ast* caseBody = _ParseExpression(parser);
            if (caseBody == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           parser->Next.Position,
                           "expected an expression for case body");
            }

            Ast* newCase = AstSwitchCase(caseValue,
                                         caseBody,
                                         MergePositions(caseValue->Position, caseBody->Position));

            if (cases == NULL) {
                cases   = newCase;
                current = newCase;
            } else {
                current->Next = newCase;
                current       = newCase;
            }

        } else if (CHECKTV(KEY_DEFAULT)) {
            if (defaultCase != NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           parser->Next.Position,
                           "multiple default cases are not allowed");
            }
            ACCEPTV_FREE(KEY_DEFAULT);
            ACCEPTV_FREE("=>");
            defaultCase = _ParseExpression(parser);
            if (defaultCase == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           parser->Next.Position,
                           "expected an expression for default case body");
            }
        }
    }

    ended = parser->Next.Position;
    ACCEPTV_FREE("}");
    return AstSwitch(node, cases, defaultCase, MergePositions(start, ended));
}

static Ast* _ParseTernaryOrIf(Parser* parser) {
    Ast* conditionOrTrue = _ParseSwitchExpression(parser);
    if (conditionOrTrue == NULL) {
        return NULL;
    }
    if (CHECKTV("?")) {
        ACCEPTV_FREE("?");
        Ast *trueBranch = _ParseTernaryOrIf(parser), *falseBranch = NULL;
        if (trueBranch == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       conditionOrTrue->Position,
                       "expected a true branch");
        }
        if (!CHECKTV(":")) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected a colon");
        }
        ACCEPTV_FREE(":");
        falseBranch = _ParseTernaryOrIf(parser);
        if (falseBranch == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       conditionOrTrue->Position,
                       "expected a false branch");
        }
        return AstTernary(conditionOrTrue,
                          trueBranch,
                          falseBranch,
                          MergePositions(conditionOrTrue->Position, falseBranch->Position));

    } else if (CHECKTV(KEY_IF)) {
        ACCEPTV_FREE(KEY_IF);
        ACCEPTV_FREE("(");
        Ast* condition = _ParseTernaryOrIf(parser);
        ACCEPTV_FREE(")");
        if (condition == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected a condition");
        }
        ACCEPTV_FREE(KEY_ELSE);
        Ast* elseBranch = _ParseTernaryOrIf(parser);
        if (elseBranch == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected a else branch");
        }
        return AstTernary(condition,
                          conditionOrTrue,
                          elseBranch,
                          MergePositions(conditionOrTrue->Position, elseBranch->Position));
    }
    return conditionOrTrue;
}

static Ast* _ParseAssignment(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseTernaryOrIf(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("=")) {
        // =
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseTernaryOrIf(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary(AST_ASSIGN, lhs, rhs, MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseAugmentedMuliplicative(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseAssignment(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("*=") || CHECKTV("/=") || CHECKTV("%=")) {
        // = | /= | %=
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseAssignment(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary((strcmp(op, "*=") == 0   ? AST_MUL_ASSIGN
                         : strcmp(op, "/=") == 0 ? AST_DIV_ASSIGN
                                                 : AST_MOD_ASSIGN),
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseAugmentedAddetive(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseAugmentedMuliplicative(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("+=") || CHECKTV("-=")) {
        // += | -=
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseAugmentedMuliplicative(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary((strcmp(op, "+=") == 0 ? AST_ADD_ASSIGN : AST_SUB_ASSIGN),
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseAugmentedShift(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseAugmentedAddetive(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("<<=") || CHECKTV(">>=")) {
        // <<= | >>=
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseAugmentedAddetive(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary((strcmp(op, "<<=") == 0 ? AST_LSHFT_ASSIGN : AST_RSHFT_ASSIGN),
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseAugmentedBitwise(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseAugmentedShift(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV("&=") || CHECKTV("|=") || CHECKTV("^=")) {
        // &= | |= | ^=
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseAugmentedShift(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary((strcmp(op, "&=") == 0   ? AST_AND_ASSIGN
                         : strcmp(op, "|=") == 0 ? AST_OR_ASSIGN
                                                 : AST_XOR_ASSIGN),
                        lhs,
                        rhs,
                        MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseExpression(Parser* parser) {
    return _ParseAugmentedBitwise(parser);
}

static Ast* _ParseListOfExpressions(Parser* parser) {
    Ast* head = NULL;
    Ast* tail = NULL;

    Ast* expression = _ParseExpression(parser);
    if (expression != NULL) {
        head = expression;
        tail = expression;

        while (CHECKTV(",")) {
            ACCEPTV_FREE(",");
            expression = _ParseExpression(parser);
            if (expression == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           tail->Position,
                           "expected expression after comma");
            }
            tail->Next = expression;
            tail       = expression;
        }
    }

    return head;
}

static Ast* _ParseFunction(Parser* parser);
static Ast* _ParseStatement(Parser* parser);

static Ast* _ParseClassMember(Parser* parser) {
    bool _static_ = false;
    Ast* node     = NULL;

    if (CHECKTV(KEY_STATIC)) {
        ACCEPTV_FREE(KEY_STATIC);
        _static_ = true;
    }

    if (CHECKTV(KEY_FN)) {
        node = _ParseFunction(parser);
        if (strcmp(node->A->Value, CONSTRUCTOR_NAME) == 0 && _static_) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       node->Position,
                       "constructor cannot be static");
        }
    } else if (_static_ && CHECKTT(TK_IDN)) {
        node = _ParseAssignment(parser);
        if (node->Type != AST_ASSIGN) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       node->Position,
                       "expected a method or function declaration");
        }
        if (node->A->Type != AST_NAME) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       node->A->Position,
                       "expected an identifier or name for member");
        }
        ACCEPTV_FREE(";");
    }

    if (node == NULL) {
        return NULL;
    }

    return AstClassMember(_static_, node, node->Position);
}

static Ast* _ParseListOfClassMembers(Parser* parser) {
    Ast* head = NULL;
    Ast* tail = NULL;

    Ast* member = _ParseClassMember(parser);
    if (member != NULL) {
        head = member;
        tail = member;

        while (true) {
            member = _ParseClassMember(parser);
            if (member == NULL) {
                break;
            }
            tail->Next = member;
            tail       = member;
        }
    }

    return head;
}

static Ast* _ParseClass(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast *    className = NULL, *body = NULL;
    ACCEPTV_FREE(KEY_CLASS);
    className = _ParseTerminal(parser);
    if (className == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a class name");
    }
    if (className->Type != AST_NAME) {
        ThrowError(parser->Lexer->Path,
                   parser->Lexer->Data,
                   className->Position,
                   "expected an identifier or name");
    }
    Ast* superClass = NULL;
    if (CHECKTV("(")) {
        ACCEPTV_FREE("(");
        superClass = _ParseExpression(parser);
        if (superClass == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       start,
                       "expected a superclass name");
        }
        ACCEPTV_FREE(")");
    }
    ACCEPTV_FREE("{");
    body  = _ParseListOfClassMembers(parser);
    ended = parser->Next.Position;
    ACCEPTV_FREE("}");
    return AstClass(className, superClass, body, MergePositions(start, ended));
}

static Ast* _ParseFunction(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast *    fnName = NULL, *parameters = NULL, *current = parameters, *body = NULL;
    ACCEPTV_FREE(KEY_FN);
    fnName = _ParseTerminal(parser);
    if (fnName == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a function name");
    }
    if (fnName->Type != AST_NAME) {
        ThrowError(parser->Lexer->Path,
                   parser->Lexer->Data,
                   fnName->Position,
                   "expected an identifier or name");
    }
    ACCEPTV_FREE("(");
    current = parameters = _ParseListOfExpressions(parser);
    while (current != NULL) {
        if (current->Type != AST_NAME) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       current->Position,
                       "expected an identifier or name");
        }
        current = current->Next;
    }
    ACCEPTV_FREE(")");
    bool async = CHECKTV(KEY_ASYNC);
    if (async) {
        ACCEPTV_FREE(KEY_ASYNC);
    }
    ACCEPTV_FREE("{");
    body  = _ParseListOfStatements(parser);
    ended = parser->Next.Position;
    ACCEPTV_FREE("}");
    return AstFunction(fnName, parameters, body, async, MergePositions(start, ended));
}

static Ast* _ParseImportStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_IMPORT);
    Ast *current = NULL, *imports = current;
    if (CHECKTV("{")) {
        ACCEPTV_FREE("{");
        current = _ParseListOfExpressions(parser), imports = current;
        if (current == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       start,
                       "expected a list of imports");
        }
        while (current != NULL) {
            if (current->Type != AST_NAME) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           current->Position,
                           "expected an identifier or name");
            }
            current = current->Next;
        }
        ACCEPTV_FREE("}");
        ACCEPTV_FREE(KEY_FROM);
    }
    Ast* moduleName = _ParseTerminal(parser);
    if (moduleName == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a module name");
    }
    if (moduleName->Type != AST_STR) {
        ThrowError(parser->Lexer->Path,
                   parser->Lexer->Data,
                   moduleName->Position,
                   "expected a string or path");
    }
    ACCEPTV_FREE(";");
    return AstImport(imports, moduleName, MergePositions(start, ended));
}

static Ast* _ParseDeclarationList(Parser* parser);

static Ast* _ParseInitializerConditionMutator(Parser* parser) {
    String op  = NULL;
    Ast *  lhs = _ParseExpression(parser), *rhs = NULL;

    if (lhs == NULL) {
        return NULL;
    }

    while (CHECKTV(":=")) {
        if (lhs->Type != AST_NAME) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "expected an identifier or name");
        }

        // :=
        op = parser->Next.Value;
        ACCEPTT(TK_SYM);

        rhs = _ParseExpression(parser);

        if (rhs == NULL) {
            free(op);
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       lhs->Position,
                       "missing right operand");
        }

        lhs = AstBinary(AST_SHORT_ASSIGN, lhs, rhs, MergePositions(lhs->Position, rhs->Position));

        free(op);
    }

    return lhs;
}

static Ast* _ParseVarStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_VAR);
    Ast* declarations = _ParseDeclarationList(parser);
    ended             = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstVarDeclaration(AST_VAR_DECLARATION, declarations, MergePositions(start, ended));
}

static Ast* _ParseConstStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_CONST);
    Ast* declarations = _ParseDeclarationList(parser);
    ended             = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstVarDeclaration(AST_CONST_DECLARATION, declarations, MergePositions(start, ended));
}

static Ast* _ParseLocalStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_LOCAL);
    Ast* declarations = _ParseDeclarationList(parser);
    ended             = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstVarDeclaration(AST_LOCAL_DECLARATION, declarations, MergePositions(start, ended));
}

static Ast* _ParseDeclarationList(Parser* parser) {
    Ast* head = NULL;
    Ast* tail = NULL;

    do {
        Ast* dec = _ParseTerminal(parser);

        if (dec == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected a declaration");
        }

        if (dec->Type != AST_NAME) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       dec->Position,
                       "expected an identifier or name");
        }

        if (CHECKTV("=")) {
            ACCEPTV_FREE("=");
            Ast* value = _ParseExpression(parser);
            if (value == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           dec->Position,
                           "expected an expression");
            }
            dec->B = value;
        }

        if (head == NULL) {
            head = dec;
            tail = dec;
        } else {
            tail->Next = dec;
            tail       = dec;
        }

        if (!CHECKTV(",")) {
            break;
        }
        ACCEPTV_FREE(",");
    } while (1);

    return head;
}

static Ast* _ParseIfStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast *    initializerCondition = NULL, *thenBranch = NULL, *elseBranch = NULL;
    ACCEPTV_FREE(KEY_IF);
    ACCEPTV_FREE("(");
    initializerCondition = _ParseInitializerConditionMutator(parser);
    if (initializerCondition == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a condition");
    }
    if (CHECKTV(";")) {
        ACCEPTV_FREE(";");
        initializerCondition->Next = _ParseExpression(parser);
        if (initializerCondition->Next == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected a condition");
        }
    }
    ACCEPTV_FREE(")");
    thenBranch = _ParseStatement(parser);
    if (thenBranch == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a then branch");
    }
    if (CHECKTV(KEY_ELSE)) {
        ACCEPTV_FREE(KEY_ELSE);
        elseBranch = _ParseStatement(parser);
        if (elseBranch == NULL) {
            ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected an else branch");
        }
        ended = parser->Next.Position;
    }

    return AstIf(initializerCondition, thenBranch, elseBranch, MergePositions(start, ended));
}

static Ast* _ParseSwitchStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_SWITCH);
    ACCEPTV_FREE("(");
    Ast* node = _ParseExpression(parser);
    if (node == NULL) {
        ThrowError(parser->Lexer->Path,
                   parser->Lexer->Data,
                   parser->Next.Position,
                   "expected an expression");
    }
    ACCEPTV_FREE(")");
    ACCEPTV_FREE("{");
    Ast *cases = NULL, *current = cases, *defaultCase = NULL;

    while (CHECKTV(KEY_CASE) || CHECKTV(KEY_DEFAULT)) {
        if (CHECKTV(KEY_CASE)) {
            ACCEPTV_FREE(KEY_CASE);
            Ast* caseValue = _ParseListOfExpressions(parser);
            if (caseValue == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           parser->Next.Position,
                           "expected an expression after 'case'");
            }
            ACCEPTV_FREE(":");
            Ast* caseBody = _ParseStatement(parser);
            if (caseBody == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           parser->Next.Position,
                           "expected a statement for case body");
            }

            Ast* newCase = AstSwitchCase(caseValue,
                                         caseBody,
                                         MergePositions(caseValue->Position, caseBody->Position));

            if (cases == NULL) {
                cases   = newCase;
                current = newCase;
            } else {
                current->Next = newCase;
                current       = newCase;
            }

        } else if (CHECKTV(KEY_DEFAULT)) {
            if (defaultCase != NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           parser->Next.Position,
                           "multiple default cases are not allowed");
            }
            ACCEPTV_FREE(KEY_DEFAULT);
            ACCEPTV_FREE(":");
            defaultCase = _ParseStatement(parser);
            if (defaultCase == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           parser->Next.Position,
                           "expected a statement block for default case body");
            }
        }
    }

    ended = parser->Next.Position;
    ACCEPTV_FREE("}");
    return AstSwitch(node, cases, defaultCase, MergePositions(start, ended));
}

static Ast* _ParseForStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast *    initializerConditionMutator = NULL, *body = NULL;
    ACCEPTV_FREE(KEY_FOR);
    ACCEPTV_FREE("(");
    initializerConditionMutator = _ParseInitializerConditionMutator(parser);
    ACCEPTV_FREE(";");
    if (initializerConditionMutator != NULL) {
        initializerConditionMutator->Next = _ParseExpression(parser);
    }
    ACCEPTV_FREE(";");
    if (initializerConditionMutator != NULL && initializerConditionMutator->Next != NULL) {
        initializerConditionMutator->Next->Next = _ParseExpression(parser);
    }
    ACCEPTV_FREE(")");
    body = _ParseStatement(parser);
    if (body == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a body");
    }
    ended = body->Position;
    return AstFor(initializerConditionMutator, body, MergePositions(start, ended));
}

static Ast* _ParseWhileStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast *    initializerConditionMutator = NULL, *body = NULL;
    ACCEPTV_FREE(KEY_WHILE);
    ACCEPTV_FREE("(");
    initializerConditionMutator = _ParseInitializerConditionMutator(parser);
    if (initializerConditionMutator == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a condition");
    }

    if (CHECKTV(";")) {
        // Condition?
        ACCEPTV_FREE(";");
        initializerConditionMutator->Next = _ParseExpression(parser);
        if (initializerConditionMutator->Next == NULL) {
            ThrowError(parser->Lexer->Path,
                       parser->Lexer->Data,
                       parser->Next.Position,
                       "expected a condition");
        }

        if (CHECKTV(";")) {
            // Mutator?
            ACCEPTV_FREE(";");
            initializerConditionMutator->Next->Next = _ParseExpression(parser);
            if (initializerConditionMutator->Next->Next == NULL) {
                ThrowError(parser->Lexer->Path,
                           parser->Lexer->Data,
                           parser->Next.Position,
                           "expected a mutator");
            }
        }
    }
    ACCEPTV_FREE(")");
    body = _ParseStatement(parser);
    if (body == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a body");
    }
    ended = body->Position;
    return AstWhile(initializerConditionMutator, body, MergePositions(start, ended));
}

static Ast* _ParseDoWhileStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast *    body = NULL, *condition = NULL;
    ACCEPTV_FREE(KEY_DO);
    body = _ParseStatement(parser);
    if (body == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a body");
    }
    ACCEPTV_FREE("while");
    ACCEPTV_FREE("(");
    condition = _ParseExpression(parser);
    if (condition == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a condition");
    }
    ended = parser->Next.Position;
    ACCEPTV_FREE(")");
    return AstDoWhile(condition, body, MergePositions(start, ended));
}

static Ast* _ParseTryCatchStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast *    tryBlock = NULL, *catchBlock = NULL, *errorName = NULL;
    ACCEPTV_FREE(KEY_TRY);
    tryBlock = _ParseStatement(parser);
    if (tryBlock == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a try block");
    }
    if (tryBlock->Type != AST_BLOCK) {
        ThrowError(parser->Lexer->Path,
                   parser->Lexer->Data,
                   tryBlock->Position,
                   "expected a block statement");
    }
    ACCEPTV_FREE(KEY_CATCH);
    ACCEPTV_FREE("(");
    errorName = _ParseTerminal(parser);
    if (errorName == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected an error name");
    }
    if (errorName->Type != AST_NAME) {
        ThrowError(parser->Lexer->Path,
                   parser->Lexer->Data,
                   errorName->Position,
                   "expected an identifier or name");
    }
    ACCEPTV_FREE(")");
    catchBlock = _ParseStatement(parser);
    if (catchBlock == NULL) {
        ThrowError(parser->Lexer->Path, parser->Lexer->Data, start, "expected a catch block");
    }
    if (catchBlock->Type != AST_BLOCK) {
        ThrowError(parser->Lexer->Path,
                   parser->Lexer->Data,
                   tryBlock->Position,
                   "expected a block statement");
    }
    ended = parser->Next.Position;
    return AstTryCatch(tryBlock, errorName, catchBlock, MergePositions(start, ended));
}

static Ast* _ParseBlockStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast*     statements = NULL;
    ACCEPTV_FREE("{");
    statements = _ParseListOfStatements(parser);
    ended      = parser->Next.Position;
    ACCEPTV_FREE("}");
    return AstBlock(statements, MergePositions(start, ended));
}

static Ast* _ParseContinueStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_CONTINUE);
    ended = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstContinue(MergePositions(start, ended));
}

static Ast* _ParseBreakStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_BREAK);
    ended = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstBreak(MergePositions(start, ended));
}

static Ast* _ParseReturnStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    ACCEPTV_FREE(KEY_RETURN);
    Ast* expression = _ParseExpression(parser);
    ended           = parser->Next.Position;
    ACCEPTV_FREE(";");
    return AstReturn(expression, MergePositions(start, ended));
}

static Ast* _ParseExpressionStatement(Parser* parser) {
    Position start = parser->Next.Position, ended = start;
    Ast*     expression = _ParseExpression(parser);
    if (expression == NULL) {
        if (CHECKTV(";")) {
            while (CHECKTV(";")) {
                ended = parser->Next.Position;
                ACCEPTV_FREE(";");
            }

            return AstEmptyStatement(MergePositions(start, ended));
        }
        return NULL;
    }
    while (CHECKTV(";")) {
        ended = parser->Next.Position;
        ACCEPTV_FREE(";");
    }
    return AstExpressionStatement(expression, MergePositions(start, ended));
}

static Ast* _ParseStatement(Parser* parser) {
    if (CHECKTV(KEY_CLASS)) {
        return _ParseClass(parser);
    } else if (CHECKTV(KEY_FN)) {
        return _ParseFunction(parser);
    } else if (CHECKTV(KEY_IMPORT)) {
        return _ParseImportStatement(parser);
    } else if (CHECKTV(KEY_VAR)) {
        return _ParseVarStatement(parser);
    } else if (CHECKTV(KEY_CONST)) {
        return _ParseConstStatement(parser);
    } else if (CHECKTV(KEY_LOCAL)) {
        return _ParseLocalStatement(parser);
    } else if (CHECKTV(KEY_IF)) {
        return _ParseIfStatement(parser);
    } else if (CHECKTV(KEY_SWITCH)) {
        return _ParseSwitchStatement(parser);
    } else if (CHECKTV(KEY_FOR)) {
        return _ParseForStatement(parser);
    } else if (CHECKTV(KEY_WHILE)) {
        return _ParseWhileStatement(parser);
    } else if (CHECKTV(KEY_DO)) {
        return _ParseDoWhileStatement(parser);
    } else if (CHECKTV(KEY_TRY)) {
        return _ParseTryCatchStatement(parser);
    } else if (CHECKTV(KEY_CONTINUE)) {
        return _ParseContinueStatement(parser);
    } else if (CHECKTV(KEY_BREAK)) {
        return _ParseBreakStatement(parser);
    } else if (CHECKTV(KEY_RETURN)) {
        return _ParseReturnStatement(parser);
    } else if (CHECKTV("{")) {
        return _ParseBlockStatement(parser);
    }
    return _ParseExpressionStatement(parser);
}

static Ast* _ParseListOfStatements(Parser* parser) {
    Ast* head = NULL;
    Ast* tail = NULL;

    while (CHECKTT(TK_EOF) == false) {
        Ast* statement = _ParseStatement(parser);
        if (statement == NULL) {
            break;
        }

        if (head == NULL) {
            head = statement;
            tail = statement;
        } else {
            tail->Next = statement;
            tail       = statement;
        }
    }

    return head;
}

static Ast* _ParseProgram(Parser* parser) {
    Ast*   ast = AstProgram(_ParseListOfStatements(parser), parser->Next.Position);
    String eof = parser->Next.Value;
    ACCEPTT(TK_EOF);
    free(eof);
    return ast;
}

Ast* Parse(Parser* parser) {
    parser->Next = NextToken(parser->Lexer);
    return _ParseProgram(parser);
}

void FreeParser(Parser* parser) {
    free(parser);
}