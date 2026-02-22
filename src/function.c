#include "./function.h"

UserFunction* CreateUserFunction(String name, int argc) {
    UserFunction* userFunction  = Allocate(sizeof(UserFunction));
    userFunction->Scope         = NULL;
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
    clone->Scope       = userFunction->Scope;
    clone->CodeC       = userFunction->CodeC;
    clone->Codes       = Reallocate(
        clone->Codes,
        sizeof(uint8_t) * (userFunction->CodeC + 1)
    );
    memcpy(clone->Codes, userFunction->Codes, sizeof(uint8_t) * (userFunction->CodeC + 1));
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

int UserFunctionAddCapture(UserFunction* userFunction, int depth, int sourceOffset) {
    int offset = userFunction->CaptureC;
    CaptureMeta capture;
    capture.Depth    = depth;
    capture.Src      = sourceOffset;
    capture.Dst      = offset; // The destination offset is determined by the current capture count
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

String UserFunctionToString(UserFunction* userFunction) {
    const char* name = userFunction->Name != NULL ? userFunction->Name : "<anonymous>";
    
    if (userFunction->Argc == -1) {
        // Variadic function
        size_t nameLen = strlen(name);
        size_t bufferSize = nameLen + 32; // "function " + name + "(...$n) {...}" + null
        char* buffer = Allocate(bufferSize);
        snprintf(buffer, bufferSize, "function %s(...$n) {...}", name);
        return buffer;
    } else {
        // Fixed argument count
        // Calculate required buffer size for arguments
        // Each arg is "$N" where N can be multiple digits, plus ", " separator
        size_t argsSize = 0;
        for (int i = 0; i < userFunction->Argc; i++) {
            // Calculate digits in (i + 1)
            int num = i + 1;
            int digits = 1;
            while (num >= 10) {
                digits++;
                num /= 10;
            }
            argsSize += 1 + digits; // "$" + digits
            if (i < userFunction->Argc - 1) {
                argsSize += 2; // ", "
            }
        }
        
        // Build arguments string
        char* args = Allocate(argsSize + 1);
        args[0] = '\0';
        size_t offset = 0;
        for (int i = 0; i < userFunction->Argc; i++) {
            int written = snprintf(args + offset, argsSize + 1 - offset, "$%d", i + 1);
            offset += written;
            if (i < userFunction->Argc - 1) {
                strcpy(args + offset, ", ");
                offset += 2;
            }
        }
        
        // Build final string
        size_t nameLen = strlen(name);
        size_t bufferSize = nameLen + argsSize + 32; // "function " + name + "(" + args + ") {...}" + null
        char* buffer = Allocate(bufferSize);
        snprintf(buffer, bufferSize, "function %s(%s) {...}", name, args);
        free(args);
        return buffer;
    }
}

void FreeUserFunction(UserFunction* userFunction) {
    // for (int i = 0; i < userFunction->CaptureC; i++) {
    //     EnvCell* capture = userFunction->Captures[i];
    //     if (capture != NULL && (--capture->RefCount) <= 0) {
    //         free(capture);
    //         userFunction->Captures[i] = NULL;
    //     }
    // }
    if (userFunction->Name != NULL) free(userFunction->Name);
    free(userFunction->CaptureMetas);
    free(userFunction->Captures);
    free(userFunction->Codes);
    free(userFunction);
}

NativeFunction* CreateNativeFunctionMeta(const String name, int argc, NativeFunctionCallback funcPtr) {
    NativeFunction* meta = Allocate(sizeof(NativeFunction));
    meta->Name    = name;
    meta->Argc    = argc;
    meta->FuncPtr = funcPtr;
    return meta;
}

String NativeFunctionMetaToString(NativeFunction* meta) {
    // fmt: native function %name ($1, $2) {} if argc != -1 else native function %name (...$n) {}
    
    if (meta->Argc == -1) {
        // Variadic function
        size_t nameLen = strlen(meta->Name);
        size_t bufferSize = nameLen + 32; // "native function " + name + "(...$n) {...}" + null
        char* buffer = Allocate(bufferSize);
        snprintf(buffer, bufferSize, "native function %s(...$n) {...}", meta->Name);
        return buffer;
    } else {
        // Fixed argument count
        // Calculate required buffer size for arguments
        // Each arg is "$N" where N can be multiple digits, plus ", " separator
        size_t argsSize = 0;
        for (int i = 0; i < meta->Argc; i++) {
            // Calculate digits in (i + 1)
            int num = i + 1;
            int digits = 1;
            while (num >= 10) {
                digits++;
                num /= 10;
            }
            argsSize += 1 + digits; // "$" + digits
            if (i < meta->Argc - 1) {
                argsSize += 2; // ", "
            }
        }
        
        // Build arguments string
        char* args = Allocate(argsSize + 1);
        args[0] = '\0';
        size_t offset = 0;
        for (int i = 0; i < meta->Argc; i++) {
            int written = snprintf(args + offset, argsSize + 1 - offset, "$%d", i + 1);
            offset += written;
            if (i < meta->Argc - 1) {
                strcpy(args + offset, ", ");
                offset += 2;
            }
        }
        
        // Build final string
        size_t nameLen = strlen(meta->Name);
        size_t bufferSize = nameLen + argsSize + 32; // "native function " + name + "(" + args + ") {...}" + null
        char* buffer = Allocate(bufferSize);
        snprintf(buffer, bufferSize, "native function %s(%s) {...}", meta->Name, args);
        free(args);
        return buffer;
    }
}

void FreeNativeFunction(NativeFunction* nativeFunction) {
    if (nativeFunction->Name != NULL) free(nativeFunction->Name);
    free(nativeFunction);
}