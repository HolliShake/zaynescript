#include "./environment.h"

EnvCell* CreateEnvCell(Value* value) {
    EnvCell* envCell    = Allocate(sizeof(EnvCell));
    envCell->Value      = value;
    envCell->IsCaptured = false;
    return envCell;
}

Environment* CreateEnvironment(Value* parent, int localC) {
    Environment* environment = Allocate(sizeof(Environment));
    environment->Parent      = parent;
    environment->Locals      = Callocate(localC, sizeof(EnvCell));
    environment->LocalC      = localC;

    // Initialize
    for (int i = 0;i < localC;i++) {
        environment->Locals[i] = CreateEnvCell(NULL);
    }

    return environment;
}

void EnvironmentSetLocal(Environment* environment, int offset, Value* value) {
    environment->Locals[offset]->Value = value;
}

EnvCell* EnvironmentGetLocal(Environment* environment, int offset) {
    return environment->Locals[offset];
}

void FreeEnvironment(Environment* environment) {
    environment->Parent = NULL;
    // Uninitialize
    for (int i = 0; i < environment->LocalC; i++) {
        //NOTE: memory leak (EnvCells that are captured are not freed. Since Environment is only freed when unreachable (and thus capturing UserFunctions are also unreachable), these cells should be freed)
        if (environment->Locals[i] != NULL && !(environment->Locals[i]->IsCaptured)) {
            environment->Locals[i]->Value = NULL;
            free(environment->Locals[i]);
        }
    }
    free(environment->Locals);
    free(environment);
}