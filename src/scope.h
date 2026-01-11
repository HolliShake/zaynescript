#include "./global.h"
#include "./hashmap.h"


#ifndef SCOPE_H
#define SCOPE_H

/*
 * scope.h - Scope management for variable tracking and symbol resolution
 * 
 * This module provides functionality for managing lexical scopes during compilation.
 * It handles symbol tables, variable captures for closures, and scope hierarchy
 * traversal. Scopes track local variables, captured variables, and control flow
 * jump points for loops.
 */

/*
 * CreateSymbol - Creates a new symbol
 * 
 * Allocates and initializes a Symbol structure with the given properties.
 * Symbols represent variables and their metadata within a scope.
 * 
 * Parameters:
 *   name        - The name of the symbol (string identifier)
 *   isGlobal    - Whether the symbol is global (true) or local (false)
 *   isLocalToFn - Whether the symbol is local to a function scope
 *   isConstant  - Whether the symbol is constant (immutable)
 *   offset      - The offset of the symbol in the function's local variables array
 * 
 * Returns:
 *   Pointer to the newly created Symbol structure, or NULL on allocation failure
 */
Symbol* CreateSymbol(String name, bool isGlobal, bool isLocalToFn, bool isConstant, int offset);

/*
 * CreateScope - Creates a new scope
 * 
 * Allocates and initializes a Scope structure with the given type and parent.
 * Scopes form a tree structure representing the lexical nesting of code blocks.
 * 
 * Parameters:
 *   type   - The type of scope (e.g., ST_GLOBAL, ST_FUNCTION, ST_BLOCK, ST_LOOP)
 *   parent - The parent scope in the scope hierarchy, or NULL for the global scope
 * 
 * Returns:
 *   Pointer to the newly created Scope structure, or NULL on allocation failure
 */
Scope* CreateScope(ScopeType type, Scope* parent);

/*
 * ScopeAddContinueJump - Adds a continue jump offset to the nearest loop scope
 * 
 * Registers a bytecode offset that needs to be patched with the continue target
 * address. This is used during compilation to handle continue statements in loops.
 * The function traverses up the scope chain to find the nearest loop scope.
 * 
 * Parameters:
 *   scope  - Pointer to the current scope
 *   offset - The bytecode offset of the continue jump instruction to be patched
 */
void ScopeAddContinueJump(Scope* scope, int offset);

/*
 * ScopeAddBreakJump - Adds a break jump offset to the nearest loop scope
 * 
 * Registers a bytecode offset that needs to be patched with the break target
 * address. This is used during compilation to handle break statements in loops.
 * The function traverses up the scope chain to find the nearest loop scope.
 * 
 * Parameters:
 *   scope  - Pointer to the current scope
 *   offset - The bytecode offset of the break jump instruction to be patched
 */
void ScopeAddBreakJump(Scope* scope, int offset);

/*
 * ScopeIs - Checks if a scope is of a given type
 * 
 * Tests whether the provided scope exactly matches the specified type.
 * Does not check parent scopes.
 * 
 * Parameters:
 *   scope - Pointer to the scope to check
 *   type  - The type of scope to check for (e.g., ST_FUNCTION, ST_LOOP)
 * 
 * Returns:
 *   true if the scope is of the given type, false otherwise
 */
bool ScopeIs(Scope* scope, ScopeType type);

/*
 * ScopeInside - Checks if a scope is inside a given type
 * 
 * Traverses up the scope chain to determine if any ancestor scope
 * (including the current scope) matches the specified type.
 * 
 * Parameters:
 *   scope - Pointer to the scope to check
 *   type  - The type of scope to check for
 * 
 * Returns:
 *   true if the scope or any ancestor is of the given type, false otherwise
 */
bool ScopeInside(Scope* scope, ScopeType type);

/*
 * ScopeHasLocal - Checks if a scope has a local variable with a given name
 * 
 * Searches only the current scope's symbol table for a variable with the
 * specified name. Does not search parent scopes.
 * 
 * Parameters:
 *   scope - Pointer to the scope to check
 *   name  - The name of the variable to check for
 * 
 * Returns:
 *   true if the scope has a local variable with the given name, false otherwise
 */
bool ScopeHasLocal(Scope* scope, String name);

/*
 * ScopeHasName - Checks if a scope has a variable with a given name
 * 
 * Searches the current scope and all parent scopes for a variable with the
 * specified name. This performs a full scope chain lookup.
 * 
 * Parameters:
 *   scope - Pointer to the scope to check
 *   name  - The name of the variable to check for
 * 
 * Returns:
 *   true if the scope or any ancestor has a variable with the given name, false otherwise
 */
bool ScopeHasName(Scope* scope, String name);

/*
 * ScopeIsLocalToGlobal - Checks if a variable is local to the global scope
 * 
 * Determines whether a variable with the given name is defined in the global scope.
 * This is used to distinguish between global and local variable access.
 * 
 * Parameters:
 *   scope - Pointer to the scope to check
 *   name  - The name of the variable to check for
 * 
 * Returns:
 *   true if the variable is local to the global scope, false otherwise
 */
bool ScopeIsLocalToGlobal(Scope *scope, String name);

/*
 * ScopeIsLocalToFn - Checks if a variable is local to a function
 * 
 * Determines whether a variable with the given name is defined in the current
 * function scope (not captured from an outer function).
 * 
 * Parameters:
 *   scope - Pointer to the scope to check
 *   name  - The name of the variable to check for
 * 
 * Returns:
 *   true if the variable is local to the current function, false otherwise
 */
bool ScopeIsLocalToFn(Scope* scope, String name);

/*
 * ScopeIsLocalToFnClosure - Checks if a variable is local to a function closure
 * 
 * Determines whether a variable with the given name is captured from an outer
 * function scope, making it part of a closure.
 * 
 * Parameters:
 *   scope - Pointer to the scope to check
 *   name  - The name of the variable to check for
 * 
 * Returns:
 *   true if the variable is captured in a closure, false otherwise
 */
bool ScopeIsLocalToFnClosure(Scope* scope, String name);

/*
 * ScopeSetSymbol - Sets a symbol in a scope
 * 
 * Adds or updates a symbol in the scope's symbol table. This is used during
 * compilation to register variable declarations.
 * 
 * Parameters:
 *   scope       - Pointer to the scope to set the symbol in
 *   name        - The name of the symbol
 *   isGlobal    - Whether the symbol is global
 *   isLocalToFn - Whether the symbol is local to a function
 *   isConstant  - Whether the symbol is constant (immutable)
 *   offset      - The offset of the symbol in the function's local variables array
 */
void ScopeSetSymbol(Scope* scope, String name, bool isGlobal, bool isLocalToFn, bool isConstant, int offset);

/*
 * ScopeGetSymbol - Gets a symbol from a scope
 * 
 * Retrieves a symbol from the scope's symbol table. Can optionally search
 * parent scopes if the symbol is not found in the current scope.
 * 
 * Parameters:
 *   scope   - Pointer to the scope to get the symbol from
 *   name    - The name of the symbol
 *   recurse - Whether to recurse up the scope chain if not found locally
 * 
 * Returns:
 *   Pointer to the symbol if found, NULL otherwise
 */
Symbol* ScopeGetSymbol(Scope* scope, String name, bool recurse);

/*
 * ScopeHasCapture - Checks if a scope has a captured variable with a given name
 * 
 * Searches the scope's capture table for a variable with the specified name.
 * Captured variables are those from outer scopes that are used in closures.
 * 
 * Parameters:
 *   scope - Pointer to the scope to check
 *   name  - The name of the variable to check for
 * 
 * Returns:
 *   true if the scope has a captured variable with the given name, false otherwise
 */
bool ScopeHasCapture(Scope* scope, String name);

/*
 * ScopeSetCapture - Sets a captured variable in a scope
 * 
 * Adds or updates a captured variable in the scope's capture table. This is used
 * during compilation to track variables that need to be captured for closures.
 * 
 * Parameters:
 *   scope       - Pointer to the scope to set the captured variable in
 *   name        - The name of the captured variable
 *   isGlobal    - Whether the captured variable is global
 *   isLocalToFn - Whether the captured variable is local to a function
 *   isConstant  - Whether the captured variable is constant
 *   offset      - The offset of the captured variable in the function's local variables array
 */
void ScopeSetCapture(Scope* scope, String name, bool isGlobal, bool isLocalToFn, bool isConstant, int offset);

/*
 * ScopeGetCapture - Gets a captured variable from a scope
 * 
 * Retrieves a captured variable from the scope's capture table. Can optionally
 * search parent scopes if the capture is not found in the current scope.
 * 
 * Parameters:
 *   scope   - Pointer to the scope to get the captured variable from
 *   name    - The name of the captured variable
 *   recurse - Whether to recurse up the scope chain if not found locally
 * 
 * Returns:
 *   Pointer to the captured variable symbol if found, NULL otherwise
 */
Symbol* ScopeGetCapture(Scope* scope, String name, bool recurse);

/*
 * ScopeGetFirst - Gets the first scope of a given type
 * 
 * Traverses up the scope chain to find the first (nearest) scope that matches
 * the specified type. This is useful for finding enclosing function or loop scopes.
 * 
 * Parameters:
 *   scope - Pointer to the scope to start searching from
 *   type  - The type of scope to search for
 * 
 * Returns:
 *   Pointer to the first scope of the given type, NULL if not found
 */
Scope* ScopeGetFirst(Scope* scope, ScopeType type);

/*
 * ScopeCountNested - Counts the number of nested scopes of a given type
 * 
 * Traverses up the scope chain and counts how many scopes of the specified
 * type are encountered. This is useful for determining nesting depth.
 * 
 * Parameters:
 *   scope - Pointer to the scope to start counting from
 *   type  - The type of scope to count
 * 
 * Returns:
 *   Number of nested scopes of the given type (0 if none found)
 */
int ScopeCountNested(Scope* scope, ScopeType type);

/*
 * FreeScope - Frees all memory associated with a scope
 * 
 * Recursively frees a scope and all its child scopes, including their symbol
 * tables and capture tables. This should be called when compilation is complete.
 * 
 * Parameters:
 *   scope - Pointer to the scope to free (can be NULL)
 */
void FreeScope(Scope* scope);

#endif