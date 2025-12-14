#include "./parser.h"

Parser* CreateParser(Tokenizer* tokenizer) {
    Parser* parser = Allocate(sizeof(Parser));
    parser->Tokenizer = tokenizer;
    return parser;
}

#define CHECKTV(v) _CheckTokenV(parser, v)

#define CHECKTT(t) _CheckTokenT(parser, t)

#define ACCEPTV(v) _AcceptTokenV(parser, v)

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
        parser->Next = NextToken(parser->Tokenizer);
        return;
    }
    // Throw error
    fprintf(stderr, "Expected token value %s, got %s\n", value, parser->Next.Value);
    exit(EXIT_FAILURE);
}

static void _AcceptTokenT(Parser* parser, TokenType type) {
    if (_CheckTokenT(parser, type)) {
        parser->Next = NextToken(parser->Tokenizer);
        return;
    }
    // Throw error
    fprintf(stderr, "Expected token type %d, got %d\n", type, parser->Next.Type);
    exit(EXIT_FAILURE);
}

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
                parser->Tokenizer->Path, 
                parser->Tokenizer->Data, 
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
    }

    return lhs;
}

static Ast* _ListOfStatements(Parser* parser) {
    Ast* head = NULL;
    Ast* tail = NULL;
    
    Ast* statement = _Multiplicative(parser);
    if (statement != NULL) {
        head = statement;
        tail = statement;
        
        while (CHECKTT(TK_EOF) == false) {
            statement = _Multiplicative(parser);
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
    parser->Next = NextToken(parser->Tokenizer);
    return _ParseProgram(parser);
}

void FreeParser(Parser* parser) {
    free(parser);
}
