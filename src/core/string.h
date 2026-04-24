/**
 * @file string.h
 * @brief Declares the loader for the @c core:string built-in module (native
 *        string helpers on @c VLT_STR values).
 */

#include "../../utf/utf8proc/utf8proc.h"
#include "../array.h"
#include "../error.h"
#include "../function.h"
#include "../global.h"
#include "../hashmap.h"
#include "../value.h"


#ifndef CORE_STRING_H
#	define CORE_STRING_H

/**
 * @brief Builds the @c import { ... } from "core:string" object: a @c
 * VLT_OBJECT map of native callables (case mapping, Unicode-aware predicates,
 *        whitespace or delimiter @c split, @c join, @c ord / @c chr, UTF-8
 *        @c bytes, RFC 4648 @c encode / @c decode (Base64), @c replace, and
 *        related helpers).
 * @param interpreter Used to allocate the module object and each wrapped native
 *                    @c Value (same lifetime rules as other @c LoadCore*
 * loaders).
 * @return Hash-backed module object; entries are native functions keyed by the
 *         export names registered in @c string.c (never NULL on success).
 */
Value* LoadCoreString(Interpreter* interpreter);

#endif