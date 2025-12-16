#include "./interpreter.h"

Interpreter* CreateInterpreter() {
    Interpreter* interpreter = Allocate(sizeof(Interpreter));
    interpreter->Allocated   = 0;
    interpreter->GcRoot      = NULL;
    interpreter->True        = NewBoolValue(interpreter, 1);
    interpreter->False       = NewBoolValue(interpreter, 0);
    interpreter->Null        = NewNullValue(interpreter);
    return interpreter;
}

static Value* _Eval(Interpreter* interpreter, Ast* expr) {

}

static void _Visit(
    Interpreter* interpreter, 
    UserFunction* userFunction, 
    Ast* node
) {
    if (node->Type == AST_EXPRESSION_STATEMENT) {
        _Eval(interpreter, node->A);
    }
}

static void _CallFunction(
    Interpreter* interpreter, 
    UserFunction* userFunction, 
    int argc, 
    Value** arguments
) {
    Ast* fnName     = userFunction->Function->A;
    Ast* parameters = userFunction->Function->B;
    Ast* body       = userFunction->Function->C;

    if (argc != userFunction->Argc) {
        // TODO: Implement proper error message formatting
        // Format detailed error message with expected vs actual argument counts
        char message[256];
        snprintf(message, sizeof(message), 
                "function '%s' expects %d argument(s), but %d were provided",
                fnName && fnName->Value ? fnName->Value : "<anonymous>",
                userFunction->Argc,
                argc);
        ThrowError(
            userFunction->Path,
            userFunction->Data,
            userFunction->Function->Position,
            (String) message
        );
    } else {
        for (int i = 0; i < argc; i++) {
            arguments[i] = arguments[i];
        }
    }

    while (body != NULL) {
        _Visit(interpreter, userFunction, body);
        body = body->Next;
    }
}

void Interpret(Interpreter* interpreter, Parser* parser) {
    Ast* program = Parse(parser);
    UserFunction* userFunction = NewUserFunction(
        parser->Lexer->Path,
        parser->Lexer->Data,
        program, 0 // 0 arguments
    );
    _CallFunction(interpreter, userFunction, 0, NULL);
}