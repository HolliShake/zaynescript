#include "../error.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_MYSQL_H
#	define CORE_MYSQL_H

/**
 * @file mysql.h
 * @brief Public bridge types and loader entry for the optional MySQL/MariaDB
 *        core module.
 */

/**
 * @brief Opaque connection handle from libmysqlclient/libmariadb used by the
 *        Database wrapper in mysql.c.
 */
typedef struct st_mysql		MYSQL;
/**
 * @brief Opaque result-set handle returned by mysql_store_result() for query
 *        reads.
 */
typedef struct st_mysql_res MYSQL_RES;
/**
 * @brief One fetched result row represented as an array of C strings (NULL
 *        entries indicate SQL NULL).
 */
typedef char**				MYSQL_ROW;
/**
 * @brief Unsigned 64-bit count type used by the client API for affected rows
 *        and insert IDs.
 */
typedef unsigned long long	my_ulonglong;

/**
 * @brief Creates the `Database` class-backed module that lazily loads
 *        libmariadb/libmysqlclient and exposes connect/query/exec operations.
 * @param interpreter Runtime that owns created class/functions and receives
 *        Error values when the native client library is unavailable.
 * @return Module object containing `Database`.
 */
Value* LoadCoreMysql(Interpreter* interpreter);

#endif /* CORE_MYSQL_H */
