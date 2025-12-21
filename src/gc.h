#include "./global.h"

#ifndef GC_H
#define GC_H

/**
 * @brief Marks a value for garbage collection
 * 
 * @param value Pointer to the value to mark
 */
void Mark(Value* value);

/**
 * @brief Performs garbage collection on the interpreter
 * 
 * @param interpreter Pointer to the interpreter instance
 */
void GarbageCollect(Interpreter* interpreter);

/**
 * @brief Forces garbage collection on the interpreter
 * 
 * @param interpreter Pointer to the interpreter instance
 */
void ForceGarbageCollect(Interpreter* interpreter);


#endif