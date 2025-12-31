#include "./global.h"

#ifndef FUNCTION_H
#define FUNCTION_H

/**
 * @brief Creates a new user function
 * 
 * @param name The name of the function
 * @param argc The number of arguments the function takes
 * @return Pointer to the newly created UserFunction structure
 */
UserFunction* CreateUserFunction(String name, int argc);

/**
 * @brief Clones an existing user function
 * 
 * @param userFunction Pointer to the user function to clone
 * @return Pointer to the cloned UserFunction structure
 */
UserFunction* UserFunctionClone(UserFunction* userFunction);

/**
 * @brief Emits a local variable to the user function
 * 
 * @param userFunction Pointer to the user function to emit the local variable to
 * @return The offset of the local variable
 */
int UserFunctionEmitLocal(UserFunction* userFunction);

/**
 * @brief Adds a captured variable to the user function
 * 
 * @param userFunction Pointer to the user function to add the captured variable to
 * @param isGlobal Whether the captured variable is global
 * @param src The source offset of the captured variable
 * @param dst The destination offset of the captured variable
 * @return The offset of the captured variable
 */
int UserFunctionAddCapture(UserFunction* userFunction, bool isGlobal, int src, int dst);

/**
 * @brief Creates a new native function metadata structure
 * 
 * @param name The name of the native function
 * @param argc The number of arguments the native function takes
 * @param funcPtr Pointer to the native function implementation
 * @return Pointer to the newly created NativeFunctionMeta structure
 */
NativeFunctionMeta* CreateNativeFunctionMeta(const String name, int argc, NativeFunction funcPtr);

#endif