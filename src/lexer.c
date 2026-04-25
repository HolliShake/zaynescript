
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
static Token MakeToken(TokenKind type, String value, Position position) {
	Token token;
	token.Type	   = type;
	token.Value	   = value;
	token.Position = position;
	return token;
}

// Helper function to build string from runes
static String RunesToString(Rune* runes, int start, int end) {
	if (start >= end) {
		String empty = Allocate(1);
		empty[0]	 = '\0';
		return empty;
	}

	// Calculate total size needed
	size_t totalSize = 0;
	for (int i = start; i < end; i++) {
		totalSize += utf_size_of_codepoint(runes[i]);
	}

	String result = Allocate(totalSize + 1);
	String ptr	  = result;

	for (int i = start; i < end; i++) {
		unsigned char buffer[5];
		int			  size = utf_encode_char(runes[i], buffer);
		for (int j = 0; j < size; j++) {
			*ptr++ = buffer[j];
		}
	}

	*ptr = '\0';

	return result;
}

// Tokenize identifier or keyword
static Token TokenizeIdentifier(Lexer* lexer) {
	Position pos   = PositionFromLineAndColm(lexer->Line, lexer->Colm);
	int		 start = lexer->Indx;

	while (utf_is_letter_or_digit(CurrentRune(lexer))) {
		Advance(lexer);
	}

	pos.ColmEnded = lexer->Colm;
	String value  = RunesToString(lexer->Data, start, lexer->Indx);

	// Check for keywords
	const String keywords[] = { KEY_IF,		  KEY_ELSE,	 KEY_SWITCH, KEY_CASE,
								KEY_DEFAULT,  KEY_WHILE, KEY_FOR,	 KEY_DO,
								KEY_TRY,	  KEY_CATCH, KEY_RETURN, KEY_BREAK,
								KEY_CONTINUE, KEY_RAISE, KEY_ASSERT, KEY_NULL,
								KEY_TRUE,	  KEY_FALSE, KEY_CLASS,	 KEY_ENUM,
								KEY_IMPORT,	  KEY_FROM,	 KEY_STATIC, KEY_CONST,
								KEY_VAR,	  KEY_LOCAL, KEY_FN,	 KEY_ASYNC,
								KEY_AWAIT,	  KEY_NEW,	 KEY_TYPEOF, KEY_THIS };

	for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
		if (strcmp(value, keywords[i]) == 0) {
			return MakeToken(TK_KEY, value, pos);
		}
	}

	return MakeToken(TK_IDN, value, pos);
}

// Tokenize number (integer or float)
static Token TokenizeNumber(Lexer* lexer) {
	Position pos		= PositionFromLineAndColm(lexer->Line, lexer->Colm);
	int		 start		= lexer->Indx;
	bool	 hasDecimal = false;

	while (utf_is_digit(CurrentRune(lexer)) || CurrentRune(lexer) == '.') {
		if (CurrentRune(lexer) == '.') {
			if (hasDecimal)
				break;	// Second decimal point, stop
			hasDecimal = true;
		}
		Advance(lexer);
	}

	if (CurrentRune(lexer) == 'e' || CurrentRune(lexer) == 'E') {
		hasDecimal = true;	// Scientific notation counts as a float
		Advance(lexer);
		if (CurrentRune(lexer) == '+' || CurrentRune(lexer) == '-') {
			Advance(lexer);	 // Skip exponent sign
		}
		while (utf_is_digit(CurrentRune(lexer))) {
			Advance(lexer);
		}
	}

	TokenKind kind = hasDecimal ? TK_NUM : TK_INT;
	int		  end  = lexer->Indx;

	if (CurrentRune(lexer) == 'n' || CurrentRune(lexer) == 'N') {
		Advance(lexer);							// Skip exponent sign
		kind = hasDecimal ? TK_BNUM : TK_BINT;	// Big numeric or big integer
	}

	pos.ColmEnded = lexer->Colm;
	String value  = RunesToString(lexer->Data, start, end);

	return MakeToken(kind, value, pos);
}

// Append one codepoint as UTF-8 to a growable buffer (avoids a full Rune[]
// scratch buffer for large string literals).
static bool LexerAppendUtf8(String* out, size_t* cap, size_t* off, Rune r) {
	unsigned char buf[5];
	int			  sz = utf_encode_char((int) r, buf);
	if (sz <= 0) {
		return false;
	}
	if (*off + (size_t) sz + 1 > *cap) {
		size_t newCap = (*cap + (size_t) sz + 1) * 2;
		*out		  = Reallocate(*out, newCap);
		*cap		  = newCap;
	}
	memcpy(*out + *off, buf, (size_t) sz);
	*off += (size_t) sz;
	return true;
}

// Tokenize string literal
static Token TokenizeString(Lexer* lexer) {
	Position pos   = PositionFromLineAndColm(lexer->Line, lexer->Colm);
	Rune	 quote = CurrentRune(lexer);
	Advance(lexer);	 // Skip opening quote

	int	 scan	= lexer->Indx;
	bool closed = false;
	while (lexer->Data[scan] != 0) {
		if (lexer->Data[scan] == quote) {
			closed = true;
			break;
		}
		if (lexer->Data[scan] == '\n') {
			break;	// Do not allow newlines
		}
		if (lexer->Data[scan] == '\\' && lexer->Data[scan + 1] != 0) {
			scan += 2;	// Safely skip the backslash and the escaped char
		} else {
			scan++;
		}
	}

	size_t srcSpan = (size_t) (scan - lexer->Indx);
	size_t cap	   = srcSpan * 4 + 64;
	if (cap < 256) {
		cap = 256;
	}
	String utf8	   = Allocate(cap);
	size_t utf8Len = 0;

	while (CurrentRune(lexer) != 0 && CurrentRune(lexer) != quote) {
		if (CurrentRune(lexer) == '\n') {
			break;
		}
		if (CurrentRune(lexer) == '\\') {
			Advance(lexer);	 // Skip escape character '\'

			switch (CurrentRune(lexer)) {
				case 'b':
					if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, '\b')) {
						goto encode_fail;
					}
					break;
				case 'n':
					if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, '\n')) {
						goto encode_fail;
					}
					break;
				case 't':
					if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, '\t')) {
						goto encode_fail;
					}
					break;
				case 'r':
					if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, '\r')) {
						goto encode_fail;
					}
					break;
				case 'e':
					if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, '\033')) {
						goto encode_fail;
					}
					break;	// The ANSI Escape!
				case '\\':
					if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, '\\')) {
						goto encode_fail;
					}
					break;
				case '\'':
					if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, '\'')) {
						goto encode_fail;
					}
					break;
				case '"':
					if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, '"')) {
						goto encode_fail;
					}
					break;
				case 'x':
					{
						Advance(lexer);	 // skip 'x'
						Rune value = 0;

						// Consume exactly 2 hex digits (standard \xHH)
						for (int _hi = 0; _hi < 2 && CurrentRune(lexer) != 0;
							 _hi++) {
							Rune c = CurrentRune(lexer);
							if (c >= '0' && c <= '9')
								value = (value << 4) | (c - '0');
							else if (c >= 'a' && c <= 'f')
								value = (value << 4) | (c - 'a' + 10);
							else if (c >= 'A' && c <= 'F')
								value = (value << 4) | (c - 'A' + 10);
							else
								break;
							Advance(lexer);
						}
						if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, value)) {
							goto encode_fail;
						}
						continue;  // Skip the Advance() at the bottom of the
								   // outer block
					}
				case 'u':
					{
						Advance(lexer);	 // skip 'u'
						Rune value = 0;

						// Consume exactly 4 hex digits (\uXXXX)
						for (int _hi = 0; _hi < 4 && CurrentRune(lexer) != 0;
							 _hi++) {
							Rune c = CurrentRune(lexer);
							if (c >= '0' && c <= '9')
								value = (value << 4) | (c - '0');
							else if (c >= 'a' && c <= 'f')
								value = (value << 4) | (c - 'a' + 10);
							else if (c >= 'A' && c <= 'F')
								value = (value << 4) | (c - 'A' + 10);
							else
								break;
							Advance(lexer);
						}
						if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, value)) {
							goto encode_fail;
						}
						continue;
					}
				case 'U':
					{
						Advance(lexer);	 // skip 'U'
						Rune value = 0;

						// Consume exactly 8 hex digits (\UXXXXXXXX)
						for (int _hi = 0; _hi < 8 && CurrentRune(lexer) != 0;
							 _hi++) {
							Rune c = CurrentRune(lexer);
							if (c >= '0' && c <= '9')
								value = (value << 4) | (c - '0');
							else if (c >= 'a' && c <= 'f')
								value = (value << 4) | (c - 'a' + 10);
							else if (c >= 'A' && c <= 'F')
								value = (value << 4) | (c - 'A' + 10);
							else
								break;
							Advance(lexer);
						}
						if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, value)) {
							goto encode_fail;
						}
						continue;
					}
				default:
					// Keep unknown escape content without the backslash
					if (CurrentRune(lexer) != 0) {
						if (!LexerAppendUtf8(&utf8,
											 &cap,
											 &utf8Len,
											 CurrentRune(lexer))) {
							goto encode_fail;
						}
					}
					break;
			}
			if (CurrentRune(lexer) != 0) {
				Advance(lexer);	 // Skip the resolved escape character
			}
		} else {
			if (!LexerAppendUtf8(&utf8, &cap, &utf8Len, CurrentRune(lexer))) {
				goto encode_fail;
			}
			Advance(lexer);
		}
	}

	utf8[utf8Len] = '\0';
	if (utf8Len + 1 < cap) {
		utf8 = Reallocate(utf8, utf8Len + 1);
	}

	String value = utf8;

	if (closed && CurrentRune(lexer) == quote) {
		Advance(lexer);	 // Skip closing quote
		pos.LineEnded = lexer->Line;
		pos.ColmEnded = lexer->Colm;
		return MakeToken(TK_STR, value, pos);
	}
	free(value);
	pos.LineEnded = lexer->Line;
	pos.ColmEnded = lexer->Colm;
	ThrowError(lexer->Path, lexer->Data, pos, "Unclosed string literal");
	return MakeToken(TK_STR, AllocateString("Unclosed string literal"), pos);

encode_fail:
	free(utf8);
	pos.LineEnded = lexer->Line;
	pos.ColmEnded = lexer->Colm;
	ThrowError(lexer->Path, lexer->Data, pos, "Invalid string literal");
	return MakeToken(TK_STR, AllocateString("Invalid string literal"), pos);
}

// Tokenize symbol
static Token TokenizeSymbol(Lexer* lexer) {
	Position pos   = PositionFromLineAndColm(lexer->Line, lexer->Colm);
	int		 start = lexer->Indx;

	Rune current = CurrentRune(lexer);
	Advance(lexer);

	// Check for multi-character symbols
	Rune next = CurrentRune(lexer);

	// Check for three-character operators first (e.g., <<=, >>=,
	// ...)
	if ((current == '<' && next == '<' && PeekRune(lexer, 1) == '=')
		|| (current == '>' && next == '>' && PeekRune(lexer, 1) == '=')
		|| (current == '.' && next == '.' && PeekRune(lexer, 1) == '.')) {
		Advance(lexer);
		Advance(lexer);
	}
	// Check for two-character operators
	else if ((current == '=' && next == '=') || (current == '=' && next == '>')
			 || (current == ':' && next == '=')
			 || (current == '!' && next == '=')
			 || (current == '<' && next == '=')
			 || (current == '>' && next == '=')
			 || (current == '<' && next == '<')
			 || (current == '>' && next == '>')
			 || (current == '&' && next == '&')
			 || (current == '|' && next == '|')
			 || (current == '+' && next == '+')
			 || (current == '-' && next == '-')
			 || (current == '+' && next == '=')
			 || (current == '-' && next == '=')
			 || (current == '*' && next == '=')
			 || (current == '/' && next == '=')
			 || (current == '%' && next == '=')
			 || (current == '&' && next == '=')
			 || (current == '|' && next == '=')
			 || (current == '^' && next == '=')
			 || (current == '-' && next == '>')) {
		Advance(lexer);
	}

	pos.ColmEnded = lexer->Colm;
	String value  = RunesToString(lexer->Data, start, lexer->Indx);

	return MakeToken(TK_SYM, value, pos);
}

Lexer* CreateLexer(String path, Rune* data) {
	Lexer* lexer = Allocate(sizeof(Lexer));
	lexer->Path	 = path;
	lexer->Data	 = data;
	lexer->Line	 = 1;
	lexer->Colm	 = 1;
	lexer->Indx	 = 0;
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
			Advance(lexer);	 // Skip '/'
			Advance(lexer);	 // Skip '*'
			while (!(CurrentRune(lexer) == '*' && PeekRune(lexer, 1) == '/')
				   && CurrentRune(lexer) != 0) {
				Advance(lexer);
			}
			if (CurrentRune(lexer) == '*') {
				Advance(lexer);	 // Skip '*'
				Advance(lexer);	 // Skip '/'
			}
			continue;
		}

		// No more comments or whitespace
		break;
	}

	Position pos	 = PositionFromLineAndColm(lexer->Line, lexer->Colm);
	Rune	 current = CurrentRune(lexer);

	// End of file
	if (current == 0) {
		String empty = Allocate(1);
		empty[0]	 = '\0';
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
