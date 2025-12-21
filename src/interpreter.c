#include "./interpreter.h"

Interpreter* CreateInterpreter() {
    Interpreter* interpreter    = Allocate(sizeof(Interpreter));
    interpreter->Allocated      = 0;
    interpreter->GcRoot         = NULL;
    interpreter->True           = NewBoolValue(interpreter, 1);
    interpreter->False          = NewBoolValue(interpreter, 0);
    interpreter->Null           = NewNullValue(interpreter);
    interpreter->Constants      = Allocate(sizeof(Value*));
    interpreter->ConstantC      = 0;
    interpreter->Constants[0]   = NULL;
    return interpreter;
}

void Interpret(Interpreter* interpreter) {
   
}