/**
 * @file class.h
 * @brief User-defined class and instance management interface
 *
 * This header defines the interface for creating and managing user-defined
 * classes and their instances, including inheritance, member definition,
 * and string representation.
 */

#include "./global.h"
#include "./hashmap.h"

#ifndef CLASS_H
#define CLASS_H

/**
 * Creates a new user-defined class.
 * 
 * Allocates and initializes a new UserClass structure with the specified
 * name and optional base class for inheritance.
 * 
 * @param name Name of the class.
 * @param base Pointer to the base class Value, or NULL if no inheritance.
 * @return Pointer to the newly created UserClass structure, or NULL on allocation failure.
 */
UserClass* CreateUserClass(String name, Value* base);

/**
 * Extends a user-defined class with a base class.
 * 
 * Establishes an inheritance relationship by setting the base class for
 * the specified class. This allows the class to inherit members from the
 * base class.
 * 
 * @param cls Pointer to the UserClass to extend.
 * @param base Pointer to the base class Value.
 */
void ClassExtend(UserClass* cls, Value* base);

/**
 * Defines a member (static or instance) on a user-defined class.
 * 
 * Adds or updates a member on the class. Members can be either static
 * (belonging to the class itself) or instance (belonging to instances
 * of the class).
 * 
 * @param cls Pointer to the UserClass on which to define the member.
 * @param key Pointer to the Value representing the member name.
 * @param value Pointer to the Value representing the member value.
 * @param isStatic Boolean indicating whether the member is static (true) or instance (false).
 */
void ClassDefineMember(UserClass* cls, Value* key, Value* value, bool isStatic);

/**
 * Defines a member (static or instance) on a user-defined class using a string key.
 * 
 * Adds or updates a member on the class. Members can be either static
 * (belonging to the class itself) or instance (belonging to instances
 * of the class). This function uses a string key for the member name.
 * 
 * @param cls Pointer to the UserClass on which to define the member.
 * @param key String name of the member.
 * @param value Pointer to the Value representing the member value.
 * @param isStatic Boolean indicating whether the member is static (true) or instance (false).
 */
void ClassDefineMemberByString(UserClass* cls, String key, Value* value, bool isStatic);

/**
 * Checks if a user-defined class has a specific member.
 * 
 * Searches for a member with the specified name in the class, checking
 * either static or instance members. Optionally filters for callable
 * members only (methods).
 * 
 * @param cls Pointer to the UserClass to check.
 * @param key String name of the member to look for.
 * @param isStatic Boolean indicating whether to check static (true) or instance (false) members.
 * @param callable Boolean indicating whether to check for callable members only.
 * @return true if the member exists, false otherwise.
 */
bool ClassHasMember(UserClass* cls, String key, bool isStatic, bool callable);

/**
 * Retrieves a member (static or instance) from a user-defined class.
 * 
 * Looks up a member by name in the specified class, searching either
 * static or instance members.
 * 
 * @param cls Pointer to the UserClass from which to retrieve the member.
 * @param key String name of the member to retrieve.
 * @param isStatic Boolean indicating whether to look for a static (true) or instance (false) member.
 * @return Pointer to the Value of the member if found, or NULL if not found.
 */
Value* ClassGetMember(UserClass* cls, String key, bool isStatic);

/**
 * Converts a user-defined class instance to its string representation.
 * 
 * Generates a human-readable string representation of the class instance,
 * typically including the class name and instance information.
 * 
 * @param instance Pointer to the ClassInstance to convert.
 * @return String representation of the class instance. Caller is responsible for freeing the returned string.
 */
String ClassInstanceToString(ClassInstance* instance);

/**
 * Creates a new instance of a user-defined class.
 * 
 * Allocates and initializes a new ClassInstance structure with the specified
 * prototype. The prototype defines the class from which this instance is created.
 * 
 * @param proto Pointer to the prototype Value of the class instance.
 * @return Pointer to the newly created ClassInstance structure, or NULL on allocation failure.
 */
ClassInstance* CreateClassInstance(Value* proto);

/**
 * Converts a user-defined class to its string representation.
 * 
 * Generates a human-readable string representation of the class,
 * typically including the class name and type information.
 * 
 * @param cls Pointer to the UserClass to convert.
 * @return String representation of the class. Caller is responsible for freeing the returned string.
 */
String ClassToString(UserClass* cls);

#endif /* CLASS_H */