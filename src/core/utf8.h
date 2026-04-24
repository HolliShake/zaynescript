/**
 * @file utf8.h
 * @brief UTF-8 / Unicode helpers built on utf8proc, oriented around @c Rune
 * strings (NUL-terminated UTF-32 code units), matching @ref VLT_STR storage.
 *
 * All @c Rune* results are allocated with @ref Allocate and must be released
 * with @c free. All @c String (UTF-8) results are @ref Allocate NUL-terminated
 * buffers and must be released with @c free.
 *
 * The @c core:utf8 module exposes a script-facing API; load with
 * @c LoadCoreUtf8 / @c import { ... } from "core:utf8".
 */
#include "../../utf/utf8proc/utf8proc.h"
#include "../error.h"
#include "../global.h"
#include "../hashmap.h"
#include "../value.h"

#ifndef CORE_UTF8_H
#	define CORE_UTF8_H

/** Loads the @c core:utf8 built-in module (unicode helpers). */
Value* LoadCoreUtf8(Interpreter* interpreter);

#endif
