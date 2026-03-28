#include "./global.h"
#include "./hashmap.h"


#ifndef SCOPE_H
#define SCOPE_H

/**
 * @file scope.h
 * @brief Scope management for variable tracking and symbol resolution
 * 
 * This module provides functionality for managing lexical scopes during compilation.
 * It handles symbol tables, variable captures for closures, and scope hierarchy
 * traversal. Scopes track local variables, captured variables, and control flow
 * jump points for loops.
 */

/**
 * @brief Creates a new symbol
 * 
 * Allocates and initializes a Symbol structure with the given properties.
 * Symbols represent variables and their metadata within a scope.
 * 
 * @param isGlobal Whether the symbol is global (true) or local (false)
 * @param isLocalToFn Whether the symbol is local to a function scope
 * @param isConstant Whether the symbol is constant (immutable)
 * @param offset The offset of the symbol in the function's local variables array
 * @return Pointer to the newly created Symbol structure, or NULL on allocation failure
 */
Symbol* CreateSymbol(bool isGlobal, bool isLocalToFn, bool isConstant, int offset);

/**
 * @brief Creates a new scope
 * 
 * Allocates and initializes a Scope structure with the given type and parent.
 * Scopes form a tree structure representing the lexical nesting of code blocks.
 * 
 * @param type The type of scope (e.g., ST_GLOBAL, ST_FUNCTION, ST_BLOCK, ST_LOOP)
 * @param parent The parent scope in the scope hierarchy, or NULL for the global scope
 * @return Pointer to the newly created Scope structure, or NULL on allocation failure
 */
Scope* CreateScope(ScopeType type, Scope* parent);

/**
 * @brief Adds a continue jump offset to the nearest loop scope
 * 
 * Registers a bytecode offset that needs to be patched with the continue target
 * address. This is used during compilation to handle continue statements in loops.
 * The function traverses up the scope chain to find the nearest loop scope.
 * 
 * @param scope Pointer to the current scope
 * @param offset The bytecode offset of the continue jump instruction to be patched
 */
void ScopeAddContinueJump(Scope* scope, int offset);

/**
 * @brief Adds a break jump offset to the nearest loop scope
 * 
 * Registers a bytecode offset that needs to be patched with the break target
 * address. This is used during compilation to handle break statements in loops.
 * The function traverses up the scope chain to find the nearest loop scope.
 * 
 * @param scope Pointer to the current scope
 * @param offset The bytecode offset of the break jump instruction to be patched
 */
void ScopeAddBreakJump(Scope* scope, int offset);

/**
 * @brief Checks if a scope is of a given type
 * 
 * Tests whether the provided scope exactly matches the specified type.
 * Does not check parent scopes.
 * 
 * @param scope Pointer to the scope to check
 * @param type The type of scope to check for (e.g., ST_FUNCTION, ST_LOOP)
 * @return true if the scope is of the given type, false otherwise
 */
bool ScopeIs(Scope* scope, ScopeType type);

/**
 * @brief Checks if a scope is inside a given type
 * 
 * Traverses up the scope chain to determine if any ancestor scope
 * (including the current scope) matches the specified type.
 * 
 * @param scope Pointer to the scope to check
 * @param type The type of scope to check for
 * @return true if the scope or any ancestor is of the given type, false otherwise
 */
bool ScopeInside(Scope* scope, ScopeType type);

/**
 * @brief Checks if a scope has a local variable with a given name
 * 
 * Searches only the current scope's symbol table for a variable with the
 * specified name. Does not search parent scopes.
 * 
 * @param scope Pointer to the scope to check
 * @param name The name of the variable to check for
 * @return true if the scope has a local variable with the given name, false otherwise
 */
bool ScopeHasLocal(Scope* scope, String name);

/**
 * @brief Checks if a scope has a variable with a given name
 * 
 * Searches the current scope and all parent scopes for a variable with the
 * specified name. This performs a full scope chain lookup.
 * 
 * @param scope Pointer to the scope to check
 * @param name The name of the variable to check for
 * @return true if the scope or any ancestor has a variable with the given name, false otherwise
 */
bool ScopeHasName(Scope* scope, String name);

/**
 * @brief Checks if a variable is local to the global scope
 * 
 * Determines whether a variable with the given name is defined in the global scope.
 * This is used to distinguish between global and local variable access.
 * 
 * @param scope Pointer to the scope to check
 * @param name The name of the variable to check for
 * @return true if the variable is local to the global scope, false otherwise
 */
bool ScopeIsLocalToGlobal(Scope *scope, String name);

/**
 * @brief Checks if a variable is local to a function closure
 * 
 * Determines whether a variable with the given name is captured from an outer
 * function scope, making it part of a closure.
 * 
 * @param scope Pointer to the scope to check
 * @param name The name of the variable to check for
 * @return true if the variable is captured in a closure, false otherwise
 */
bool ScopeIsLocalToFn(Scope* scope, String name);

/**
 * @brief Sets a symbol in a scope
 * 
 * Adds or updates a symbol in the scope's symbol table. This is used during
 * compilation to register variable declarations.
 * 
 * @param scope Pointer to the scope to set the symbol in
 * @param name The name of the symbol
 * @param isGlobal Whether the symbol is global
 * @param isLocalToFn Whether the symbol is local to a function
 * @param isConstant Whether the symbol is constant (immutable)
 * @param offset The offset of the symbol in the function's local variables array
 */
void ScopeSetSymbol(Scope* scope, String name, bool isGlobal, bool isLocalToFn, bool isConstant, int offset);

/**
 * @brief Gets a symbol from a scope
 * 
 * Retrieves a symbol from the scope's symbol table. Can optionally search
 * parent scopes if the symbol is not found in the current scope.
 * 
 * @param scope Pointer to the scope to get the symbol from
 * @param name The name of the symbol
 * @param recurse Whether to recurse up the scope chain if not found locally
 * @return Pointer to the symbol if found, NULL otherwise
 */
Symbol* ScopeGetSymbol(Scope* scope, String name, bool recurse);

/**
 * @brief Gets the depth of a symbol in the scope chain
 * 
 * Traverses up the scope chain to find how many levels deep the specified symbol
 * is located. The depth is counted from the starting scope (depth 0) upwards through
 * parent scopes. This is useful for determining variable shadowing levels or closure
 * capture distances.
 * 
 * @param scope Pointer to the scope to start searching from
 * @param name The name of the symbol to search for
 * @return The depth level where the symbol is found (0 = current scope, 1 = parent, etc.),
 *         or -1 if the symbol is not found in any parent scope
 */
int ScopeGetDepthOfSymbol(Scope* scope, String name);

/**
 * @brief Checks if a scope has a captured variable with a given name
 * 
 * Searches the scope's capture table for a variable with the specified name.
 * Captured variables are those from outer scopes that are used in closures.
 * 
 * @param scope Pointer to the scope to check
 * @param name The name of the variable to check for
 * @return true if the scope has a captured variable with the given name, false otherwise
 */
bool ScopeHasCapture(Scope* scope, String name);

/**
 * @brief Sets a captured variable in a scope
 * 
 * Adds or updates a captured variable in the scope's capture table. This is used
 * during compilation to track variables that need to be captured for closures.
 * 
 * @param scope Pointer to the scope to set the captured variable in
 * @param name The name of the captured variable
 * @param isGlobal Whether the captured variable is global
 * @param isLocalToFn Whether the captured variable is local to a function
 * @param isConstant Whether the captured variable is constant
 * @param offset The offset of the captured variable in the function's local variables array
 */
void ScopeSetCapture(Scope* scope, String name, bool isGlobal, bool isLocalToFn, bool isConstant, int offset);

/**
 * @brief Gets a captured variable from a scope
 * 
 * Retrieves a captured variable from the scope's capture table. Can optionally
 * search parent scopes if the capture is not found in the current scope.
 * 
 * @param scope Pointer to the scope to get the captured variable from
 * @param name The name of the captured variable
 * @param recurse Whether to recurse up the scope chain if not found locally
 * @return Pointer to the captured variable symbol if found, NULL otherwise
 */
Symbol* ScopeGetCapture(Scope* scope, String name, bool recurse);

/**
 * @brief Gets the first scope of a given type
 * 
 * Traverses up the scope chain to find the first (nearest) scope that matches
 * the specified type. This is useful for finding enclosing function or loop scopes.
 * 
 * @param scope Pointer to the scope to start searching from
 * @param type The type of scope to search for
 * @return Pointer to the first scope of the given type, NULL if not found
 */
Scope* ScopeGetFirst(Scope* scope, ScopeType type);

/**
 * @brief Counts the number of nested scopes of a given type
 * 
 * Traverses up the scope chain and counts how many scopes of the specified
 * type are encountered. This is useful for determining nesting depth.
 * 
 * @param scope Pointer to the scope to start counting from
 * @param type The type of scope to count
 * @return Number of nested scopes of the given type (0 if none found)
 */
int ScopeCountNested(Scope* scope, ScopeType type);

/**
 * @brief Frees all memory associated with a scope
 * 
 * Recursively frees a scope and all its child scopes, including their symbol
 * tables and capture tables. This should be called when compilation is complete.
 * 
 * @param scope Pointer to the scope to free (can be NULL)
 */
void FreeScope(Scope* scope);

#endif