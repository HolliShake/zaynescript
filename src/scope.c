#include "./scope.h"

Symbol* CreateSymbol(String name, bool isGlobal, bool isLocalToFn, int offset) {
    Symbol* symbol       = Allocate(sizeof(Symbol));
    symbol->IsGlobal     = isGlobal;
    symbol->IsLocalToFn  = isLocalToFn;
    symbol->Offset       = offset;
    return symbol;
}

Scope* CreateScope(ScopeType type, Scope* parent) {
    Scope* scope    = Allocate(sizeof(Scope));
    scope->Type     = type;
    scope->Parent   = parent;
    scope->Symbols  = CreateHashMap(16);
    scope->Captures = CreateHashMap(16);
    scope->Returned = false;
    return scope;
}

bool ScopeIs(Scope* scope, ScopeType type) {
    return scope->Type == type;
}

bool ScopeInside(Scope* scope, ScopeType type) {
    while (scope != NULL) {
        if (scope->Type == type) {
            return true;
        }
        scope = scope->Parent;
    }
    return false;
}

bool ScopeHasLocal(Scope* scope, String name) {
    if (!HashMapContains(scope->Symbols, name)) {
        return false;
    }
    Symbol* symbol = (Symbol*) HashMapGet(scope->Symbols, name);
    return symbol != NULL && symbol->IsLocalToFn;
}

bool ScopeHasName(Scope* scope, String name) {
    if (HashMapContains(scope->Symbols, name)) {
        return true;
    }
    
    if (scope->Parent != NULL) {
        return ScopeHasName(scope->Parent, name);
    }
    
    return false;
}

bool ScopeIsLocalToFn(Scope* scope, String name) {
    Scope* current = scope;
    while (current != NULL) {
        if (HashMapContains(current->Symbols, name)) {
            // Found the symbol, now check if we're inside a function scope
            Scope* check = current;
            while (check != NULL) {
                if (check->Type == SCOPE_FUNCTION) {
                    return true;
                }
                check = check->Parent;
            }
            return false;
        }
        if (current->Type == SCOPE_FUNCTION) {
            // We've reached a function boundary without finding the symbol
            return false;
        }
        current = current->Parent;
    }
    return false;
}

void ScopeSetSymbol(Scope* scope, String name, bool isGlobal, bool isLocalToFn, int offset) {
    String key = AllocateString(name);
    Symbol* symbol = CreateSymbol(key, isGlobal, isLocalToFn, offset);
    HashMapSet(scope->Symbols, key, symbol);
}

Symbol* ScopeGetSymbol(Scope* scope, String name, bool recurse) {
    Symbol* symbol = (Symbol*) HashMapGet(scope->Symbols, name);
    if (symbol != NULL) {
        return symbol;
    }
    
    if (recurse && scope->Parent != NULL) {
        return ScopeGetSymbol(scope->Parent, name, recurse);
    }
    
    return NULL;
}

Scope* ScopeGetFirst(Scope* scope, ScopeType type) {
    while (scope != NULL) {
        if (scope->Type == type) {
            return scope;
        }
        scope = scope->Parent;
    }
    return NULL;
}

void FreeScope(Scope* scope) {
    if (scope == NULL) {
        return;
    }
    free(scope);
}