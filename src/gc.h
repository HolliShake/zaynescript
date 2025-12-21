#include "./global.h"

#ifndef GC_H
#define GC_H

/**
 * @brief Performs garbage collection on the interpreter
 * 
 * @param interpreter Pointer to the interpreter instance
 */
void GarbageCollect(Interpreter* interpreter);


#endif