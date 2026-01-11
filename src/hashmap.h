#include "./global.h"

#ifndef HASHMAP_H
#define HASHMAP_H

/*
 * CreateHashMap - Creates and initializes a new hash map
 *
 * Creates a hash map with the specified number of buckets. The hash map
 * uses separate chaining for collision resolution.
 *
 * Parameters:
 *   size - Number of buckets to allocate for the hash map
 *
 * Returns:
 *   Pointer to newly allocated HashMap structure, or NULL on failure
 */
HashMap* CreateHashMap(size_t size);

/*
 * HashMapSet - Sets a key-value pair in the hash map
 *
 * If the key already exists, its value will be updated. Otherwise, a new
 * entry will be created. The hash map takes ownership of the key string
 * and value pointer.
 *
 * Parameters:
 *   hashmap - Pointer to the hash map
 *   key     - String key for the entry
 *   value   - Pointer to the value to store
 *
 * Returns:
 *   Nothing
 */
void HashMapSet(HashMap* hashmap, String key, void* value);

/*
 * HashMapGet - Retrieves a value from the hash map by key
 *
 * Searches for the specified key in the hash map and returns the
 * associated value if found.
 *
 * Parameters:
 *   hashmap - Pointer to the hash map
 *   key     - String key to search for
 *
 * Returns:
 *   Pointer to the value if found, NULL otherwise
 */
void* HashMapGet(HashMap* hashmap, String key);

/*
 * HashMapExtend - Extends one hash map with entries from another
 *
 * Entries from the source hash map will be added to the destination hash map.
 * If a key already exists in the destination, its value will be overwritten.
 * The source hash map is not modified.
 *
 * Parameters:
 *   dest - Pointer to the destination hash map
 *   src  - Pointer to the source hash map
 *
 * Returns:
 *   Nothing
 */
void HashMapExtend(HashMap* dest, HashMap* src);

/*
 * HashMapContains - Checks if a key exists in the hash map
 *
 * Searches the hash map to determine if the specified key is present.
 *
 * Parameters:
 *   hashmap - Pointer to the hash map
 *   key     - String key to search for
 *
 * Returns:
 *   1 if the key exists, 0 otherwise
 */
int HashMapContains(HashMap* hashmap, String key);

/*
 * HashMapToString - Converts the hash map to a string representation
 *
 * Creates a string representation of the hash map suitable for debugging
 * or display purposes.
 *
 * Parameters:
 *   hashmap - Pointer to the hash map
 *
 * Returns:
 *   String representation of the hash map
 */
String HashMapToString(HashMap* hashmap);

#endif