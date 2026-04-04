/**
 * @file import.h
 * @brief Module import graph and dependency tracking interface.
 *
 * Provides structures and functions for managing module imports,
 * tracking dependencies between modules, and detecting circular
 * import chains.
 */

#include "./global.h"

#ifndef IMPORT_H
#	define IMPORT_H


/**
 * @brief Creates a new import AST node.
 *
 * @param interpreter Pointer to the interpreter instance, used
 * to access the import graph and manage memory.
 * @param module The module path to import.
 * @return Pointer to the newly created ImportNode structure, or
 * NULL on failure.
 */
ImportNode* CreateOrGetImportNode(Interpreter* interpreter, String module);

/**
 * @brief Adds a dependency from one import node to another.
 *
 * Links the source import node as a dependency of the
 * destination import node, indicating that the destination
 * module relies on the source module. This is used for tracking
 * module dependencies and detecting circular imports.
 *
 * @param dst Pointer to the destination ImportNode that depends
 * on the source.
 * @param src Pointer to the source ImportNode that the
 * destination depends on.
 */
void ImportNodeAddDependency(ImportNode* dst, ImportNode* src);

/**
 * @brief Checks if an import node has a circular dependency.
 *
 * Traverses the dependency graph starting from the given import
 * node to determine if there is a cycle that leads back to
 * itself. This is used to prevent infinite loops and stack
 * overflows caused by circular imports.
 *
 * @param node Pointer to the ImportNode to check for circular
 * dependencies.
 * @param modulePath The module path associated with the import
 * node (for error reporting).
 * @return true if a circular dependency is detected, false
 * otherwise.
 */
bool ImportNodeHasCircularDependency(ImportNode* node, String modulePath);

/**
 * @brief Checks if one import node depends on another.
 *
 * Determines if the destination import node has a direct or
 * indirect dependency on the source import node by traversing
 * the dependency graph.
 *
 * @param node Pointer to the destination ImportNode to check.
 * @param dependency Pointer to the source ImportNode that may be
 * a dependency.
 * @return true if the destination node depends on the source
 * node, false otherwise.
 */
bool ImportNodeHasDependency(ImportNode* node, ImportNode* dependency);

/**
 * @brief Frees all memory associated with an import node and its
 * dependencies.
 *
 * Recursively releases the memory for the given import node,
 * including its path string, dependencies array, and the node
 * structure itself. This should be called when an imported
 * module is no longer needed to clean up resources.
 *
 * @param node Pointer to the ImportNode to free.
 */
void FreeImportNode(ImportNode* node);

#endif