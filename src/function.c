#include "./function.h"

UserFunction* CreateUserFunction(String name, int argc) {
    UserFunction* userFunction = Allocate(sizeof(UserFunction));
    userFunction->ParentEnv    = NULL;
    userFunction->Name         = name;
    userFunction->Codes        = Allocate(sizeof(uint8_t) * 1);
    userFunction->Codes[0]     = 255;
    userFunction->CodeC        = 0;
    userFunction->Argc         = argc;
    userFunction->LocalC       = 0;
    return userFunction;
}


int UserFunctionEmitLocal(UserFunction* userFunction) {
    return userFunction->LocalC++;
}