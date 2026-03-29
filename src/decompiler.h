#include "./global.h"

#ifndef DECOMPILER_H
#    define DECOMPILER_H

/**
 * @brief Decompiles a user function back into source code
 *
 * @param interpreter Pointer to the interpreter instance
 * @param uf Pointer to the user function to decompile
 * @return String containing the decompiled source code
 */
String DecompileFunction(Interpreter* interpreter, UserFunction* uf);

#endif