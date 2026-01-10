#include "../global.h"
#include "../value.h"

#ifndef MATH_H
#define MATH_H

/**
 * @brief Loads the core Math module
 * 
 * @param interpeter The interpreter instance
 * @return Pointer to the loaded core Math module
 */
Value* LoadCoreMath(Interpreter* interpeter);

#endif

