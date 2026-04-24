#include "../error.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_MYSQL_H
#	define CORE_MYSQL_H


typedef struct st_mysql		MYSQL;
typedef struct st_mysql_res MYSQL_RES;
typedef char**				MYSQL_ROW;
typedef unsigned long long	my_ulonglong;

Value* LoadCoreMysql(Interpreter* interpreter);

#endif /* CORE_MYSQL_H */
