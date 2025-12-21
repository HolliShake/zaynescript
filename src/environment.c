#include "./environment.h"


Environment* CreateEnvironment(Environment* parent, int localC) {
    Environment* environment = Allocate(sizeof(Environment));
    environment->Parent      = parent;
    environment->Locals      = Allocate(sizeof(Value*) * (localC + 1));
    environment->LocalC      = localC;
    // Initialize all locals to NULL
    for (int i = 0; i < localC; i++) {
        environment->Locals[i] = NULL;
    }
    environment->Locals[localC] = NULL;
    return environment;
}

void EnvironmentSetLocal(Environment* environment, int offset, Value* value) {
    environment->Locals[offset] = value;
}

Value* EnvironmentGetLocal(Environment* environment, int offset) {
    return environment->Locals[offset];
}
