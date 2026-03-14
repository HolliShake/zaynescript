#include "./lexer.h"

// Helper function to get current rune
static Rune CurrentRune(Lexer* lexer) {
    return lexer->Data[lexer->Indx];
}

// Helper function to peek ahead
static Rune PeekRune(Lexer* lexer, int offset) {
    return lexer->Data[lexer->Indx + offset];
}

// Helper function to advance lexer
static void Advance(Lexer* lexer) {
    Rune current = CurrentRune(lexer);
    if (current == '\n') {
        lexer->Line++;
        lexer->Colm = 1;  // Reset column to 1 on newline
        lexer->Indx++;
    } else if (current != 0) {
        lexer->Colm++;
        lexer->Indx++;
    }
}

// Helper function to skip whitespace
static void SkipWhitespace(Lexer* lexer) {
    while (utf_is_white_space(CurrentRune(lexer))) {
        Advance(lexer);
    }
}

// Helper function to create a token
static Token MakeToken(TokenKind type, char* value, Position position) {
    Token token;
    token.Type = type;
    token.Value = value;
    token.Position = position;
    return token;
}

// Helper function to build string from runes
static char* RunesToString(Rune* runes, int start, int end) {
    if (start >= end) {
        char* empty = Allocate(1);
        empty[0] = '\0';
        return empty;
    }
    
    // Calculate total size needed
    size_t totalSize = 0;
    for (int i = start; i < end; i++) {
        totalSize += utf_size_of_codepoint(runes[i]);
    }
    
    char* result = Allocate(totalSize + 1);
    char* ptr = result;
    
    for (int i = start; i < end; i++) {
        unsigned char buffer[5];
        int size = utf_encode_char(runes[i], buffer);
        for (int j = 0; j < size; j++) {
            *ptr++ = buffer[j];
        }
    }

    *ptr = '\0';
    
    return result;
}

// Tokenize identifier or keyword
static Token TokenizeIdentifier(Lexer* lexer) {
    Position pos = PositionFromLineAndColm(lexer->Line, lexer->Colm);
    int start = lexer->Indx;
    
    while (utf_is_letter_or_digit(CurrentRune(lexer))) {
        Advance(lexer);
    }
    
    pos.ColmEnded = lexer->Colm;
    char* value = RunesToString(lexer->Data, start, lexer->Indx);
    
    // Check for keywords
    const char* keywords[] = {
        KEY_IF,
        KEY_ELSE,
        KEY_SWITCH,
        KEY_CASE,
        KEY_DEFAULT,
        KEY_WHILE,
        KEY_FOR,
        KEY_DO,
        KEY_TRY,
        KEY_CATCH,
        KEY_RETURN,
        KEY_BREAK,
        KEY_CONTINUE,
        KEY_NULL,
        KEY_TRUE,   
        KEY_FALSE,
        KEY_CLASS,
        KEY_ENUM,
        KEY_IMPORT,
        KEY_FROM,
        KEY_STATIC,
        KEY_CONST,
        KEY_VAR,
        KEY_LOCAL,
        KEY_FN,
        KEY_ASYNC,
        KEY_AWAIT,
        KEY_NEW,
        KEY_THIS
    };
    
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strcmp(value, keywords[i]) == 0) {
            return MakeToken(TK_KEY, value, pos);
        }
    }
    
    return MakeToken(TK_IDN, value, pos);
}

// Tokenize number (integer or float)
static Token TokenizeNumber(Lexer* lexer) {
    Position pos = PositionFromLineAndColm(lexer->Line, lexer->Colm);
    int start = lexer->Indx;
    bool hasDecimal = false;
    
    while (utf_is_digit(CurrentRune(lexer)) || CurrentRune(lexer) == '.') {
        if (CurrentRune(lexer) == '.') {
            if (hasDecimal) break; // Second decimal point, stop
            hasDecimal = true;
        }
        Advance(lexer);
    }
    
    pos.ColmEnded = lexer->Colm;
    char* value = RunesToString(lexer->Data, start, lexer->Indx);
    
    return MakeToken(hasDecimal ? TK_NUM : TK_INT, value, pos);
}

// Tokenize string literal
static Token TokenizeString(Lexer* lexer) {
    Position pos = PositionFromLineAndColm(lexer->Line, lexer->Colm);
    Rune quote = CurrentRune(lexer);
    Advance(lexer); // Skip opening quote

    int maxLength = 0;
    int scan = lexer->Indx;
    while (lexer->Data[scan] != 0 && lexer->Data[scan] != quote) {
        maxLength++;
        scan++;
    }

    Rune* decoded = Allocate(sizeof(Rune) * (maxLength + 1));
    int decodedLength = 0;

    while (CurrentRune(lexer) != 0 && CurrentRune(lexer) != quote) {
        if (CurrentRune(lexer) == '\\') {
            Advance(lexer); // Skip escape character
            switch (CurrentRune(lexer)) {
                case 'n': decoded[decodedLength++] = '\n'; break;
                case 't': decoded[decodedLength++] = '\t'; break;
                case 'r': decoded[decodedLength++] = '\r'; break;
                case '\\': decoded[decodedLength++] = '\\'; break;
                case '\'': decoded[decodedLength++] = '\''; break;
                case '"': decoded[decodedLength++] = '"'; break;
                default:
                    // Keep unknown escape content without the backslash.
                    if (CurrentRune(lexer) != 0) {
                        decoded[decodedLength++] = CurrentRune(lexer);
                    }
                    break;
            }
            if (CurrentRune(lexer) != 0) {
                Advance(lexer); // Skip escaped character
            }
        } else {
            decoded[decodedLength++] = CurrentRune(lexer);
            Advance(lexer);
        }
    }

    decoded[decodedLength] = 0;
    char* value = RunesToString(decoded, 0, decodedLength);
    free(decoded);
    
    if (CurrentRune(lexer) == quote) {
        Advance(lexer); // Skip closing quote
    }
    
    pos.LineEnded = lexer->Line;
    pos.ColmEnded = lexer->Colm;
    
    return MakeToken(TK_STR, value, pos);
}

// Tokenize symbol
static Token TokenizeSymbol(Lexer* lexer) {
    Position pos = PositionFromLineAndColm(lexer->Line, lexer->Colm);
    int start = lexer->Indx;
    
    Rune current = CurrentRune(lexer);
    Advance(lexer);
    
    // Check for multi-character symbols
    Rune next = CurrentRune(lexer);
    
    // Check for three-character operators first (e.g., <<=, >>=, ...)
    if ((current == '<' && next == '<' && PeekRune(lexer, 1) == '=') ||
        (current == '>' && next == '>' && PeekRune(lexer, 1) == '=') ||
        (current == '.' && next == '.' && PeekRune(lexer, 1) == '.')) {
        Advance(lexer);
        Advance(lexer);
    }
    // Check for two-character operators
    else if ((current == '=' && next == '=') ||
             (current == ':' && next == '=') ||
             (current == '!' && next == '=') ||
             (current == '<' && next == '=') ||
             (current == '>' && next == '=') ||
             (current == '<' && next == '<') ||
             (current == '>' && next == '>') ||
             (current == '&' && next == '&') ||
             (current == '|' && next == '|') ||
             (current == '+' && next == '+') ||
             (current == '-' && next == '-') ||
             (current == '+' && next == '=') ||
             (current == '-' && next == '=') ||
             (current == '*' && next == '=') ||
             (current == '/' && next == '=') ||
             (current == '%' && next == '=') ||
             (current == '&' && next == '=') ||
             (current == '|' && next == '=') ||
             (current == '^' && next == '=') ||
             (current == '-' && next == '>')) {
        Advance(lexer);
    }
    
    pos.ColmEnded = lexer->Colm;
    char* value = RunesToString(lexer->Data, start, lexer->Indx);
    
    return MakeToken(TK_SYM, value, pos);
}

Lexer* CreateLexer(String path, Rune* data) {
    Lexer* lexer = Allocate(sizeof(Lexer));
    lexer->Path = path;
    lexer->Data = data;
    lexer->Line = 1;
    lexer->Colm = 1;
    lexer->Indx = 0;
    return lexer;
}

Token NextToken(Lexer* lexer) {
    // Skip whitespace and comments
    while (true) {
        SkipWhitespace(lexer);
        
        // Check for single-line comments starting with //
        if (CurrentRune(lexer) == '/' && PeekRune(lexer, 1) == '/') {
            while (CurrentRune(lexer) != '\n' && CurrentRune(lexer) != 0) {
                Advance(lexer);
            }
            continue;
        }
        
        // Check for multi-line comments
        if (CurrentRune(lexer) == '/' && PeekRune(lexer, 1) == '*') {
            Advance(lexer); // Skip '/'
            Advance(lexer); // Skip '*'
            while (!(CurrentRune(lexer) == '*' && PeekRune(lexer, 1) == '/') && 
                   CurrentRune(lexer) != 0) {
                Advance(lexer);
            }
            if (CurrentRune(lexer) == '*') {
                Advance(lexer); // Skip '*'
                Advance(lexer); // Skip '/'
            }
            continue;
        }
        
        // No more comments or whitespace
        break;
    }
    
    Position pos = PositionFromLineAndColm(lexer->Line, lexer->Colm);
    Rune current = CurrentRune(lexer);
    
    // End of file
    if (current == 0) {
        char* empty = Allocate(1);
        empty[0] = '\0';
        return MakeToken(TK_EOF, empty, pos);
    }
    
    // Identifier or keyword
    if (utf_is_letter(current)) {
        return TokenizeIdentifier(lexer);
    }
    
    // Number
    if (utf_is_digit(current)) {
        return TokenizeNumber(lexer);
    }
    
    // String literal
    if (current == '"' || current == '\'') {
        return TokenizeString(lexer);
    }
    
    // Symbol
    return TokenizeSymbol(lexer);
}

void FreeLexer(Lexer* lexer) {
    free(lexer);
}