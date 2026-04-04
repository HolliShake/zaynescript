#include "./sqlite.h"

#include "../../sqlite/sqlite3.h"

// ─── Module-level handle for the Statement class
// ────────────────────────────── Kept alive through Database's static members
// (see _BuildDbClass).
static Value* _StmtClass = NULL;

// ─── Pointer encoding helpers
// ─────────────────────────────────────────────────
static inline Value* _PtrToValue(Interpreter* interp, void* ptr) {
	return NewOpquePtrValue(interp, ptr);
}

static inline void* _ValueToPtr(Value* val) {
	if (val && ValueIsOpaquePtr(val))
		return val->Value.Opaque;
	return NULL;
}

// ─── Database handle helpers
// ──────────────────────────────────────────────────
static sqlite3* _GetDB(ClassInstance* instance) {
	Value* val = (Value*) HashMapGet(instance->Members, "_ptr");
	return (sqlite3*) _ValueToPtr(val);
}

static void _SetDB(Interpreter* interp, ClassInstance* instance, sqlite3* db) {
	HashMapSet(instance->Members, "_ptr", _PtrToValue(interp, db));
}

// ─── Statement handle helpers
// ─────────────────────────────────────────────────
static sqlite3_stmt* _GetStmt(ClassInstance* instance) {
	Value* val = (Value*) HashMapGet(instance->Members, "_stmt");
	return (sqlite3_stmt*) _ValueToPtr(val);
}

static void
_SetStmt(Interpreter* interp, ClassInstance* instance, sqlite3_stmt* stmt) {
	HashMapSet(instance->Members, "_stmt", _PtrToValue(interp, stmt));
}

static sqlite3* _GetStmtDB(ClassInstance* instance) {
	Value* val = (Value*) HashMapGet(instance->Members, "_db");
	return (sqlite3*) _ValueToPtr(val);
}

// ─── Row helper
// ─────────────────────────────────────────────────────────────── Converts the
// current row of a prepared statement into an interpreter object.
static Value* _StmtRowToObject(Interpreter* interp, sqlite3_stmt* stmt) {
	Value*	 row = NewObjectValue(interp);
	HashMap* map = CoerceToHashMap(row);
	int		 n	 = sqlite3_column_count(stmt);

	for (int i = 0; i < n; i++) {
		const char* colName = sqlite3_column_name(stmt, i);
		Value*		colVal;

		switch (sqlite3_column_type(stmt, i)) {
			case SQLITE_INTEGER:
				colVal = NewIntValue(interp, sqlite3_column_int(stmt, i));
				break;
			case SQLITE_FLOAT:
				colVal = NewNumValue(interp, sqlite3_column_double(stmt, i));
				break;
			case SQLITE3_TEXT:
				{
					const char* text =
						(const char*) sqlite3_column_text(stmt, i);
					colVal = NewStrValue(interp, text ? (String) text : "");
					break;
				}
			case SQLITE_NULL:
			default:
				colVal = interp->Null;
				break;
		}

		HashMapSet(map, (String) colName, colVal);
	}
	return row;
}

// ─── Bind helper
// ──────────────────────────────────────────────────────────────
static int _BindParam(sqlite3_stmt* stmt, int idx, Value* val) {
	if (ValueIsNull(val)) {
		return sqlite3_bind_null(stmt, idx);
	} else if (ValueIsInt(val)) {
		return sqlite3_bind_int(stmt, idx, val->Value.I32);
	} else if (ValueIsNum(val)) {
		return sqlite3_bind_double(stmt, idx, val->Value.Num);
	} else if (ValueIsAnyNum(val)) {
		return sqlite3_bind_double(stmt, idx, CoerceToNum(val));
	} else if (ValueIsBool(val)) {
		return sqlite3_bind_int(stmt, idx, val->Value.I32);
	} else if (ValueIsStr(val)) {
		Rune*  runes = (Rune*) val->Value.Opaque;
		String str	 = RunesStrToString(runes);
		int	   rc	 = sqlite3_bind_text(stmt, idx, str, -1, SQLITE_TRANSIENT);
		free(str);
		return rc;
	} else {
		String s  = ValueToString(val);
		int	   rc = sqlite3_bind_text(stmt, idx, s, -1, SQLITE_TRANSIENT);
		free(s);
		return rc;
	}
}

// =============================================================================
// Statement class
// =============================================================================

static Value* _StmtInit(Interpreter* interp, int argc, Value** arguments) {
	// Instances are created internally via Database.prepare(); init is a no-op.
	(void) argc;
	(void) arguments;
	return interp->Null;
}

static Value* _StmtStep(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	if (!stmt)
		return NewErrorValue(interp, "Statement is finalized or invalid");

	int rc = sqlite3_step(stmt);
	if (rc == SQLITE_ROW)
		return interp->True;
	if (rc == SQLITE_DONE)
		return interp->False;

	sqlite3* db = _GetStmtDB(cls);
	return NewErrorFValue(interp,
						  "sqlite3_step: %s",
						  db ? sqlite3_errmsg(db) : "unknown error");
}

static Value* _StmtReset(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	if (!stmt)
		return NewErrorValue(interp, "Statement is finalized or invalid");
	sqlite3_reset(stmt);
	return interp->Null;
}

static Value* _StmtFinalize(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	if (!stmt)
		return interp->Null;
	sqlite3_finalize(stmt);
	// Clear the pointer so double-finalize is safe.
	HashMapSet(cls->Members, "_stmt", _PtrToValue(interp, NULL));
	return interp->Null;
}

static Value* _StmtBind(Interpreter* interp, int argc, Value** arguments) {
	if (argc < 3)
		return NewErrorValue(
			interp,
			"Statement.bind expects 2 arguments: (index, value)");

	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	if (!stmt)
		return NewErrorValue(interp, "Statement is finalized or invalid");

	int idx = (int) CoerceToNum(arguments[1]);
	int rc	= _BindParam(stmt, idx, arguments[2]);
	if (rc != SQLITE_OK) {
		sqlite3* db = _GetStmtDB(cls);
		return NewErrorFValue(interp,
							  "sqlite3_bind: %s",
							  db ? sqlite3_errmsg(db) : "unknown error");
	}
	return interp->Null;
}

static Value* _StmtGetRow(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	if (!stmt)
		return NewErrorValue(interp, "Statement is finalized or invalid");
	return _StmtRowToObject(interp, stmt);
}

static Value*
_StmtColumnCount(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	if (!stmt)
		return NewIntValue(interp, 0);
	return NewIntValue(interp, sqlite3_column_count(stmt));
}

static Value*
_StmtColumnName(Interpreter* interp, int argc, Value** arguments) {
	if (argc < 2)
		return NewErrorValue(
			interp,
			"Statement.columnName expects 1 argument: (index)");

	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	if (!stmt)
		return NewErrorValue(interp, "Statement is finalized or invalid");

	int			idx	 = (int) CoerceToNum(arguments[1]);
	const char* name = sqlite3_column_name(stmt, idx);
	return NewStrValue(interp, name ? (String) name : "");
}

static ModuleFunction _StmtClassMethods[] = {
	{ .Name		 = CONSTRUCTOR_NAME,
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _StmtInit,
	  .Value	 = NULL },
	{ .Name		 = "step",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StmtStep,
	  .Value	 = NULL },
	{ .Name		 = "reset",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StmtReset,
	  .Value	 = NULL },
	{ .Name		 = "finalize",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StmtFinalize,
	  .Value	 = NULL },
	{ .Name		 = "bind",
	  .Argc		 = 3,
	  .CFunction = (NativeFunctionCallback) _StmtBind,
	  .Value	 = NULL },
	{ .Name		 = "getRow",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StmtGetRow,
	  .Value	 = NULL },
	{ .Name		 = "columnCount",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StmtColumnCount,
	  .Value	 = NULL },
	{ .Name		 = "columnName",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _StmtColumnName,
	  .Value	 = NULL },
	{ .Name = NULL }
};

// =============================================================================
// Database class
// =============================================================================

static Value* _DbInit(Interpreter* interp, int argc, Value** arguments) {
	ClassInstance* cls = CoerceToClassInstance(arguments[0]);

	const char* path	  = ":memory:";
	String		pathAlloc = NULL;

	if (argc >= 2 && ValueIsStr(arguments[1])) {
		Rune* runes = (Rune*) arguments[1]->Value.Opaque;
		pathAlloc	= RunesStrToString(runes);
		path		= pathAlloc;
	}

	sqlite3* db = NULL;
	int		 rc = sqlite3_open(path, &db);

	if (pathAlloc)
		free(pathAlloc);

	if (rc != SQLITE_OK) {
		Value* err =
			NewErrorFValue(interp, "sqlite3_open: %s", sqlite3_errmsg(db));
		if (db)
			sqlite3_close(db);
		return err;
	}

	_SetDB(interp, cls, db);
	return interp->Null;
}

static Value* _DbClose(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	sqlite3*	   db  = _GetDB(cls);
	if (db) {
		sqlite3_close(db);
		_SetDB(interp, cls, NULL);
	}
	return interp->Null;
}

static Value* _DbExec(Interpreter* interp, int argc, Value** arguments) {
	if (argc < 2)
		return NewErrorValue(interp, "Database.exec expects 1 argument: (sql)");

	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	sqlite3*	   db  = _GetDB(cls);
	if (!db)
		return NewErrorValue(interp, "Database is closed");

	if (!ValueIsStr(arguments[1]))
		return NewErrorValue(interp, "Database.exec: sql must be a string");

	Rune*  runes  = (Rune*) arguments[1]->Value.Opaque;
	String sql	  = RunesStrToString(runes);
	char*  errmsg = NULL;
	int	   rc	  = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	free(sql);

	if (rc != SQLITE_OK) {
		Value* err = NewErrorFValue(interp, "sqlite3_exec: %s", errmsg);
		sqlite3_free(errmsg);
		return err;
	}
	return interp->Null;
}

static Value* _DbQuery(Interpreter* interp, int argc, Value** arguments) {
	if (argc < 2)
		return NewErrorValue(
			interp,
			"Database.query expects at least 1 argument: (sql, ...params)");

	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	sqlite3*	   db  = _GetDB(cls);
	if (!db)
		return NewErrorValue(interp, "Database is closed");

	if (!ValueIsStr(arguments[1]))
		return NewErrorValue(interp, "Database.query: sql must be a string");

	Rune*  runes = (Rune*) arguments[1]->Value.Opaque;
	String sql	 = RunesStrToString(runes);

	sqlite3_stmt* stmt = NULL;
	int			  rc   = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	free(sql);

	if (rc != SQLITE_OK)
		return NewErrorFValue(interp,
							  "sqlite3_prepare_v2: %s",
							  sqlite3_errmsg(db));

	// Bind variadic parameters (arguments[2..argc-1] → index 1..N)
	for (int i = 2; i < argc; i++) {
		rc = _BindParam(stmt, i - 1, arguments[i]);
		if (rc != SQLITE_OK) {
			sqlite3_finalize(stmt);
			return NewErrorFValue(interp,
								  "sqlite3_bind: %s",
								  sqlite3_errmsg(db));
		}
	}

	Value* rows = NewArrayValue(interp);
	Array* arr	= (Array*) rows->Value.Opaque;

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		ArrayPush(arr, _StmtRowToObject(interp, stmt));

	sqlite3_finalize(stmt);

	if (rc != SQLITE_DONE)
		return NewErrorFValue(interp, "sqlite3_step: %s", sqlite3_errmsg(db));

	return rows;
}

static Value* _DbRun(Interpreter* interp, int argc, Value** arguments) {
	if (argc < 2)
		return NewErrorValue(
			interp,
			"Database.run expects at least 1 argument: (sql, ...params)");

	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	sqlite3*	   db  = _GetDB(cls);
	if (!db)
		return NewErrorValue(interp, "Database is closed");

	if (!ValueIsStr(arguments[1]))
		return NewErrorValue(interp, "Database.run: sql must be a string");

	Rune*  runes = (Rune*) arguments[1]->Value.Opaque;
	String sql	 = RunesStrToString(runes);

	sqlite3_stmt* stmt = NULL;
	int			  rc   = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	free(sql);

	if (rc != SQLITE_OK)
		return NewErrorFValue(interp,
							  "sqlite3_prepare_v2: %s",
							  sqlite3_errmsg(db));

	// Bind variadic parameters (arguments[2..argc-1] → index 1..N)
	for (int i = 2; i < argc; i++) {
		rc = _BindParam(stmt, i - 1, arguments[i]);
		if (rc != SQLITE_OK) {
			sqlite3_finalize(stmt);
			return NewErrorFValue(interp,
								  "sqlite3_bind: %s",
								  sqlite3_errmsg(db));
		}
	}

	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	if (rc != SQLITE_DONE && rc != SQLITE_ROW)
		return NewErrorFValue(interp, "sqlite3_step: %s", sqlite3_errmsg(db));

	Value*	 result = NewObjectValue(interp);
	HashMap* map	= CoerceToHashMap(result);
	HashMapSet(map,
			   "lastInsertRowid",
			   NewNumValue(interp, (double) sqlite3_last_insert_rowid(db)));
	HashMapSet(map, "changes", NewIntValue(interp, sqlite3_changes(db)));
	return result;
}

static Value* _DbPrepare(Interpreter* interp, int argc, Value** arguments) {
	if (argc < 2)
		return NewErrorValue(interp,
							 "Database.prepare expects 1 argument: (sql)");

	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	sqlite3*	   db  = _GetDB(cls);
	if (!db)
		return NewErrorValue(interp, "Database is closed");

	if (!ValueIsStr(arguments[1]))
		return NewErrorValue(interp, "Database.prepare: sql must be a string");

	Rune*  runes = (Rune*) arguments[1]->Value.Opaque;
	String sql	 = RunesStrToString(runes);

	sqlite3_stmt* stmt = NULL;
	int			  rc   = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	free(sql);

	if (rc != SQLITE_OK)
		return NewErrorFValue(interp,
							  "sqlite3_prepare_v2: %s",
							  sqlite3_errmsg(db));

	// Retrieve Statement class stored in Database's static members.
	Class* dbCls		= CoerceToUserClass(cls->Proto);
	Value* stmtClassVal = ClassGetMember(dbCls, "_StmtClass", true);
	if (!stmtClassVal) {
		sqlite3_finalize(stmt);
		return NewErrorValue(interp, "internal: Statement class not found");
	}

	ClassInstance* stmtInst = CreateClassInstance(stmtClassVal);
	_SetStmt(interp, stmtInst, stmt);
	// Store db pointer so step() can report errors.
	HashMapSet(stmtInst->Members, "_db", _PtrToValue(interp, db));

	return NewClassInstanceValue(interp, stmtInst);
}

static Value*
_DbLastInsertRowid(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	sqlite3*	   db  = _GetDB(cls);
	if (!db)
		return NewIntValue(interp, 0);
	return NewNumValue(interp, (double) sqlite3_last_insert_rowid(db));
}

static Value* _DbChanges(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	sqlite3*	   db  = _GetDB(cls);
	if (!db)
		return NewIntValue(interp, 0);
	return NewIntValue(interp, sqlite3_changes(db));
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
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _DbQuery,
	  .Value	 = NULL },
	{ .Name		 = "run",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _DbRun,
	  .Value	 = NULL },
	{ .Name		 = "prepare",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _DbPrepare,
	  .Value	 = NULL },
	{ .Name		 = "lastInsertRowid",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _DbLastInsertRowid,
	  .Value	 = NULL },
	{ .Name		 = "changes",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _DbChanges,
	  .Value	 = NULL },
	{ .Name = NULL }
};

// =============================================================================
// Class factory + module loader
// =============================================================================

static Value*
_BuildClass(Interpreter* interp, const char* name, ModuleFunction methods[]) {
	Value* classVal =
		NewClassValue(interp, CreateUserClass((String) name, NULL));
	Class* cls = CoerceToUserClass(classVal);

	for (int i = 0; methods[i].Name != NULL; i++) {
		ModuleFunction* func = &methods[i];
		if (func->CFunction) {
			ClassDefineMemberByString(
				cls,
				func->Name,
				NewNativeFunctionValue(
					interp,
					CreateNativeFunctionMeta((String) func->Name,
											 func->Argc,
											 func->CFunction)),
				false);
		}
	}
	return classVal;
}

Value* LoadCoreSqlite(Interpreter* interp) {
	// Build Statement class first; cache it so prepare() can use it.
	if (!_StmtClass)
		_StmtClass = _BuildClass(interp, "Statement", _StmtClassMethods);

	Value* _DbmsClass = _BuildClass(interp, "Database", _DbClassMethods);

	// Store Statement class as a static member of Database so the GC can
	// traverse it as long as the Database class is alive.
	ClassDefineMemberByString(CoerceToUserClass(_DbmsClass),
							  "_StmtClass",
							  _StmtClass,
							  true);

	Value*	 module = NewObjectValue(interp);
	HashMap* map	= CoerceToHashMap(module);
	HashMapSet(map, "Database", _DbmsClass);
	HashMapSet(map, "Statement", _StmtClass);
	return module;
}
