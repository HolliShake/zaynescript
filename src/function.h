#include "./global.h"

#ifndef FUNCTION_H
#define FUNCTION_H

/**
 * Creates a new user function.
 *
 * @param name The name of the function.
 * @param argc The number of arguments the function takes.
 * @return Pointer to the newly created UserFunction structure, or NULL on failure.
 */
UserFunction* CreateUserFunction(String name, int argc);

/**
 * Clones an existing user function.
 *
 * @param userFunction Pointer to the user function to clone.
 * @return Pointer to the cloned UserFunction structure, or NULL on failure.
 */
UserFunction* UserFunctionClone(UserFunction* userFunction);

/**
 * Emits a local variable to the user function.
 *
 * @param userFunction Pointer to the user function to emit the local variable to.
 * @return The offset of the local variable.
 */
int UserFunctionEmitLocal(UserFunction* userFunction);

/**
 * Adds a captured variable to the user function.
 *
 * @param userFunction Pointer to the user function to add the captured variable to.
 * @param depth The depth of the captured variable's scope relative to the closure.
 * @param sourceOffset The source offset of the captured variable.
 * @return The offset of the captured variable.
 */
int UserFunctionAddCapture(UserFunction* userFunction, int depth, int sourceOffset);

/**
 * Converts a user function structure to its string representation.
 *
 * @param userFunction Pointer to the UserFunction structure.
 * @return String representation of the user function.
 */
String UserFunctionToString(UserFunction* userFunction);

/**
 * Frees a User Defined function
 * 
 * @param userFunction Pointer to the UserFunction structure 
 */
void FreeUserFunction(UserFunction* userFunction);

/**
 * Creates a new native function metadata structure.
 *
 * @param name The name of the native function.
 * @param argc The number of arguments the native function takes.
 * @param funcPtr Pointer to the native function implementation.
 * @return Pointer to the newly created NativeFunction structure, or NULL on failure.
 */
NativeFunction* CreateNativeFunctionMeta(const String name, int argc, NativeFunctionCallback funcPtr);

/**
 * Converts a native function metadata structure to its string representation.
 *
 * @param meta Pointer to the NativeFunction structure.
 * @return String representation of the native function metadata.
 */
String NativeFunctionMetaToString(NativeFunction* meta);

/**
 * Frees a native function
 * 
 * @param nativeFunction Pointer to the NativeFunction structure 
 */
void FreeNativeFunction(NativeFunction* nativeFunction);

#endif