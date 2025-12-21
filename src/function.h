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

#endif