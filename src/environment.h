#include "./global.h"

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

/**
 * @brief Creates a new environment
 * 
 * @param parent The parent environment
 * @param localC The number of local variables in the environment
 * @return Pointer to the newly created Environment structure
 */
Environment* CreateEnvironment(Environment* parent, int localC);


/**
 * @brief Sets a local variable in the environment
 * 
 * @param environment The environment to set the local variable in
 * @param offset The offset of the local variable
 * @param value The value to set the local variable to
 */
void EnvironmentSetLocal(Environment* environment, int offset, Value* value);


/**
 * @brief Gets a local variable from the environment
 * 
 * @param environment The environment to get the local variable from
 * @param offset The offset of the local variable
 * @return The value of the local variable
 */
Value* EnvironmentGetLocal(Environment* environment, int offset);

#endif