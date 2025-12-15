#include "./interpreter.h"

Interpreter* CreateInterpreter() {
    Interpreter* interpreter = Allocate(sizeof(Interpreter));
    interpreter->True  = Allocate(sizeof(Value));
    interpreter->False = Allocate(sizeof(Value));
    interpreter->Null  = Allocate(sizeof(Value));
    return interpreter;
}

void Interpret(Interpreter* interpreter, Parser* parser) {
    Ast* program = Parse(parser);
}