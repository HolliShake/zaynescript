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
 * @brief Creates a new user-defined class
 * 
 * Allocates and initializes a new UserClass structure with the specified
 * name and optional base class for inheritance.
 * 
 * @param[in] name Name of the class
 * @param[in] base Pointer to the base class Value, or NULL if no inheritance
 * @return Pointer to the newly created UserClass structure, or NULL on allocation failure
 */
UserClass* CreateUserClass(String name, Value* base);

/**
 * @brief Extends a user-defined class with a base class
 * 
 * Establishes an inheritance relationship by setting the base class for
 * the specified class. This allows the class to inherit members from the
 * base class.
 * 
 * @param[in,out] cls Pointer to the UserClass to extend
 * @param[in] base Pointer to the base class Value
 */
void ClassExtend(UserClass* cls, Value* base);

/**
 * @brief Defines a member (static or instance) on a user-defined class
 * 
 * Adds or updates a member on the class. Members can be either static
 * (belonging to the class itself) or instance (belonging to instances
 * of the class).
 * 
 * @param[in,out] cls Pointer to the UserClass on which to define the member
 * @param[in] key Pointer to the Value representing the member name
 * @param[in] value Pointer to the Value representing the member value
 * @param[in] isStatic Boolean indicating whether the member is static (true) or instance (false)
 */
void ClassDefineMember(UserClass* cls, Value* key, Value* value, bool isStatic);

/**
 * @brief Checks if a user-defined class has a specific member
 * 
 * Searches for a member with the specified name in the class, checking
 * either static or instance members. Optionally filters for callable
 * members only (methods).
 * 
 * @param[in] cls Pointer to the UserClass to check
 * @param[in] key String name of the member to look for
 * @param[in] isStatic Boolean indicating whether to check static (true) or instance (false) members
 * @param[in] callable Boolean indicating whether to check for callable members only
 * @return true if the member exists, false otherwise
 */
bool ClassHasMember(UserClass* cls, String key, bool isStatic, bool callable);

/**
 * @brief Retrieves a member (static or instance) from a user-defined class
 * 
 * Looks up a member by name in the specified class, searching either
 * static or instance members.
 * 
 * @param[in] cls Pointer to the UserClass from which to retrieve the member
 * @param[in] key String name of the member to retrieve
 * @param[in] isStatic Boolean indicating whether to look for a static (true) or instance (false) member
 * @return Pointer to the Value of the member if found, or NULL if not found
 */
Value* ClassGetMember(UserClass* cls, String key, bool isStatic);

/**
 * @brief Converts a user-defined class instance to its string representation
 * 
 * Generates a human-readable string representation of the class instance,
 * typically including the class name and instance information.
 * 
 * @param[in] instance Pointer to the ClassInstance to convert
 * @return String representation of the class instance. Caller is responsible for freeing the returned string.
 */
String ClassInstanceToString(ClassInstance* instance);

/**
 * @brief Creates a new instance of a user-defined class
 * 
 * Allocates and initializes a new ClassInstance structure with the specified
 * prototype. The prototype defines the class from which this instance is created.
 * 
 * @param[in] proto Pointer to the prototype Value of the class instance
 * @return Pointer to the newly created ClassInstance structure, or NULL on allocation failure
 */
ClassInstance* CreateClassInstance(Value* proto);

/**
 * @brief Converts a user-defined class to its string representation
 * 
 * Generates a human-readable string representation of the class,
 * typically including the class name and type information.
 * 
 * @param[in] cls Pointer to the UserClass to convert
 * @return String representation of the class. Caller is responsible for freeing the returned string.
 */
String ClassToString(UserClass* cls);

#endif /* CLASS_H */