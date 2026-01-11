#include "./global.h"

#ifndef DECOMPILER_H
#define DECOMPILER_H

/*
 * DecompileFunction - Decompiles a user function back into source code
 *
 * interpreter: Pointer to the interpreter instance
 * uf: Pointer to the user function to decompile
 *
 * Returns: String containing the decompiled source code
 */
String DecompileFunction(Interpreter* interpreter, UserFunction* uf);

#endif