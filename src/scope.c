#include "./scope.h"

Symbol* CreateSymbol(bool isGlobal, bool isLocalToFn, bool isConstant, int offset) {
    Symbol* symbol      = Allocate(sizeof(Symbol));
    symbol->IsGlobal    = isGlobal;
    symbol->IsLocalToFn = isLocalToFn;
    symbol->IsConstant  = isConstant;
    symbol->Offset      = offset;
    return symbol;
}

Scope* CreateScope(ScopeType type, Scope* parent) {
    Scope* scope         = Allocate(sizeof(Scope));
    scope->Type          = type;
    scope->Parent        = parent;
    scope->Symbols       = CreateHashMap(16);
    scope->Captures      = CreateHashMap(16);
    scope->Returned      = false;
    scope->ContinueJumps = Allocate(sizeof(int) * 1);
    scope->ContinueJumpC = 0;
    scope->BreakJumps    = Allocate(sizeof(int) * 1);
    scope->BreakJumpC    = 0;
    return scope;
}

void ScopeAddContinueJump(Scope* scope, int offset) {
    Scope* LoopScope = ScopeGetFirst(scope, SCOPE_LOOP);
    if (LoopScope == NULL) {
        printf("LoopNotFound!");
        exit(EXIT_FAILURE);
        return;
    }
    LoopScope->ContinueJumps[LoopScope->ContinueJumpC++] = offset;
    LoopScope->ContinueJumps =
        Reallocate(LoopScope->ContinueJumps, sizeof(int) * (LoopScope->ContinueJumpC + 1));
}

void ScopeAddBreakJump(Scope* scope, int offset) {
    Scope* LoopScope = ScopeGetFirst(scope, SCOPE_LOOP);
    if (LoopScope == NULL) {
        printf("LoopNotFound!");
        exit(EXIT_FAILURE);
        return;
    }
    LoopScope->BreakJumps[LoopScope->BreakJumpC++] = offset;
    LoopScope->BreakJumps =
        Reallocate(LoopScope->BreakJumps, sizeof(int) * (LoopScope->BreakJumpC + 1));
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

bool ScopeIsLocalToGlobal(Scope* scope, String name) {
    Scope* current = scope;
    while (current != NULL) {
        if (HashMapContains(current->Symbols, name)) {
            // Found the symbol, now check if we're inside a function scope
            Scope* check = current;
            while (check != NULL) {
                if (check->Type == SCOPE_GLOBAL) {
                    return true;
                }
                check = check->Parent;
            }
            return false;
        }
        if (current->Type == SCOPE_GLOBAL) {
            // We've reached a function boundary without finding the symbol
            return false;
        }
        current = current->Parent;
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

void ScopeSetSymbol(Scope* scope,
                    String name,
                    bool   isGlobal,
                    bool   isLocalToFn,
                    bool   isConstant,
                    int    offset) {
    // Note: memory leak (if 'name' already exists in scope->Symbols, HashMapSet overwrites the old
    // Symbol* without freeing it)
    Symbol* symbol = CreateSymbol(isGlobal, isLocalToFn, isConstant, offset);
    HashMapSet(scope->Symbols, name, symbol);
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

int ScopeGetDepthOfSymbol(Scope* scope, String name) {
    int depth = 0;
    while (scope != NULL) {
        Symbol* currentSymbol = (Symbol*) HashMapGet(scope->Symbols, name);
        if (currentSymbol != NULL) {
            return depth;
        }
        if (scope->Type == SCOPE_FUNCTION || scope->Type == SCOPE_GLOBAL) {
            depth++;
        }
        scope = scope->Parent;
    }
    Panic("Not found!");
    return -1;  // Not found
}

bool ScopeHasCapture(Scope* scope, String name) {
    while (scope != NULL) {
        Symbol* symbol = (Symbol*) HashMapGet(scope->Captures, name);
        if (symbol != NULL) {
            return true;
        }
        if (scope->Type == SCOPE_FUNCTION) {
            return false;
        }
        scope = scope->Parent;
    }
    return false;
}

void ScopeSetCapture(Scope* scope,
                     String name,
                     bool   isGlobal,
                     bool   isLocalToFn,
                     bool   isConstant,
                     int    offset) {
    Scope* closureScope = ScopeGetFirst(scope, SCOPE_FUNCTION);
    if (closureScope == NULL) {
        return;
    }
    // Note: memory leak (if 'name' already exists in closureScope->Captures, HashMapSet overwrites
    // the old Symbol* without freeing it)
    Symbol* symbol = CreateSymbol(isGlobal, isLocalToFn, isConstant, offset);
    HashMapSet(closureScope->Captures, name, symbol);
}

Symbol* ScopeGetCapture(Scope* scope, String name, bool recurse) {
    while (scope != NULL) {
        Symbol* symbol = (Symbol*) HashMapGet(scope->Captures, name);
        if (symbol != NULL) {
            return symbol;
        }
        if (scope->Type == SCOPE_FUNCTION) {
            return NULL;
        }
        if (!recurse) {
            return NULL;
        }
        scope = scope->Parent;
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

int ScopeCountNested(Scope* scope, ScopeType type) {
    int count = 0;
    while (scope != NULL) {
        if (scope->Type == type) {
            count++;
        }
        scope = scope->Parent;
    }
    return count;
}

static void _FreeHashMapSymbols(HashMap* hashmap) {
    if (hashmap == NULL) {
        return;
    }

    for (size_t i = 0; i < hashmap->Size; i++) {
        HashNode* node = &hashmap->Buckets[i];
        while (node != NULL && node->Key != NULL) {
            Symbol* symbol = (Symbol*) node->Val;
            if (symbol != NULL) {
                free(symbol);
            }
            node = node->Next;
        }
    }
}

void FreeScope(Scope* scope) {
    if (scope == NULL) {
        return;
    }
    _FreeHashMapSymbols(scope->Symbols);
    FreeHashMap(scope->Symbols);
    _FreeHashMapSymbols(scope->Captures);
    FreeHashMap(scope->Captures);
    free(scope->ContinueJumps);
    free(scope->BreakJumps);
    free(scope);
}