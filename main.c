#include "src/global.h"
#include "src/tokenizer.h"
#include "src/parser.h"

// Simple example using the allocator system
int main() {
    printf("=== LanguageX - Allocator Demo ===\n\n");

    Rune* data = StringToRunes("\n\n\n5 *\n\n\n\n");
    Tokenizer* tokenizer = CreateTokenizer("test.txt", data);
    Parser* parser = CreateParser(tokenizer);
    Ast* ast = Parse(parser);

    FreeTokenizer(tokenizer);
    FreeParser(parser);
    printf("Done!\n");
    return 0;
}