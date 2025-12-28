#include "./global.h"
#include "./value.h"

#ifndef IO_H
#define IO_H

/**
 * @brief Loads the core IO module
 * 
 * @param interpeter The interpreter instance
 * @return Pointer to the loaded core IO module
 */
Value* LoadCoreIo(Interpreter* interpeter);

#endif