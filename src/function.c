#include "./function.h"
#include "global.h"

UserFunction* CreateUserFunction(String name, int argc) {
    UserFunction* userFunction  = Allocate(sizeof(UserFunction));
    userFunction->ParentEnv     = NULL;
    userFunction->Name          = name;
    userFunction->Codes         = Allocate(sizeof(uint8_t) * 1);
    userFunction->Codes[0]      = 255;
    userFunction->CodeC         = 0;
    userFunction->Argc          = argc;
    userFunction->LocalC        = 0;
    userFunction->CaptureMetas  = Allocate(sizeof(CaptureMeta) * 1);
    userFunction->CaptureC      = 0;
    userFunction->Captures      = Allocate(sizeof(EnvCell*) * 1);
    return userFunction;
}

UserFunction* UserFunctionClone(UserFunction* userFunction) {
    UserFunction* clone = CreateUserFunction(
        userFunction->Name != NULL ? AllocateString(userFunction->Name) : NULL,
        userFunction->Argc
    );
    clone->ParentEnv   = userFunction->ParentEnv;
    clone->CodeC       = userFunction->CodeC;
    clone->Codes       = Reallocate(
        clone->Codes,
        sizeof(uint8_t) * (userFunction->CodeC + 1)
    );
    memcpy(clone->Codes, userFunction->Codes, sizeof(uint8_t) * userFunction->CodeC);
    clone->LocalC       = userFunction->LocalC;
    clone->CaptureC     = userFunction->CaptureC;
    clone->CaptureMetas = Reallocate(
        clone->CaptureMetas,
        sizeof(CaptureMeta) * (userFunction->CaptureC + 1)
    );
    memcpy(
        clone->CaptureMetas,
        userFunction->CaptureMetas,
        sizeof(CaptureMeta) * userFunction->CaptureC
    );
    clone->Captures    = Reallocate(
        clone->Captures,
        sizeof(EnvCell*) * (userFunction->CaptureC + 1)
    );
    memcpy(
        clone->Captures,
        userFunction->Captures,
        sizeof(EnvCell*) * userFunction->CaptureC
    );
    return clone;
}

int UserFunctionEmitLocal(UserFunction* userFunction) {
    return userFunction->LocalC++;
}

int UserFunctionAddCapture(UserFunction* userFunction, bool isGlobal, int src, int dst) {
    int offset = userFunction->CaptureC;
    CaptureMeta capture;
    capture.IsGlobal = isGlobal;
    capture.Src      = src;
    capture.Dst      = dst;
    userFunction->CaptureMetas[userFunction->CaptureC++] = capture;
    userFunction->CaptureMetas = Reallocate(
        userFunction->CaptureMetas, 
        sizeof(CaptureMeta) * (userFunction->CaptureC + 1)
    );
    userFunction->Captures[offset] = NULL;
    userFunction->Captures = Reallocate(
        userFunction->Captures,
        sizeof(EnvCell*) * (userFunction->CaptureC + 1)
    );
    return offset;
}