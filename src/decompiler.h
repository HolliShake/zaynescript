/**
 * @file decompiler.h
 * @brief Decompiler interface for converting bytecode back into source text.
 *
 * Provides a single entry point for disassembling a compiled UserFunction
 * back into a human-readable source-code-like representation. Primarily
 * used for debugging and introspection of compiled functions.
 */

#include "./global.h"

#ifndef DECOMPILER_H
#	define DECOMPILER_H

/**
 * @brief Decompiles a compiled user function back into readable source text.
 *
 * Walks the bytecode of the given UserFunction and reconstructs a
 * human-readable representation of its instructions and operands.
 *
 * @param interpreter Pointer to the interpreter instance (used to resolve
 * constants and nested functions).
 * @param uf Pointer to the UserFunction whose bytecode will be decompiled.
 * @return Newly allocated string containing the decompiled source text.
 * The caller is responsible for freeing this string.
 */
String DecompileFunction(Interpreter* interpreter, UserFunction* uf);

#endif