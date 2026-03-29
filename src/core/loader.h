#include "../global.h"
#include "./array.h"
#include "./date.h"
#include "./io.h"
#include "./math.h"
#include "./os.h"
#include "./promise.h"


#ifndef CORE_LOADER_H
#    define CORE_LOADER_H

/**
 * @typedef LoadCoreCallback
 * @brief Function pointer type for core module loading functions
 */
typedef Value* (*LoadCoreCallback)(Interpreter* interpreter);

/**
 * @struct core_module_mapping_struct
 * @brief Mapping of core module names to their loader functions
 */
typedef struct core_module_mapping_struct {
    String           Name;
    LoadCoreCallback Loader;
} CoreMapper;

/**
 * @brief Loads a core module by name
 *
 * This function looks up the core module by its name and invokes
 * the corresponding loader function to initialize and return
 * the module.
 *
 * @param interpreter The interpreter instance
 * @param moduleName  The name of the core module to load
 * @return Value* Pointer to the loaded core module, or NULL if not found
 */
Value* LoadCoreModule(Interpreter* interpreter, String moduleName);

#endif /* CORE_LOADER_H */