#include "./hashmap.h"

#define LOAD_FACTOR_THRESHOLD 0.75

static size_t _Hash(String key, size_t size) {
    if (key == NULL || size == 0) {
        return 0;
    }
    size_t hash = 0;
    while (*key) {
        hash = (hash * 31) + *key;
        key++;
    }
    return hash % size;
}

static void _Rehash(HashMap* hashmap) {
    if (hashmap == NULL) {
        return;
    }
    
    size_t oldSize = hashmap->Size;
    HashNode* oldBuckets = hashmap->Buckets;
    
    // Double the size
    hashmap->Size = oldSize * 2;
    hashmap->Count = 0;
    hashmap->Buckets = Allocate(sizeof(HashNode) * hashmap->Size);
    
    if (hashmap->Buckets == NULL) {
        // Allocation failed, restore old state
        hashmap->Size = oldSize;
        hashmap->Buckets = oldBuckets;
        return;
    }
    
    // Initialize new buckets
    for (size_t i = 0; i < hashmap->Size; i++) {
        hashmap->Buckets[i].Key  = NULL;
        hashmap->Buckets[i].Val  = NULL;
        hashmap->Buckets[i].Next = NULL;
    }
    
    // Rehash all existing entries
    for (size_t i = 0; i < oldSize; i++) {
        HashNode* node = &oldBuckets[i];
        while (node != NULL && node->Key != NULL) {
            HashNode* next = node->Next;
            //Note: memory leak (HashMapSet internally AllocateStrings the key, but the old node->Key is never freed before the node is discarded)
            HashMapSet(hashmap, node->Key, node->Val);
            
            // free chain nodes (but not the first node in each bucket)
            if (node != &oldBuckets[i]) {
                //Note: memory leak (node->Key string is not freed before freeing the node struct; the key was heap-allocated by a prior AllocateString call)
                free(node);
            }
            node = next;
        }
    }
    
    // free old buckets array
    free(oldBuckets);
}

HashMap* CreateHashMap(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    HashMap* hashmap = Allocate(sizeof(HashMap));
    if (hashmap == NULL) {
        return NULL;
    }
    
    hashmap->Size = size;
    hashmap->Count = 0;
    hashmap->Buckets = Allocate(sizeof(HashNode) * size);
    
    if (hashmap->Buckets == NULL) {
        free(hashmap);
        return NULL;
    }
    
    for (size_t i = 0; i < size; i++) {
        hashmap->Buckets[i].Key  = NULL;
        hashmap->Buckets[i].Val  = NULL;
        hashmap->Buckets[i].Next = NULL;
    }
    
    return hashmap;
}

void HashMapSet(HashMap* hashmap, String key, void* value) {
    if (hashmap == NULL || key == NULL) {
        return;
    }
    
    // Check load factor and rehash if necessary
    double loadFactor = (double)hashmap->Count / (double)hashmap->Size;
    if (loadFactor >= LOAD_FACTOR_THRESHOLD) {
        _Rehash(hashmap);
    }
    
    size_t index = _Hash(key, hashmap->Size);
    HashNode* node = &hashmap->Buckets[index];
    
    // If bucket is empty, use it directly
    if (node->Key == NULL) {
        node->Key = AllocateString(key);
        node->Val = value;
        hashmap->Count++;
        return;
    }
    
    // Check if key already exists in chain
    HashNode* current = node;
    while (current != NULL) {
        if (strcmp(current->Key, key) == 0) {
            current->Val = value;
            return;
        }
        if (current->Next == NULL) {
            break;
        }
        current = current->Next;
    }
    
    // Add new node to chain
    HashNode* newNode = Allocate(sizeof(HashNode));
    if (newNode == NULL) {
        return;
    }
    
    newNode->Key = AllocateString(key);
    newNode->Val = value;
    newNode->Next = NULL;
    current->Next = newNode;
    hashmap->Count++;
}

void* HashMapGet(HashMap* hashmap, String key) {
    if (hashmap == NULL || key == NULL) {
        return NULL;
    }
    
    size_t index = _Hash(key, hashmap->Size);
    HashNode* node = &hashmap->Buckets[index];
    
    while (node != NULL && node->Key != NULL) {
        if (strcmp(node->Key, key) == 0) {
            return node->Val;
        }
        node = node->Next;
    }
    
    return NULL;
}

void HashMapExtend(HashMap* dest, HashMap* src) {
    if (dest == NULL || src == NULL) {
        return;
    }
    
    // Pre-calculate if we need to rehash to avoid multiple rehashes
    size_t totalCount = dest->Count + src->Count;
    double projectedLoadFactor = (double)totalCount / (double)dest->Size;
    
    // Rehash proactively if needed to avoid multiple rehashes during insertion
    while (projectedLoadFactor >= LOAD_FACTOR_THRESHOLD) {
        _Rehash(dest);
        projectedLoadFactor = (double)totalCount / (double)dest->Size;
    }
    
    for (size_t i = 0; i < src->Size; i++) {
        HashNode* node = &src->Buckets[i];
        while (node != NULL && node->Key != NULL) {
            // Clone the key from source
            String clonedKey = AllocateString(node->Key);
            if (clonedKey == NULL) {
                node = node->Next;
                continue;
            }
            
            // Direct insertion without load factor check since we pre-rehashed
            size_t index = _Hash(clonedKey, dest->Size);
            HashNode* destNode = &dest->Buckets[index];
            
            // If bucket is empty, use it directly
            if (destNode->Key == NULL) {
                destNode->Key = clonedKey;
                destNode->Val = node->Val;
                dest->Count++;
            } else {
                // Check if key exists or find end of chain
                HashNode* current = destNode;
                bool found = false;
                while (current != NULL) {
                    if (strcmp(current->Key, clonedKey) == 0) {
                        current->Val = node->Val;
                        free(clonedKey); // Free cloned key since we're updating existing
                        found = true;
                        break;
                    }
                    if (current->Next == NULL) {
                        break;
                    }
                    current = current->Next;
                }
                
                // Add new node if key doesn't exist
                if (!found) {
                    HashNode* newNode = Allocate(sizeof(HashNode));
                    if (newNode != NULL) {
                        newNode->Key = clonedKey;
                        newNode->Val = node->Val;
                        newNode->Next = NULL;
                        current->Next = newNode;
                        dest->Count++;
                    } else {
                        free(clonedKey); // Free cloned key if node allocation failed
                    }
                }
            }
            
            node = node->Next;
        }
    }
}

int HashMapContains(HashMap* hashmap, String key) {
    if (hashmap == NULL || key == NULL) {
        return 0;
    }
    return HashMapGet(hashmap, key) != NULL;
}

extern String ValueToString(Value* value);

String HashMapToString(HashMap* hashmap) {
    if (hashmap == NULL) {
        return NULL;
    }
    
    // Calculate approximate size needed for string representation
    size_t bufferSize = 100; // Initial size for "{ ... }"
    for (size_t i = 0; i < hashmap->Size; i++) {
        HashNode* node = &hashmap->Buckets[i];
        while (node != NULL && node->Key != NULL) {
            String valStr = ValueToString((Value*)node->Val);
            bufferSize += strlen(node->Key) + (valStr ? strlen(valStr) : 4) + 10;
            node = node->Next;
            free(valStr);
        }
    }
    
    String result = Allocate(bufferSize);
    if (result == NULL) {
        return NULL;
    }
    
    strcpy(result, "{ ");
    bool first = true;
    
    for (size_t i = 0; i < hashmap->Size; i++) {
        HashNode* node = &hashmap->Buckets[i];
        while (node != NULL && node->Key != NULL) {
            if (!first) {
                strcat(result, ", ");
            }
            strcat(result, "\"");
            strcat(result, node->Key);
            strcat(result, "\": ");
            String valStr = ValueToString((Value*)node->Val);
            if (valStr != NULL) {
                strcat(result, valStr);
                free(valStr);
            }
            first = false;
            node = node->Next;
        }
    }
    
    strcat(result, " }");
    return result;
}

void FreeHashMap(HashMap* hashmap) {
    if (hashmap == NULL) {
        return;
    }
    
    for (size_t i = 0; i < hashmap->Size; i++) {
        HashNode* node = &hashmap->Buckets[i];
        while (node != NULL && node->Key != NULL) {
            HashNode* next = node->Next;
            free(node->Key);
            node->Key = NULL;
            // Note: we do not free node->Val here since we don't know its type
            if (node != &hashmap->Buckets[i]) {
                free(node); // Free chain nodes, but not the first node in each bucket
            }
            node = next;
        }
    }
    
    free(hashmap->Buckets);
    free(hashmap);
}