#include "./global.h"
#include "./hashmap.h"

#ifndef CLASS_H
#define CLASS_H

/**
 * @brief Creates a new user-defined class
 * 
 * @param name Name of the class
 * @param base Pointer to the base class Value, or NULL if none
 * @return Pointer to the newly created UserClass structure
 */
UserClass* CreateUserClass(String name, Value* base);

/**
 * @brief Extends a user-defined class with a base class
 * 
 * @param cls Pointer to the UserClass to extend
 * @param base Pointer to the base class Value
 */
void ClassExtend(UserClass* cls, Value* base);

/**
 * @brief Defines a member (static or instance) on a user-defined class
 * 
 * @param cls Pointer to the UserClass on which to define the member
 * @param key Pointer to the Value representing the member name
 * @param value Pointer to the Value representing the member value
 * @param isStatic Boolean indicating whether the member is static (true) or instance (false)
 */
void ClassDefineMember(UserClass* cls, Value* key, Value* value, bool isStatic);

/**
 * @brief Checks if a user-defined class has a specific member
 * 
 * @param cls Pointer to the UserClass to check
 * @param key String name of the member to look for
 * @param isStatic Boolean indicating whether to check static (true) or instance (false) members
 * @param callable Boolean indicating whether to check for callable members only
 * @return true if the member exists, false otherwise
 */
bool ClassHasMember(UserClass* cls, String key, bool isStatic, bool callable);

/**
 * @brief Converts a user-defined class instance to its string representation
 * 
 * @param instance Pointer to the ClassInstance to convert
 * @return String representation of the class instance
 */
String ClassInstanceToString(ClassInstance* instance);

//

/**
 * @brief Creates a new instance of a user-defined class
 * 
 * @param proto Pointer to the prototype Value of the class instance
 * @return Pointer to the newly created ClassInstance structure
 */
ClassInstance* CreateClassInstance(Value* proto);

/**
 * @brief Converts a user-defined class to its string representation
 * 
 * @param cls Pointer to the UserClass to convert
 * @return String representation of the class
 */
String ClassToString(UserClass* cls);

#endif