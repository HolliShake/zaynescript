#include "src/global.h"
#include "src/lexer.h"
#include "src/parser.h"
#include "src/interpreter.h"


String ReadFile(String path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", path);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Could not allocate memory for file '%s'\n", path);
        fclose(file);
        return NULL;
    }
    
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    
    fclose(file);
    return buffer;
}

// Simple example using the allocator system
int main() {
    printf("=== LanguageX - Allocator Demo ===\n\n");

    String path = "./test.lang";
    Rune* data = StringToRunes(ReadFile(path));
    Lexer* lexer = CreateLexer("test.txt", data);
    Parser* parser = CreateParser(lexer);
    Interpreter* interpreter = CreateInterpreter();
    Interpret(interpreter, parser);

    FreeLexer(lexer);
    FreeParser(parser);
    printf("Done!\n");
    return 0;
}