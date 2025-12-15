#include "./function.h"


UserFunction* NewUserFunction(String path, Rune* data, Ast* function, int argc) {
    UserFunction* userFunction = Allocate(sizeof(UserFunction));
    userFunction->Path     = path;
    userFunction->Data     = data;
    userFunction->Function = function;
    userFunction->Argc     = argc;
    return userFunction;
}