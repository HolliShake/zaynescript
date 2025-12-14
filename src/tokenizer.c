#include "./tokenizer.h"

// Helper function to get current rune
static Rune CurrentRune(Tokenizer* tokenizer) {
    return tokenizer->Data[tokenizer->Indx];
}

// Helper function to peek ahead
static Rune PeekRune(Tokenizer* tokenizer, int offset) {
    return tokenizer->Data[tokenizer->Indx + offset];
}

// Helper function to advance tokenizer
static void Advance(Tokenizer* tokenizer) {
    Rune current = CurrentRune(tokenizer);
    if (current == '\n') {
        tokenizer->Line++;
        tokenizer->Colm = 1;  // Reset column to 1 on newline
        tokenizer->Indx++;
    } else if (current != 0) {
        tokenizer->Colm++;
        tokenizer->Indx++;
    }
}

// Helper function to skip whitespace
static void SkipWhitespace(Tokenizer* tokenizer) {
    while (utf_is_white_space(CurrentRune(tokenizer))) {
        Advance(tokenizer);
    }
}

// Helper function to create a token
static Token MakeToken(TokenType type, char* value, Position position) {
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
static Token TokenizeIdentifier(Tokenizer* tokenizer) {
    Position pos = PositionFromLineAndColm(tokenizer->Line, tokenizer->Colm);
    int start = tokenizer->Indx;
    
    while (utf_is_letter_or_digit(CurrentRune(tokenizer))) {
        Advance(tokenizer);
    }
    
    pos.colmEnded = tokenizer->Colm;
    char* value = RunesToString(tokenizer->Data, start, tokenizer->Indx);
    
    // Check for keywords
    const char* keywords[] = {
        KEY_IF, KEY_ELSE, KEY_WHILE, KEY_FOR, KEY_RETURN, KEY_BREAK, KEY_CONTINUE,
        KEY_NULL, KEY_TRUE, KEY_FALSE,
        KEY_CLASS, KEY_STRUCT, KEY_ENUM, KEY_CONST, KEY_VAR, KEY_LET, KEY_FN,
        KEY_ASYNC, KEY_AWAIT, KEY_NEW
    };
    
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strcmp(value, keywords[i]) == 0) {
            return MakeToken(TK_KEY, value, pos);
        }
    }
    
    return MakeToken(TK_IDN, value, pos);
}

// Tokenize number (integer or float)
static Token TokenizeNumber(Tokenizer* tokenizer) {
    Position pos = PositionFromLineAndColm(tokenizer->Line, tokenizer->Colm);
    int start = tokenizer->Indx;
    bool hasDecimal = false;
    
    while (utf_is_digit(CurrentRune(tokenizer)) || CurrentRune(tokenizer) == '.') {
        if (CurrentRune(tokenizer) == '.') {
            if (hasDecimal) break; // Second decimal point, stop
            hasDecimal = true;
        }
        Advance(tokenizer);
    }
    
    pos.colmEnded = tokenizer->Colm;
    char* value = RunesToString(tokenizer->Data, start, tokenizer->Indx);
    
    return MakeToken(hasDecimal ? TK_NUM : TK_INT, value, pos);
}

// Tokenize string literal
static Token TokenizeString(Tokenizer* tokenizer) {
    Position pos = PositionFromLineAndColm(tokenizer->Line, tokenizer->Colm);
    Rune quote = CurrentRune(tokenizer);
    Advance(tokenizer); // Skip opening quote
    
    int start = tokenizer->Indx;
    
    while (CurrentRune(tokenizer) != 0 && CurrentRune(tokenizer) != quote) {
        if (CurrentRune(tokenizer) == '\\') {
            Advance(tokenizer); // Skip escape character
            if (CurrentRune(tokenizer) != 0) {
                Advance(tokenizer); // Skip escaped character
            }
        } else {
            Advance(tokenizer);
        }
    }
    
    char* value = RunesToString(tokenizer->Data, start, tokenizer->Indx);
    
    if (CurrentRune(tokenizer) == quote) {
        Advance(tokenizer); // Skip closing quote
    }
    
    pos.lineEnded = tokenizer->Line;
    pos.colmEnded = tokenizer->Colm;
    
    return MakeToken(TK_STR, value, pos);
}

// Tokenize symbol
static Token TokenizeSymbol(Tokenizer* tokenizer) {
    Position pos = PositionFromLineAndColm(tokenizer->Line, tokenizer->Colm);
    int start = tokenizer->Indx;
    
    Rune current = CurrentRune(tokenizer);
    Advance(tokenizer);
    
    // Check for multi-character symbols
    Rune next = CurrentRune(tokenizer);
    
    // Check for three-character operators first (e.g., <<=, >>=, ...)
    if ((current == '<' && next == '<' && PeekRune(tokenizer, 1) == '=') ||
        (current == '>' && next == '>' && PeekRune(tokenizer, 1) == '=') ||
        (current == '.' && next == '.' && PeekRune(tokenizer, 1) == '.')) {
        Advance(tokenizer);
        Advance(tokenizer);
    }
    // Check for two-character operators
    else if ((current == '=' && next == '=') ||
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
        Advance(tokenizer);
    }
    
    pos.colmEnded = tokenizer->Colm;
    char* value = RunesToString(tokenizer->Data, start, tokenizer->Indx);
    
    return MakeToken(TK_SYM, value, pos);
}

Tokenizer* CreateTokenizer(String path, Rune* data) {
    Tokenizer* tokenizer = Allocate(sizeof(Tokenizer));
    tokenizer->Path = path;
    tokenizer->Data = data;
    tokenizer->Line = 1;
    tokenizer->Colm = 1;
    tokenizer->Indx = 0;
    return tokenizer;
}

Token NextToken(Tokenizer* tokenizer) {
    // Skip whitespace and comments
    SkipWhitespace(tokenizer);
    
    // Check for single-line comments
    if (CurrentRune(tokenizer) == '/' && PeekRune(tokenizer, 1) == '/') {
        while (CurrentRune(tokenizer) != '\n' && CurrentRune(tokenizer) != 0) {
            Advance(tokenizer);
        }
        SkipWhitespace(tokenizer);
    }
    
    // Check for multi-line comments
    if (CurrentRune(tokenizer) == '/' && PeekRune(tokenizer, 1) == '*') {
        Advance(tokenizer); // Skip '/'
        Advance(tokenizer); // Skip '*'
        while (!(CurrentRune(tokenizer) == '*' && PeekRune(tokenizer, 1) == '/') && 
               CurrentRune(tokenizer) != 0) {
            Advance(tokenizer);
        }
        if (CurrentRune(tokenizer) == '*') {
            Advance(tokenizer); // Skip '*'
            Advance(tokenizer); // Skip '/'
        }
        SkipWhitespace(tokenizer);
    }
    
    Position pos = PositionFromLineAndColm(tokenizer->Line, tokenizer->Colm);
    Rune current = CurrentRune(tokenizer);
    
    // End of file
    if (current == 0) {
        char* empty = Allocate(1);
        empty[0] = '\0';
        return MakeToken(TK_EOF, empty, pos);
    }
    
    // Identifier or keyword
    if (utf_is_letter(current)) {
        return TokenizeIdentifier(tokenizer);
    }
    
    // Number
    if (utf_is_digit(current)) {
        return TokenizeNumber(tokenizer);
    }
    
    // String literal
    if (current == '"' || current == '\'') {
        return TokenizeString(tokenizer);
    }
    
    // Symbol
    return TokenizeSymbol(tokenizer);
}

void FreeTokenizer(Tokenizer* tokenizer) {
    free(tokenizer);
}