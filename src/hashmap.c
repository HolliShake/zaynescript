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
    
    size_t oldSize = hashmap->size;
    HashNode* oldBuckets = hashmap->buckets;
    
    // Double the size
    hashmap->size = oldSize * 2;
    hashmap->count = 0;
    hashmap->buckets = Allocate(sizeof(HashNode) * hashmap->size);
    
    if (hashmap->buckets == NULL) {
        // Allocation failed, restore old state
        hashmap->size = oldSize;
        hashmap->buckets = oldBuckets;
        return;
    }
    
    // Initialize new buckets
    for (size_t i = 0; i < hashmap->size; i++) {
        hashmap->buckets[i].Key  = NULL;
        hashmap->buckets[i].Val  = NULL;
        hashmap->buckets[i].Next = NULL;
    }
    
    // Rehash all existing entries
    for (size_t i = 0; i < oldSize; i++) {
        HashNode* node = &oldBuckets[i];
        while (node != NULL && node->Key != NULL) {
            HashNode* next = node->Next;
            HashMapSet(hashmap, node->Key, node->Val);
            
            // free chain nodes (but not the first node in each bucket)
            if (node != &oldBuckets[i]) {
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
    
    hashmap->size = size;
    hashmap->count = 0;
    hashmap->buckets = Allocate(sizeof(HashNode) * size);
    
    if (hashmap->buckets == NULL) {
        free(hashmap);
        return NULL;
    }
    
    for (size_t i = 0; i < size; i++) {
        hashmap->buckets[i].Key  = NULL;
        hashmap->buckets[i].Val  = NULL;
        hashmap->buckets[i].Next = NULL;
    }
    
    return hashmap;
}

void HashMapSet(HashMap* hashmap, String key, void* value) {
    if (hashmap == NULL || key == NULL) {
        return;
    }
    
    // Check load factor and rehash if necessary
    double loadFactor = (double)hashmap->count / (double)hashmap->size;
    if (loadFactor >= LOAD_FACTOR_THRESHOLD) {
        _Rehash(hashmap);
    }
    
    size_t index = _Hash(key, hashmap->size);
    HashNode* node = &hashmap->buckets[index];
    
    // If bucket is empty, use it directly
    if (node->Key == NULL) {
        node->Key = key;
        node->Val = value;
        hashmap->count++;
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
    
    newNode->Key = key;
    newNode->Val = value;
    newNode->Next = NULL;
    current->Next = newNode;
    hashmap->count++;
}

void* HashMapGet(HashMap* hashmap, String key) {
    if (hashmap == NULL || key == NULL) {
        return NULL;
    }
    
    size_t index = _Hash(key, hashmap->size);
    HashNode* node = &hashmap->buckets[index];
    
    while (node != NULL && node->Key != NULL) {
        if (strcmp(node->Key, key) == 0) {
            return node->Val;
        }
        node = node->Next;
    }
    
    return NULL;
}

int HashMapContains(HashMap* hashmap, String key) {
    if (hashmap == NULL || key == NULL) {
        return 0;
    }
    return HashMapGet(hashmap, key) != NULL;
}