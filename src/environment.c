#include "./environment.h"


EnvCell* CreateEnvCell(Value* value) {
    EnvCell* envCell = Allocate(sizeof(EnvCell));
    envCell->Value   = value;
    return envCell;
}

Environment* CreateEnvironment(int localC) {
    Environment* environment = Allocate(sizeof(Environment));
    environment->Locals      = Callocate(localC, sizeof(EnvCell));
    environment->LocalC      = localC;
    return environment;
}

void EnvironmentSetLocal(Environment* environment, int offset, Value* value) {
    if (environment->Locals[offset] == NULL) {
        environment->Locals[offset] = CreateEnvCell(NULL);
    }
    environment->Locals[offset]->Value = value;
}

EnvCell* EnvironmentGetLocal(Environment* environment, int offset) {
    if (environment->Locals[offset] == NULL) {
        return NULL;
    }
    return environment->Locals[offset];
}
