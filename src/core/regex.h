/**
 * @file regex.h
 * @brief Core RegExp built on QuickJS libregexp (lre_compile / lre_exec).
 */

#include "../../libregex/libregexp.h"
#include "../class.h"
#include "../error.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_REGEX_H
#	define CORE_REGEX_H

/**
 * @brief Constructs the @c RegExp class (subclass of @c Object) with native
 *        instance methods: constructor, @c search, @c match, @c fullmatch,
 *        @c findall, @c exec, and @c test, and a static @c flags object
 *        enumerating libregexp option bits.
 * @param interpreter Runtime used to allocate the class, methods, and flag map.
 * @return VLT_CLASS value; allocation failures are surfaced from nested helpers
 *         as in other class constructors.
 * @origin src/core/regex.c
 */
Value* CreateRegexClass(Interpreter* interpreter);

/**
 * @brief Builds the @c core:regex module: top-level @c compile, @c search,
 *        @c match, @c fullmatch, and @c findall; registers @c RegExp and @c
 *        Pattern to the class from @ref CreateRegexClass, exposes a shared
 *        @c flags object, and copies short @c I / @c M / @c S / @c U aliases
 *        onto the module.
 * @param interpreter Runtime used to allocate the module table and native
 *        functions.
 * @return VLT_OBJECT map returned to script @c import, containing the entries
 *         above.
 * @origin src/core/regex.c
 */
Value* LoadCoreRegex(Interpreter* interpreter);

#endif
