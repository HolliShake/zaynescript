#include "./global.h"

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H


/*
 * CreateEnvCell - Creates a new environment cell
 * 
 * value: The value to store in the environment cell
 * 
 * Returns: Pointer to the newly created EnvCell structure
 */
EnvCell* CreateEnvCell(Value* value);

/*
 * CreateEnvironment - Creates a new environment
 * 
 * parent: The parent value
 * localC: The number of local variables in the environment
 * 
 * Returns: Pointer to the newly created Environment structure
 */
Environment* CreateEnvironment(Value* parent, int localC);


/*
 * EnvironmentSetLocal - Sets a local variable in the environment
 * 
 * environment: The environment to set the local variable in
 * offset: The offset of the local variable
 * value: The value to set the local variable to
 * 
 * Returns: void
 */
void EnvironmentSetLocal(Environment* environment, int offset, Value* value);


/*
 * EnvironmentGetLocal - Gets a local variable from the environment
 * 
 * environment: The environment to get the local variable from
 * offset: The offset of the local variable
 * 
 * Returns: The environment cell containing the value of the local variable
 */
EnvCell* EnvironmentGetLocal(Environment* environment, int offset);

#endif