#include "./function.h"

UserFunction* CreateUserFunction(String name, int argc) {
    UserFunction* userFunction = Allocate(sizeof(UserFunction));
    userFunction->Name         = name;
    userFunction->Codes        = Allocate(sizeof(uint8_t) * 1);
    userFunction->CodeC        = 0;
    userFunction->Argc         = argc;
    return userFunction;
}