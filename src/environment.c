#include "./environment.h"

EnvCell* CreateEnvCell(Value* value) {
    EnvCell* envCell    = Allocate(sizeof(EnvCell));
    envCell->Value      = value;
    envCell->IsCaptured = false;
    envCell->RefCount   = 1;
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

Environment* EnvironmentCloneFromValue(Value* envValue) {
    Environment* environment = CoerceToEnvironment(envValue);
    Environment* newEnv = Allocate(sizeof(Environment));
    newEnv->Parent      = environment->Parent;
    newEnv->Locals      = Callocate(environment->LocalC, sizeof(EnvCell));
    newEnv->LocalC      = environment->LocalC;

    // Initialize
    for (int i = 0;i < environment->LocalC;i++) {
        newEnv->Locals[i] = CreateEnvCell(environment->Locals[i]->Value);
    }

    return newEnv;
}

void EnvironmentSync(Environment* src, Environment* dst) {
    if (dst->LocalC != src->LocalC) {
        Panic("EnvironmentSync: Local variable count mismatch (%d vs %d)\n", dst->LocalC, src->LocalC);
    }

    for (int i = 0; i < src->LocalC; i++) {
        dst->Locals[i]->Value = src->Locals[i]->Value;
    }
}

void FreeEnvironment(Environment* environment) {
    environment->Parent = NULL;
    // Uninitialize
    for (int i = 0; i < environment->LocalC; i++) {
        if (environment->Locals[i] != NULL) {
            environment->Locals[i]->Value = NULL;
            if (--environment->Locals[i]->RefCount <= 0) {
                free(environment->Locals[i]);
            }
        }
    }
    free(environment->Locals);
    free(environment);
}