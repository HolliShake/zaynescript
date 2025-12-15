#include "./global.h"

#ifndef HASHMAP_H
#define HASHMAP_H

/**
 * @brief Forward declaration of HashNode structure
 */
typedef struct hashnode_struct HashNode;

/**
 * @brief Node structure for hash map entries
 * 
 * Represents a single key-value pair in the hash map with chaining support
 * for collision resolution.
 */
typedef struct hashnode_struct {
    String    Key;  /**< Key string for the hash map entry */
    void*     Val;  /**< Pointer to the value associated with the key */
    HashNode* Next; /**< Pointer to the next node in case of collision */
} HashNode;

/**
 * @brief Hash map structure for key-value storage
 * 
 * Implements a hash table with separate chaining for collision resolution.
 */
typedef struct hashmap_struct {
    size_t    size;    /**< Total number of buckets in the hash map */
    size_t    count;   /**< Current number of entries in the hash map */
    HashNode* buckets; /**< Array of hash node buckets */
} HashMap;

/**
 * @brief Creates and initializes a new hash map
 * 
 * @param size Number of buckets to allocate for the hash map
 * @return Pointer to newly allocated HashMap structure, or NULL on failure
 */
HashMap* NewHashMap(size_t size);

/**
 * @brief Sets a key-value pair in the hash map
 * 
 * If the key already exists, its value will be updated. Otherwise, a new
 * entry will be created.
 * 
 * @param hashmap Pointer to the hash map
 * @param key String key for the entry
 * @param value Pointer to the value to store
 */
void HashMapSet(HashMap* hashmap, String key, void* value);

/**
 * @brief Retrieves a value from the hash map by key
 * 
 * @param hashmap Pointer to the hash map
 * @param key String key to search for
 * @return Pointer to the value if found, NULL otherwise
 */
void* HashMapGet(HashMap* hashmap, String key);

/**
 * @brief Checks if a key exists in the hash map
 * 
 * @param hashmap Pointer to the hash map
 * @param key String key to search for
 * @return 1 if the key exists, 0 otherwise
 */
int HashMapContains(HashMap* hashmap, String key);

#endif