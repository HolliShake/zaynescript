#include "./global.h"

#ifndef HASHMAP_H
#define HASHMAP_H

/**
 * CreateHashMap - Creates and initializes a new hash map
 *
 * Creates a hash map with the specified number of buckets. The hash map
 * uses separate chaining for collision resolution.
 *
 * @param size Number of buckets to allocate for the hash map
 * @return Pointer to newly allocated HashMap structure, or NULL on failure
 */
HashMap* CreateHashMap(size_t size);

/**
 * HashMapSet - Sets a key-value pair in the hash map
 *
 * If the key already exists, its value will be updated. Otherwise, a new
 * entry will be created. The hash map takes ownership of the key string
 * and value pointer.
 *
 * @param hashmap Pointer to the hash map
 * @param key String key for the entry
 * @param value Pointer to the value to store
 */
void HashMapSet(HashMap* hashmap, String key, void* value);

/**
 * HashMapGet - Retrieves a value from the hash map by key
 *
 * Searches for the specified key in the hash map and returns the
 * associated value if found.
 *
 * @param hashmap Pointer to the hash map
 * @param key String key to search for
 * @return Pointer to the value if found, NULL otherwise
 */
void* HashMapGet(HashMap* hashmap, String key);

/**
 * HashMapExtend - Extends one hash map with entries from another
 *
 * Entries from the source hash map will be added to the destination hash map.
 * If a key already exists in the destination, its value will be overwritten.
 * The source hash map is not modified.
 *
 * @param dest Pointer to the destination hash map
 * @param src Pointer to the source hash map
 */
void HashMapExtend(HashMap* dest, HashMap* src);

/**
 * HashMapContains - Checks if a key exists in the hash map
 *
 * Searches the hash map to determine if the specified key is present.
 *
 * @param hashmap Pointer to the hash map
 * @param key String key to search for
 * @return 1 if the key exists, 0 otherwise
 */
int HashMapContains(HashMap* hashmap, String key);

/**
 * HashMapToString - Converts the hash map to a string representation
 *
 * Creates a string representation of the hash map suitable for debugging
 * or display purposes.
 *
 * @param hashmap Pointer to the hash map
 * @return String representation of the hash map
 */
String HashMapToString(HashMap* hashmap);

/**
 * FreeHashMap - Frees the memory used by the hash map
 *
 * Deallocates all memory associated with the hash map, including keys and
 * values. The caller is responsible for ensuring that any pointers stored
 * as values are properly freed if necessary.
 *
 * @param hashmap Pointer to the hash map to free
 */
void FreeHashMap(HashMap* hashmap);

#endif