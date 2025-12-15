#include "./global.h"

#ifndef FUNCTION_H
#define FUNCTION_H

/**
 * @brief Creates a new user function
 * 
 * @param path The path to the file containing the function
 * @param data The data of the function
 * @param function The function AST
 * @param argc The number of arguments the function takes
 * @return The new user function
 */
UserFunction* NewUserFunction(String path, Rune* data, Ast* function, int argc);

#endif