#include "../class.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"
#include "../statemachine.h"


#ifndef CORE_PROMISE_H
#define CORE_PROMISE_H

/**
 * @brief Creates the Promise class
 * 
 * This function initializes the Promise class with its methods and properties.
 * It is called during the loading of the core Promise module.
 * 
 * @param  interpreter The interpreter instance to create the Promise class in
 * @return Value* Pointer to the created Promise class, or NULL on failure
 */
Value* CreatePromiseClass(Interpreter* interpreter);

/**
 * @brief Loads the core Promise module
 * 
 * This function initializes and loads the core Promise module, which provides
 * promise functionalities for the interpreter.
 * 
 * @param  interpreter The interpreter instance to load the Promise module into
 * @return Value* Pointer to the loaded core Promise module, or NULL on failure
 */
Value* LoadCorePromise(Interpreter* interpreter);

#endif