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

Value* CreateRegexClass(Interpreter* interpreter);
Value* LoadCoreRegex(Interpreter* interpreter);

#endif
