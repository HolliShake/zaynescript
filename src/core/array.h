#include "../class.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_ARRAY_H
#define CORE_ARRAY_H

/**
 * @brief Creates the Array class
 * 
 * This function initializes and returns the Array class, which provides
 * array functionalities to the interpreter.
 * 
 * @param interpreter The interpreter instance to create the class in
 * @return Value* Pointer to the Array class value
 */
Value* CreateArrayClass(Interpreter* interpreter);

#endif