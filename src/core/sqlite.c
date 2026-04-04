#include "./sqlite.h"

#include "../../sqlite/sqlite3.h"

// ─── Module-level handle for the Statement class ─────────────────────────────
// Kept alive through the Database class's static "_StmtClass" member.
static Value* _StmtClass = NULL;

// ─── Opaque-pointer encode / decode ──────────────────────────────────────────

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

// ─── Per-statement flag helpers
// ───────────────────────────────────────────────

// Returns true when pluck mode is active (rows → first-column value only).
static bool _GetPluck(ClassInstance* cls) {
	Value* v = (Value*) HashMapGet(cls->Members, "_pluck");
	return v && ValueIsBool(v) && v->Value.I32;
}

// Returns true after stmt.bind() has permanently locked the parameters.
static bool _GetBound(ClassInstance* cls) {
	Value* v = (Value*) HashMapGet(cls->Members, "_bound");
	return v && ValueIsBool(v) && v->Value.I32;
}

// ─── Row conversion
// ───────────────────────────────────────────────────────────

// Convert the current statement row to a ZS object keyed by column name.
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

// Return the value of a single statement column (used by pluck mode).
static Value* _ColValue(Interpreter* interp, sqlite3_stmt* stmt, int idx) {
	switch (sqlite3_column_type(stmt, idx)) {
		case SQLITE_INTEGER:
			return NewIntValue(interp, sqlite3_column_int(stmt, idx));
		case SQLITE_FLOAT:
			return NewNumValue(interp, sqlite3_column_double(stmt, idx));
		case SQLITE3_TEXT:
			{
				const char* text = (const char*) sqlite3_column_text(stmt, idx);
				return NewStrValue(interp, text ? (String) text : "");
			}
		case SQLITE_NULL:
		default:
			return interp->Null;
	}
}

// Build a row value, respecting pluck mode.
static Value* _MakeRow(Interpreter* interp, sqlite3_stmt* stmt, bool pluck) {
	if (pluck) {
		if (sqlite3_column_count(stmt) == 0)
			return interp->Null;
		return _ColValue(interp, stmt, 0);
	}
	return _StmtRowToObject(interp, stmt);
}

// ─── Bind helpers
// ─────────────────────────────────────────────────────────────

// Bind a single ZS value to a 1-based SQLite parameter index.
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

// Bind a call-argument list to a prepared statement (better-sqlite3 semantics).
//
// Rules:
//   • Primitive args → bound positionally to the next anonymous "?" slot.
//   • Object args    → bound by name: for every @name / :name / $name
//                      placeholder, look up "name" in the ZS object's HashMap
//                      and bind the result (null if absent).
//
// Positional and named styles can be mixed just as in better-sqlite3.
// Returns NULL on success or an error Value on failure.
static Value* _BindParamList(Interpreter*  interp,
							 sqlite3_stmt* stmt,
							 sqlite3*	   db,
							 int		   count,
							 Value**	   params) {
	int pos = 1;  // next positional slot

	for (int i = 0; i < count; i++) {
		Value* val = params[i];

		if (ValueIsObject(val)) {
			// Named-parameter path: walk SQLite's parameter list and look up
			// each named slot against the ZS object's HashMap.
			HashMap* map	 = CoerceToHashMap(val);
			int		 nparams = sqlite3_bind_parameter_count(stmt);

			for (int j = 1; j <= nparams; j++) {
				const char* pname = sqlite3_bind_parameter_name(stmt, j);
				if (!pname)
					continue;  // anonymous ? – handled positionally
				const char* key	  = pname + 1;	// strip leading @, :, or $
				Value*		entry = (Value*) HashMapGet(map, (String) key);
				int rc = _BindParam(stmt, j, entry ? entry : interp->Null);
				if (rc != SQLITE_OK)
					return NewErrorFValue(interp,
										  "sqlite3_bind '%s': %s",
										  pname,
										  db ? sqlite3_errmsg(db)
											 : "unknown error");
			}
		} else {
			// Positional path.
			int rc = _BindParam(stmt, pos++, val);
			if (rc != SQLITE_OK)
				return NewErrorFValue(interp,
									  "sqlite3_bind [%d]: %s",
									  pos - 1,
									  db ? sqlite3_errmsg(db)
										 : "unknown error");
		}
	}
	return NULL;
}

// ─── Execution setup helper
// ───────────────────────────────────────────────────

// Reset the statement and, when temporary params are provided, clear permanent
// bindings and apply the new ones.  Errors if the statement is permanently
// bound but params have also been supplied for this call. Returns NULL on
// success or an error Value.
static Value* _PrepareExec(Interpreter*	  interp,
						   ClassInstance* cls,
						   sqlite3_stmt*  stmt,
						   sqlite3*		  db,
						   int			  argc,
						   Value**		  arguments,
						   const char*	  method) {
	sqlite3_reset(stmt);
	if (argc > 1) {
		if (_GetBound(cls))
			return NewErrorFValue(
				interp,
				"%s: this statement has permanently bound parameters – "
				"do not pass arguments when using bind()",
				method);
		sqlite3_clear_bindings(stmt);
		return _BindParamList(interp, stmt, db, argc - 1, &arguments[1]);
	}
	return NULL;
}

// =============================================================================
// Statement class
// =============================================================================

static Value* _StmtInit(Interpreter* interp, int argc, Value** arguments) {
	// Instances are created internally by Database.prepare(); init is a no-op.
	(void) argc;
	(void) arguments;
	return interp->Null;
}

// stmt.run([...params]) -> { changes: int, lastInsertRowid: num }
//
// Executes a DML prepared statement and returns an info object describing the
// changes made.  Parameters are bound only for this call unless stmt.bind() has
// already permanently locked them.
static Value* _StmtRun(Interpreter* interp, int argc, Value** arguments) {
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	sqlite3*	   db	= _GetStmtDB(cls);
	if (!stmt)
		return NewErrorValue(interp, "Statement is finalized or invalid");
	if (!db)
		return NewErrorValue(interp, "Statement has no associated database");

	Value* err = _PrepareExec(interp, cls, stmt, db, argc, arguments, "run");
	if (err)
		return err;

	int rc = sqlite3_step(stmt);
	sqlite3_reset(stmt);

	if (rc != SQLITE_DONE && rc != SQLITE_ROW)
		return NewErrorFValue(interp, "sqlite3_step: %s", sqlite3_errmsg(db));

	Value*	 result = NewObjectValue(interp);
	HashMap* map	= CoerceToHashMap(result);
	HashMapSet(map, "changes", NewIntValue(interp, sqlite3_changes(db)));
	HashMapSet(map,
			   "lastInsertRowid",
			   NewNumValue(interp, (double) sqlite3_last_insert_rowid(db)));
	return result;
}

// stmt.get([...params]) -> rowObject | null
//
// Executes the query and returns the first row as an object, or null when no
// rows match.
static Value* _StmtGet(Interpreter* interp, int argc, Value** arguments) {
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	sqlite3*	   db	= _GetStmtDB(cls);
	if (!stmt)
		return NewErrorValue(interp, "Statement is finalized or invalid");
	if (!db)
		return NewErrorValue(interp, "Statement has no associated database");

	bool   pluck = _GetPluck(cls);
	Value* err	 = _PrepareExec(interp, cls, stmt, db, argc, arguments, "get");
	if (err)
		return err;

	int	   rc	  = sqlite3_step(stmt);
	Value* result = interp->Null;

	if (rc == SQLITE_ROW) {
		result = _MakeRow(interp, stmt, pluck);
	} else if (rc != SQLITE_DONE) {
		sqlite3_reset(stmt);
		return NewErrorFValue(interp, "sqlite3_step: %s", sqlite3_errmsg(db));
	}

	sqlite3_reset(stmt);
	return result;
}

// stmt.all([...params]) -> array of rowObjects
//
// Executes the query and collects every row into an array.
static Value* _StmtAll(Interpreter* interp, int argc, Value** arguments) {
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	sqlite3*	   db	= _GetStmtDB(cls);
	if (!stmt)
		return NewErrorValue(interp, "Statement is finalized or invalid");
	if (!db)
		return NewErrorValue(interp, "Statement has no associated database");

	bool   pluck = _GetPluck(cls);
	Value* err	 = _PrepareExec(interp, cls, stmt, db, argc, arguments, "all");
	if (err)
		return err;

	Value* rows = NewArrayValue(interp);
	Array* arr	= (Array*) rows->Value.Opaque;
	int	   rc;

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		ArrayPush(arr, _MakeRow(interp, stmt, pluck));

	sqlite3_reset(stmt);

	if (rc != SQLITE_DONE)
		return NewErrorFValue(interp, "sqlite3_step: %s", sqlite3_errmsg(db));

	return rows;
}

// stmt.bind([...params]) -> this
//
// Permanently binds parameters to the statement for its entire lifetime.
// Bindings survive sqlite3_reset(), so the statement can be re-executed without
// rebinding.  After calling bind() you must NOT pass params to run/get/all.
// Supports the same positional / named-object conventions as run/get/all.
// Returns `this` for chaining: db.prepare(sql).bind(x).get()
static Value* _StmtBind(Interpreter* interp, int argc, Value** arguments) {
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	sqlite3*	   db	= _GetStmtDB(cls);
	if (!stmt)
		return NewErrorValue(interp, "Statement is finalized or invalid");

	if (argc > 1) {
		sqlite3_clear_bindings(stmt);
		Value* err = _BindParamList(interp, stmt, db, argc - 1, &arguments[1]);
		if (err)
			return err;
		HashMapSet(cls->Members, "_bound", interp->True);
	}

	return arguments[0];  // return `this` for chaining
}

// stmt.pluck([bool]) -> this
//
// When on (default), get() / all() return the value of the first column instead
// of a full row object.  Pass false to turn off.  Chainable.
static Value* _StmtPluck(Interpreter* interp, int argc, Value** arguments) {
	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	bool		   on  = true;
	if (argc > 1 && ValueIsBool(arguments[1]))
		on = (arguments[1]->Value.I32 != 0);
	HashMapSet(cls->Members, "_pluck", on ? interp->True : interp->False);
	return arguments[0];  // chainable
}

// stmt.columns() -> array of { name, column, table, database, type }
//
// Returns column metadata matching the better-sqlite3 shape.
// column / table / database are populated only when SQLite is compiled with
// SQLITE_ENABLE_COLUMN_METADATA; otherwise they are null.
static Value* _StmtColumns(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	if (!stmt)
		return NewArrayValue(interp);

	Value* result = NewArrayValue(interp);
	Array* arr	  = (Array*) result->Value.Opaque;
	int	   n	  = sqlite3_column_count(stmt);

	for (int i = 0; i < n; i++) {
		Value*	 col = NewObjectValue(interp);
		HashMap* map = CoerceToHashMap(col);

		const char* name = sqlite3_column_name(stmt, i);
		HashMapSet(map, "name", NewStrValue(interp, name ? (String) name : ""));

#ifdef SQLITE_ENABLE_COLUMN_METADATA
		const char* originName = sqlite3_column_origin_name(stmt, i);
		HashMapSet(map,
				   "column",
				   originName
					   ? (Value*) NewStrValue(interp, (String) originName)
					   : interp->Null);

		const char* tableName = sqlite3_column_table_name(stmt, i);
		HashMapSet(map,
				   "table",
				   tableName ? (Value*) NewStrValue(interp, (String) tableName)
							 : interp->Null);

		const char* dbName = sqlite3_column_database_name(stmt, i);
		HashMapSet(map,
				   "database",
				   dbName ? (Value*) NewStrValue(interp, (String) dbName)
						  : interp->Null);
#else
		HashMapSet(map, "column", interp->Null);
		HashMapSet(map, "table", interp->Null);
		HashMapSet(map, "database", interp->Null);
#endif

		const char* declType = sqlite3_column_decltype(stmt, i);
		HashMapSet(map,
				   "type",
				   declType ? (Value*) NewStrValue(interp, (String) declType)
							: interp->Null);

		ArrayPush(arr, col);
	}
	return result;
}

// stmt.finalize() -> null
//
// Destroys the prepared statement and frees its resources.  Safe to call
// multiple times.  After finalization the statement must not be used.
static Value* _StmtFinalize(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	if (!stmt)
		return interp->Null;
	sqlite3_finalize(stmt);
	HashMapSet(cls->Members, "_stmt", _PtrToValue(interp, NULL));
	return interp->Null;
}

static ModuleFunction _StmtClassMethods[] = {
	{ .Name		 = CONSTRUCTOR_NAME,
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _StmtInit,
	  .Value	 = NULL },
	{ .Name		 = "run",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _StmtRun,
	  .Value	 = NULL },
	{ .Name		 = "get",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _StmtGet,
	  .Value	 = NULL },
	{ .Name		 = "all",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _StmtAll,
	  .Value	 = NULL },
	{ .Name		 = "bind",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _StmtBind,
	  .Value	 = NULL },
	{ .Name		 = "pluck",
	  .Argc		 = VARARG,
	  .CFunction = (NativeFunctionCallback) _StmtPluck,
	  .Value	 = NULL },
	{ .Name		 = "columns",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StmtColumns,
	  .Value	 = NULL },
	{ .Name		 = "finalize",
	  .Argc		 = 1,
	  .CFunction = (NativeFunctionCallback) _StmtFinalize,
	  .Value	 = NULL },
	{ .Name = NULL }
};

// =============================================================================
// Database class
// =============================================================================

// new Database(path?) -> Database
//
// Opens (or creates) an SQLite database at path.  Pass ":memory:" or omit the
// argument for a private in-memory database.
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

// db.close() -> null
//
// Finalizes every open prepared statement on this connection (including any
// one-shot Statement objects that were never explicitly finalized), then closes
// the database.  This ensures sqlite3_close() completes immediately and ASAN
// does not report per-connection allocations as leaks.
static Value* _DbClose(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	sqlite3*	   db  = _GetDB(cls);
	if (db) {
		// Drain all remaining prepared statements so the close is not deferred.
		sqlite3_stmt* s;
		while ((s = sqlite3_next_stmt(db, NULL)) != NULL)
			sqlite3_finalize(s);
		sqlite3_close(db);
		_SetDB(interp, cls, NULL);
	}
	return interp->Null;
}

// db.exec(sql) -> null
//
// Executes one or more semicolon-separated SQL statements without parameter
// binding.  Ideal for DDL and multi-statement migration scripts.
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

// db.prepare(sql) -> Statement
//
// Compiles sql into a reusable Statement.  Call stmt.finalize() when done
// (or rely on db.close() with sqlite3_close_v2 to clean up).
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

	// Retrieve the Statement class stored as a static member of Database.
	Class* dbCls		= CoerceToUserClass(cls->Proto);
	Value* stmtClassVal = ClassGetMember(dbCls, "_StmtClass", true);
	if (!stmtClassVal) {
		sqlite3_finalize(stmt);
		return NewErrorValue(interp, "internal: Statement class not found");
	}

	ClassInstance* stmtInst = CreateClassInstance(stmtClassVal);
	_SetStmt(interp, stmtInst, stmt);
	// Cache the db handle so Statement methods can retrieve it for error
	// messages and for sqlite3_changes / sqlite3_last_insert_rowid.
	HashMapSet(stmtInst->Members, "_db", _PtrToValue(interp, db));

	return NewClassInstanceValue(interp, stmtInst);
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
	{ .Name		 = "prepare",
	  .Argc		 = 2,
	  .CFunction = (NativeFunctionCallback) _DbPrepare,
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
	if (!_StmtClass)
		_StmtClass = _BuildClass(interp, "Statement", _StmtClassMethods);

	Value* dbClass = _BuildClass(interp, "Database", _DbClassMethods);

	// Store Statement class as a static (class-level) member of Database so
	// the GC keeps it alive as long as the Database class is reachable.
	ClassDefineMemberByString(CoerceToUserClass(dbClass),
							  "_StmtClass",
							  _StmtClass,
							  true);

	Value*	 module = NewObjectValue(interp);
	HashMap* map	= CoerceToHashMap(module);
	HashMapSet(map, "Database", dbClass);
	HashMapSet(map, "Statement", _StmtClass);
	return module;
}
