/**
 * @file path.h
 * @brief Path helpers and core `path` module loader.
 */

#include "../error.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_PATH_H
#	define CORE_PATH_H

String JoinPath(String baseStr, String segmentStr);
bool   PathExists(String path);
bool   PathIsDirectory(String path);
bool   PathIsRegularFile(String path);
char   NativePathSeparator(void);

Value* LoadCorePath(Interpreter* interpreter);

#endif
