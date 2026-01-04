#include "./global.h"

#ifndef HASHMAP_H
#define HASHMAP_H

/**
 * @brief Creates and initializes a new hash map
 * 
 * @param size Number of buckets to allocate for the hash map
 * @return Pointer to newly allocated HashMap structure, or NULL on failure
 */
HashMap* CreateHashMap(size_t size);

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
 * @brief Extends one hash map with entries from another
 * 
 * Entries from the source hash map will be added to the destination hash map.
 * If a key already exists in the destination, its value will be overwritten.
 * 
 * @param dest Pointer to the destination hash map
 * @param src Pointer to the source hash map
 */
void HashMapExtend(HashMap* dest, HashMap* src);

/**
 * @brief Checks if a key exists in the hash map
 * 
 * @param hashmap Pointer to the hash map
 * @param key String key to search for
 * @return 1 if the key exists, 0 otherwise
 */
int HashMapContains(HashMap* hashmap, String key);

/**
 * @brief Converts the hash map to a string representation
 * 
 * @param hashmap Pointer to the hash map
 * @return String representation of the hash map
 */
String HashMapToString(HashMap* hashmap);

#endif