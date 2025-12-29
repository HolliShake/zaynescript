#include "./global.h"
#include "./hashmap.h"


#ifndef SCOPE_H
#define SCOPE_H

/**
 * @brief Creates a new symbol
 * 
 * @param name The name of the symbol
 * @param isGlobal Whether the symbol is global
 * @param isLocalToFn Whether the symbol is local to a function
 * @param isConstant Whether the symbol is constant
 * @param offset The offset of the symbol in the function's local variables
 * @return Pointer to the newly created Symbol structure
 */
Symbol* CreateSymbol(String name, bool isGlobal, bool isLocalToFn, bool isConstant, int offset);

/**
 * @brief Creates a new scope
 * 
 * @param type The type of scope
 * @param parent The parent scope
 * @return Pointer to the newly created Scope structure
 */
Scope* CreateScope(ScopeType type, Scope* parent);

/**
 * @brief Adds a continue jump offset to the nearest loop scope
 * 
 * @param scope Pointer to the current scope
 * @param offset The offset of the continue jump
 */
void ScopeAddContinueJump(Scope* scope, int offset);

/**
 * @brief Adds a break jump offset to the nearest loop scope
 * 
 * @param scope Pointer to the current scope
 * @param offset The offset of the break jump
 */
void ScopeAddBreakJump(Scope* scope, int offset);

/**
 * @brief Checks if a scope is of a given type
 * 
 * @param scope Pointer to the scope to check
 * @param type The type of scope to check for
 * @return true if the scope is of the given type, false otherwise
 */
bool ScopeIs(Scope* scope, ScopeType type);

/**
 * @brief Checks if a scope is inside a given type
 * 
 * @param scope Pointer to the scope to check
 * @param type The type of scope to check for
 * @return true if the scope is inside the given type, false otherwise
 */
bool ScopeInside(Scope* scope, ScopeType type);

/**
 * @brief Checks if a scope has a local variable with a given name
 * 
 * @param scope Pointer to the scope to check
 * @param name The name of the variable to check for
 * @return true if the scope has a local variable with the given name, false otherwise
 */
bool ScopeHasLocal(Scope* scope, String name);

/**
 * @brief Checks if a scope has a variable with a given name
 * 
 * @param scope Pointer to the scope to check
 * @param name The name of the variable to check for
 * @return true if the scope has a variable with the given name, false otherwise
 */
bool ScopeHasName(Scope* scope, String name);

/**
 * @brief Checks if a scope is local to the global scope
 * 
 * @param scope Pointer to the scope to check
 * @param name The name of the variable to check for
 * @return true if the scope is local to the global scope, false otherwise
 */
bool ScopeIsLocalToGlobal(Scope *scope, String name);

/**
 * @brief Checks if a scope is local to a function
 * 
 * @param scope Pointer to the scope to check
 * @param name The name of the variable to check for
 * @return true if the scope is local to a function, false otherwise
 */
bool ScopeIsLocalToFn(Scope* scope, String name);

/**
 * @brief Sets a symbol in a scope
 * 
 * @param scope Pointer to the scope to set the symbol in
 * @param name The name of the symbol
 * @param isGlobal Whether the symbol is global
 * @param isLocalToFn Whether the symbol is local to a function
 * @param isConstant Whether the symbol is constant
 * @param offset The offset of the symbol in the function's local variables
 */
void ScopeSetSymbol(Scope* scope, String name, bool isGlobal, bool isLocalToFn, bool isConstant, int offset);

/**
 * @brief Gets a symbol from a scope
 * 
 * @param scope Pointer to the scope to get the symbol from
 * @param name The name of the symbol
 * @param recurse Whether to recurse up the scope chain
 * @return Pointer to the symbol if found, NULL otherwise
 */
Symbol* ScopeGetSymbol(Scope* scope, String name, bool recurse);

/**
 * @brief Gets the first scope of a given type
 * 
 * @param scope Pointer to the scope to get the first scope of the given type from
 * @param type The type of scope to get the first scope of
 * @return Pointer to the first scope of the given type, NULL otherwise
 */
Scope* ScopeGetFirst(Scope* scope, ScopeType type);

/**
 * @brief Frees all memory associated with a scope
 * 
 * @param scope Pointer to the scope to free
 */
void FreeScope(Scope* scope);

#endif