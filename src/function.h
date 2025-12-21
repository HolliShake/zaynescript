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
 * @brief Emits a local variable to the user function
 * 
 * @param userFunction Pointer to the user function to emit the local variable to
 * @return The offset of the local variable
 */
int UserFunctionEmitLocal(UserFunction* userFunction);

#endif