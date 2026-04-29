
#include "./mysql.h"

#define PROP_DB_PTR "__ptr"

typedef struct mysql_api_struct {
	void* Handle;
	MYSQL* (*init)(MYSQL*);
	MYSQL* (*real_connect)(MYSQL*,
						   const char*,
						   const char*,
						   const char*,
						   const char*,
						   unsigned int,
						   const char*,
						   unsigned long);
	void (*close)(MYSQL*);
	int (*query)(MYSQL*, const char*);
	MYSQL_RES* (*store_result)(MYSQL*);
	void (*free_result)(MYSQL_RES*);
	int (*next_result)(MYSQL*);
	my_ulonglong (*affected_rows)(MYSQL*);
	my_ulonglong (*insert_id)(MYSQL*);
	const char* (*error)(MYSQL*);
	unsigned int (*field_count)(MYSQL*);
	unsigned int (*num_fields)(MYSQL_RES*);
	MYSQL_ROW (*fetch_row)(MYSQL_RES*);
} MysqlApi;

static MysqlApi _mysql_api = { 0 };

static bool _LoadMysqlApi(void) {
	if (_mysql_api.Handle)
		return true;

	const char* candidates[] = { "mariadb-connector-c.dll",
								 "libmariadb.dll",
								 "libmariadb.so",
								 "libmariadb.so.3",
								 "libmysqlclient.so",
								 "libmysqlclient.so.21",
								 NULL };

	for (int i = 0; candidates[i] != NULL; i++) {
		void* handle = dlopen(candidates[i], RTLD_LAZY);
		if (!handle)
			continue;

#define LOAD_SYM(name, type)                                                   \
	do {                                                                       \
		_mysql_api.name = (type) dlsym(handle, "mysql_" #name);                \
		if (!_mysql_api.name) {                                                \
			dlclose(handle);                                                   \
			_mysql_api.Handle = NULL;                                          \
			return false;                                                      \
		}                                                                      \
	} while (0)

		_mysql_api.Handle = handle;
		LOAD_SYM(init, MYSQL * (*) (MYSQL*) );
		LOAD_SYM(real_connect,
				 MYSQL
					 * (*) (MYSQL*,
							const char*,
							const char*,
							const char*,
							const char*,
							unsigned int,
							const char*,
							unsigned long) );
		LOAD_SYM(close, void (*)(MYSQL*));
		LOAD_SYM(query, int (*)(MYSQL*, const char*));
		LOAD_SYM(store_result, MYSQL_RES * (*) (MYSQL*) );
		LOAD_SYM(free_result, void (*)(MYSQL_RES*));
		LOAD_SYM(next_result, int (*)(MYSQL*));
		LOAD_SYM(affected_rows, my_ulonglong (*)(MYSQL*));
		LOAD_SYM(insert_id, my_ulonglong (*)(MYSQL*));
		LOAD_SYM(error, const char* (*) (MYSQL*) );
		LOAD_SYM(field_count, unsigned int (*)(MYSQL*));
		LOAD_SYM(num_fields, unsigned int (*)(MYSQL_RES*));
		LOAD_SYM(fetch_row, MYSQL_ROW (*)(MYSQL_RES*));
#undef LOAD_SYM
		return true;
	}

	return false;
}

static inline Value* _PtrToValue(Interpreter* interp, void* ptr) {
	return NewOpquePtrValue(interp, ptr);
}

static void _MysqlHandleDestroy(Value* val) {
	if (!val || !ValueIsOpaquePtr(val))
		return;
	MYSQL* db = (MYSQL*) val->Value.Opaque;
	if (!db)
		return;
	if (_mysql_api.close)
		_mysql_api.close(db);
	val->Value.Opaque = NULL;
}

static Value* _NewMysqlPtrValue(Interpreter* interp, MYSQL* db) {
	Value* v	 = _PtrToValue(interp, db);
	v->Destroyer = _MysqlHandleDestroy;
	return v;
}

static inline void* _ValueToPtr(Value* val) {
	if (val && ValueIsOpaquePtr(val))
		return val->Value.Opaque;
	return NULL;
}

static MYSQL* _GetDB(ClassInstance* instance) {
	Value* val = (Value*) HashMapGet(instance->Members, PROP_DB_PTR);
	return (MYSQL*) _ValueToPtr(val);
}

static void _SetDB(Interpreter* interp, ClassInstance* instance, MYSQL* db) {
	if (!db) {
		HashMapSet(instance->Members, PROP_DB_PTR, interp->Null);
		return;
	}
	HashMapSet(instance->Members, PROP_DB_PTR, _NewMysqlPtrValue(interp, db));
}

static String _ValueToCString(Value* v) {
	if (!v || ValueIsNull(v))
		return NULL;
	if (ValueIsStr(v)) {
		Rune* runes = (Rune*) v->Value.Opaque;
		return RunesStrToString(runes);
	}
	return ValueToString(v);
}

static unsigned int _CoercePort(Value* v, unsigned int fallback) {
	if (!v || ValueIsNull(v))
		return fallback;
	if (ValueIsInt(v))
		return (unsigned int) v->Value.I32;
	if (ValueIsAnyNum(v))
		return (unsigned int) CoerceToNum(v);
	return fallback;
}

static Value* _DbInit(Interpreter* interp, int argc, Value** arguments) {
	ClassInstance* cls = CoerceToClassInstance(arguments[0]);

	String		 host	  = NULL;
	String		 user	  = NULL;
	String		 password = NULL;
	String		 database = NULL;
	unsigned int port	  = 3306;

	if (argc > 1 && ValueIsObject(arguments[1])) {
		HashMap* opts = CoerceToHashMap(arguments[1]);
		host		  = _ValueToCString((Value*) HashMapGet(opts, "host"));
		user		  = _ValueToCString((Value*) HashMapGet(opts, "user"));
		password	  = _ValueToCString((Value*) HashMapGet(opts, "password"));
		database	  = _ValueToCString((Value*) HashMapGet(opts, "database"));
		port		  = _CoercePort((Value*) HashMapGet(opts, "port"), 3306);
	} else {
		if (argc > 1)
			host = _ValueToCString(arguments[1]);
		if (argc > 2)
			user = _ValueToCString(arguments[2]);
		if (argc > 3)
			password = _ValueToCString(arguments[3]);
		if (argc > 4)
			database = _ValueToCString(arguments[4]);
		if (argc > 5)
			port = _CoercePort(arguments[5], 3306);
	}

	if (!_LoadMysqlApi()) {
		free(host);
		free(user);
		free(password);
		free(database);
		return NewErrorFValue(interp,
							  "%s: could not load libmariadb/libmysqlclient",
							  RUNTIME_ERROR);
	}

	MYSQL* db = _mysql_api.init(NULL);
	if (!db) {
		free(host);
		free(user);
		free(password);
		free(database);
		return NewErrorFValue(interp, "%s: mysql_init failed", RUNTIME_ERROR);
	}

	if (!_mysql_api
			 .real_connect(db, host, user, password, database, port, NULL, 0)) {
		Value* err = NewErrorFValue(interp,
									"%s: mysql_real_connect: %s",
									RUNTIME_ERROR,
									_mysql_api.error(db));
		_mysql_api.close(db);
		free(host);
		free(user);
		free(password);
		free(database);
		return err;
	}

	free(host);
	free(user);
	free(password);
	free(database);
	_SetDB(interp, cls, db);
	return interp->Null;
}

static Value* _DbClose(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	Value*		   ptr = (Value*) HashMapGet(cls->Members, PROP_DB_PTR);
	MYSQL*		   db  = (MYSQL*) _ValueToPtr(ptr);
	if (db) {
		// Prevent later GC from calling mysql_close on the same handle again.
		if (ptr && ValueIsOpaquePtr(ptr))
			ptr->Value.Opaque = NULL;
		_mysql_api.close(db);
		_SetDB(interp, cls, NULL);
	}
	return interp->Null;
}

static Value* _DbExec(Interpreter* interp, int argc, Value** arguments) {
	if (argc < 2 || !ValueIsStr(arguments[1]))
		return NewErrorFValue(
			interp,
			"%s: Database.exec expects 1 string argument: (sql)",
			ARGUMENT_ERROR);

	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	MYSQL*		   db  = _GetDB(cls);
	if (!db)
		return NewErrorFValue(interp, "%s: Database is closed", RUNTIME_ERROR);

	String sql = _ValueToCString(arguments[1]);
	int	   rc  = _mysql_api.query(db, sql);
	free(sql);
	if (rc != 0)
		return NewErrorFValue(interp,
							  "%s: mysql_query: %s",
							  RUNTIME_ERROR,
							  _mysql_api.error(db));

	// Drain and release all result sets for multi-statement safety.
	do {
		MYSQL_RES* rs = _mysql_api.store_result(db);
		if (rs)
			_mysql_api.free_result(rs);
	} while (_mysql_api.next_result(db) == 0);

	Value*	 out = NewObjectValue(interp);
	HashMap* map = CoerceToHashMap(out);
	HashMapSet(map,
			   "affectedRows",
			   NewNumValue(interp, (double) _mysql_api.affected_rows(db)));
	HashMapSet(map,
			   "insertId",
			   NewNumValue(interp, (double) _mysql_api.insert_id(db)));
	return out;
}

static Value* _DbQuery(Interpreter* interp, int argc, Value** arguments) {
	if (argc < 2 || !ValueIsStr(arguments[1]))
		return NewErrorFValue(
			interp,
			"%s: Database.query expects 1 string argument: (sql)",
			ARGUMENT_ERROR);

	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	MYSQL*		   db  = _GetDB(cls);
	if (!db)
		return NewErrorFValue(interp, "%s: Database is closed", RUNTIME_ERROR);

	String sql = _ValueToCString(arguments[1]);
	int	   rc  = _mysql_api.query(db, sql);
	free(sql);
	if (rc != 0)
		return NewErrorFValue(interp,
							  "%s: mysql_query: %s",
							  RUNTIME_ERROR,
							  _mysql_api.error(db));

	MYSQL_RES* res = _mysql_api.store_result(db);
	if (!res) {
		if (_mysql_api.field_count(db) == 0)
			return NewArrayValue(interp);
		return NewErrorFValue(interp,
							  "%s: mysql_store_result: %s",
							  RUNTIME_ERROR,
							  _mysql_api.error(db));
	}

	unsigned int field_count = _mysql_api.num_fields(res);
	Value*		 rows		 = NewArrayValue(interp);
	Array*		 arr		 = (Array*) rows->Value.Opaque;
	MYSQL_ROW	 row;

	while ((row = _mysql_api.fetch_row(res)) != NULL) {
		Value*	 obj = NewObjectValue(interp);
		HashMap* map = CoerceToHashMap(obj);

		for (unsigned int i = 0; i < field_count; i++) {
			char key[32];
			snprintf(key, sizeof(key), "%u", i);
			if (!row[i]) {
				HashMapSet(map, key, interp->Null);
				continue;
			}
			HashMapSet(map, key, NewStrValue(interp, row[i]));
		}
		ArrayPush(arr, obj);
	}

	_mysql_api.free_result(res);
	return rows;
}

static ModuleFunction _DbClassMethods[] = {
	{ .Name		 = CONSTRUCTOR_NAME,
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _DbInit,
	  .Value	 = NULL },
	{ .Name		 = "close",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _DbClose,
	  .Value	 = NULL },
	{ .Name		 = "exec",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _DbExec,
	  .Value	 = NULL },
	{ .Name		 = "query",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _DbQuery,
	  .Value	 = NULL },
	{ .Name = NULL }
};

Value* LoadCoreMysql(Interpreter* interp) {
	Value* classVal =
		NewClassValue(interp, CreateUserClass("Database", interp->Object));
	Class* cls = CoerceToUserClass(classVal);

	for (int i = 0; _DbClassMethods[i].Name != NULL; i++) {
		ModuleFunction* func = &_DbClassMethods[i];
		ClassDefineMemberByString(
			cls,
			func->Name,
			NewNativeFunctionValue(interp,
								   CreateNativeFunctionMeta((String) func->Name,
															func->Argc,
															func->CFunction)),
			false);
	}

	Value*	 module = NewObjectValue(interp);
	HashMap* map	= CoerceToHashMap(module);
	HashMapSet(map, "Database", classVal);
	return module;
}

#undef PROP_DB_PTR
