#include "./value.h"


Value* CreateValue(Interpreter* interpreter, ValueType type) {
    Value* value = Allocate(sizeof(Value));
    value->Type = type;
    return value;
}