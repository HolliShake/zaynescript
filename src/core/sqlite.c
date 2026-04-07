#include "./sqlite.h"

// ─── Internal binding property keys ──────────────────────────────────────────

#define PROP_DB_PTR		"__ptr"
#define PROP_STMT_PTR	"__stmt"
#define PROP_STMT_DB	"__db"
#define PROP_STMT_HEAD	"__statementHead"
#define PROP_STMT_NEXT	"__next"
#define PROP_STMT_PLUCK "__pluck"
#define PROP_STMT_BOUND "__bound"
#define PROP_STMT_CLASS "__StatementClass"

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
	Value* val = (Value*) HashMapGet(instance->Members, PROP_DB_PTR);
	return (sqlite3*) _ValueToPtr(val);
}

static void _SetDB(Interpreter* interp, ClassInstance* instance, sqlite3* db) {
	HashMapSet(instance->Members, PROP_DB_PTR, _PtrToValue(interp, db));
}

// ─── Statement handle helpers
// ─────────────────────────────────────────────────

static sqlite3_stmt* _GetStmt(ClassInstance* instance) {
	Value* val = (Value*) HashMapGet(instance->Members, PROP_STMT_PTR);
	return (sqlite3_stmt*) _ValueToPtr(val);
}

static void
_SetStmt(Interpreter* interp, ClassInstance* instance, sqlite3_stmt* stmt) {
	HashMapSet(instance->Members, PROP_STMT_PTR, _PtrToValue(interp, stmt));
}

static sqlite3* _GetStmtDB(ClassInstance* instance) {
	Value* val = (Value*) HashMapGet(instance->Members, PROP_STMT_DB);
	return (sqlite3*) _ValueToPtr(val);
}

// ─── Statement tracking (linked list, GC-visible)
// ─────────────────────────────
//
// Each Database instance keeps a singly-linked list of every Statement it has
// prepared, rooted at "_stmt_head" on the Database ClassInstance.  Each
// Statement stores its successor as "_next".
//
// Because both links are ordinary ZS Value members the GC traces them normally,
// keeping Statements alive as long as their originating Database is reachable.
//
// _DbClose walks this list and nulls out "_stmt" and "_db" on every node so
// that any script-level call made after close returns a clean error rather than
// dereferencing freed SQLite memory.  _StmtFinalize leaves "_next" intact so
// the walk still works on partially-finalized lists.

static void
_TrackStatement(Interpreter* interp, ClassInstance* db_cls, Value* stmt_val) {
	Value* head = (Value*) HashMapGet(db_cls->Members, PROP_STMT_HEAD);
	// Link new node → old head, then advance the head pointer.
	ClassInstance* si = CoerceToClassInstance(stmt_val);
	HashMapSet(si->Members, PROP_STMT_NEXT, head ? head : interp->Null);
	HashMapSet(db_cls->Members, PROP_STMT_HEAD, stmt_val);
}

static void _InvalidateStatements(Interpreter* interp, ClassInstance* db_cls) {
	Value* cur = (Value*) HashMapGet(db_cls->Members, PROP_STMT_HEAD);
	while (cur && !ValueIsNull(cur)) {
		ClassInstance* si	= CoerceToClassInstance(cur);
		Value*		   next = (Value*) HashMapGet(si->Members, PROP_STMT_NEXT);
		// Null both handles so any further method call on this Statement
		// immediately hits the "finalized or invalid" / "no database" guards.
		HashMapSet(si->Members, PROP_STMT_PTR, _PtrToValue(interp, NULL));
		HashMapSet(si->Members, PROP_STMT_DB, _PtrToValue(interp, NULL));
		cur = next;
	}
	HashMapSet(db_cls->Members, PROP_STMT_HEAD, interp->Null);
}

// ─── Per-statement flag helpers
// ───────────────────────────────────────────────

static bool _GetPluck(ClassInstance* cls) {
	Value* v = (Value*) HashMapGet(cls->Members, PROP_STMT_PLUCK);
	return v && ValueIsBool(v) && v->Value.I32;
}

static bool _GetBound(ClassInstance* cls) {
	Value* v = (Value*) HashMapGet(cls->Members, PROP_STMT_BOUND);
	return v && ValueIsBool(v) && v->Value.I32;
}

// ─── Integer column helper
// ────────────────────────────────────────────────
//
// SQLite stores integers as up to 64-bit signed values.  We read them with
// sqlite3_column_int64 to avoid truncation, then return a 32-bit interpreter
// integer when the value fits or a double otherwise.  Values above 2^53 will
// lose precision in the double representation; a native int64 value type in the
// interpreter would be needed to eliminate that entirely.

static Value* _IntColValue(Interpreter* interp, sqlite3_int64 ival) {
	if (ival >= INT32_MIN && ival <= INT32_MAX)
		return NewIntValue(interp, (int) ival);
	return NewNumValue(interp, (double) ival);
}

// ─── Row conversion
// ───────────────────────────────────────────────────────────

static Value* _StmtRowToObject(Interpreter* interp, sqlite3_stmt* stmt) {
	Value*	 row = NewObjectValue(interp);
	HashMap* map = CoerceToHashMap(row);
	int		 n	 = sqlite3_column_count(stmt);

	for (int i = 0; i < n; i++) {
		const char* colName = sqlite3_column_name(stmt, i);
		Value*		colVal;

		switch (sqlite3_column_type(stmt, i)) {
			case SQLITE_INTEGER:
				colVal = _IntColValue(interp, sqlite3_column_int64(stmt, i));
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
			case SQLITE_BLOB:
				// BLOBs have no representation in the current value model and
				// are returned as null.  To support them, add a byte-array
				// value type and bind it here via sqlite3_column_blob /
				// sqlite3_column_bytes.
				colVal = interp->Null;
				break;
			case SQLITE_NULL:
			default:
				colVal = interp->Null;
				break;
		}

		HashMapSet(map, (String) colName, colVal);
	}
	return row;
}

static Value* _ColValue(Interpreter* interp, sqlite3_stmt* stmt, int idx) {
	switch (sqlite3_column_type(stmt, idx)) {
		case SQLITE_INTEGER:
			return _IntColValue(interp, sqlite3_column_int64(stmt, idx));
		case SQLITE_FLOAT:
			return NewNumValue(interp, sqlite3_column_double(stmt, idx));
		case SQLITE3_TEXT:
			{
				const char* text = (const char*) sqlite3_column_text(stmt, idx);
				return NewStrValue(interp, text ? (String) text : "");
			}
		case SQLITE_BLOB:
			return interp->Null;
		case SQLITE_NULL:
		default:
			return interp->Null;
	}
}

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

static int _BindParam(sqlite3_stmt* stmt, int idx, Value* val) {
	if (ValueIsNull(val)) {
		return sqlite3_bind_null(stmt, idx);
	} else if (ValueIsInt(val)) {
		// Use bind_int64 so the interface is correct even when the interpreter
		// is later extended to a native 64-bit integer type.
		return sqlite3_bind_int64(stmt, idx, (sqlite3_int64) val->Value.I32);
	} else if (ValueIsNum(val)) {
		return sqlite3_bind_double(stmt, idx, val->Value.Num);
	} else if (ValueIsAnyNum(val)) {
		return sqlite3_bind_double(stmt, idx, CoerceToNum(val));
	} else if (ValueIsBool(val)) {
		return sqlite3_bind_int64(stmt, idx, (sqlite3_int64) val->Value.I32);
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

static Value* _BindParamList(Interpreter*  interp,
							 sqlite3_stmt* stmt,
							 sqlite3*	   db,
							 int		   count,
							 Value**	   params) {
	int pos = 1;

	for (int i = 0; i < count; i++) {
		Value* val = params[i];

		if (ValueIsObject(val)) {
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
	(void) argc;
	(void) arguments;
	// Instances are created internally by Database.prepare().
	// Constructing a Statement directly in script is not supported.
	return NewErrorValue(
		interp,
		"Statement cannot be constructed directly – use db.prepare()");
}

// stmt.run([...params]) -> { changes: int, lastInsertRowid: int|num }
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

	// Capture the error message BEFORE calling sqlite3_reset: reset may update
	// the connection's extended error state, clobbering the message we need.
	if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
		Value* e =
			NewErrorFValue(interp, "sqlite3_step: %s", sqlite3_errmsg(db));
		sqlite3_reset(stmt);
		return e;
	}

	// Read rowid and change count before reset (they survive reset, but reading
	// them here keeps the ordering explicit and easier to reason about).
	int			  changes = sqlite3_changes(db);
	sqlite3_int64 rowid	  = sqlite3_last_insert_rowid(db);

	sqlite3_reset(stmt);

	Value*	 result = NewObjectValue(interp);
	HashMap* map	= CoerceToHashMap(result);
	HashMapSet(map, "changes", NewIntValue(interp, changes));
	HashMapSet(map, "lastInsertRowid", _IntColValue(interp, rowid));
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
		// Capture error before reset.
		Value* e =
			NewErrorFValue(interp, "sqlite3_step: %s", sqlite3_errmsg(db));
		sqlite3_reset(stmt);
		return e;
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

	// Capture error before reset.
	if (rc != SQLITE_DONE) {
		Value* e =
			NewErrorFValue(interp, "sqlite3_step: %s", sqlite3_errmsg(db));
		sqlite3_reset(stmt);
		return e;
	}

	sqlite3_reset(stmt);
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
		HashMapSet(cls->Members, PROP_STMT_BOUND, interp->True);
	}

	return arguments[0];
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
	HashMapSet(cls->Members,
			   PROP_STMT_PLUCK,
			   on ? interp->True : interp->False);
	return arguments[0];
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
		// Built without SQLITE_ENABLE_COLUMN_METADATA; origin info unavailable.
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
// The "_next" tracking link is intentionally left intact so that a subsequent
// _DbClose walk still terminates correctly on a partially-finalized list.
static Value* _StmtFinalize(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls	= CoerceToClassInstance(arguments[0]);
	sqlite3_stmt*  stmt = _GetStmt(cls);
	if (!stmt)
		return interp->Null;
	sqlite3_finalize(stmt);
	HashMapSet(cls->Members, "_stmt", _PtrToValue(interp, NULL));
	HashMapSet(cls->Members, "_db", _PtrToValue(interp, NULL));
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
	// Initialise the GC-visible statement tracking list (see _TrackStatement).
	HashMapSet(cls->Members, PROP_STMT_HEAD, interp->Null);
	return interp->Null;
}

// db.close() -> null
//
// Invalidates every live Statement instance prepared on this connection by
// nulling out their internal handles (preventing use-after-free), then
// finalizes any remaining C-level statements and closes the database.
static Value* _DbClose(Interpreter* interp, int argc, Value** arguments) {
	(void) argc;
	ClassInstance* cls = CoerceToClassInstance(arguments[0]);
	sqlite3*	   db  = _GetDB(cls);
	if (db) {
		// Walk the GC-visible linked list and null _stmt/_db on every Statement
		// so that subsequent script-level calls produce a clean error instead
		// of dereferencing freed SQLite memory.
		_InvalidateStatements(interp, cls);

		// Drain any C-level statements that survive (e.g. those created outside
		// this binding) so sqlite3_close() completes immediately.
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
// Compiles sql into a reusable Statement and registers it in the database's
// tracking list so that db.close() can safely invalidate it.
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

	Class* dbCls		= CoerceToUserClass(cls->Proto);
	Value* stmtClassVal = ClassGetMember(dbCls, PROP_STMT_CLASS, true);
	if (!stmtClassVal) {
		sqlite3_finalize(stmt);
		return NewErrorValue(interp, "internal: Statement class not found");
	}

	ClassInstance* stmtInst = CreateClassInstance(stmtClassVal);
	_SetStmt(interp, stmtInst, stmt);
	HashMapSet(stmtInst->Members, PROP_STMT_DB, _PtrToValue(interp, db));

	Value* stmtVal = NewClassInstanceValue(interp, stmtInst);

	// Register in the database's tracking list so _DbClose can invalidate it.
	_TrackStatement(interp, cls, stmtVal);

	return stmtVal;
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

// LoadCoreSqlite is called once per module import.  The Statement class is
// built fresh on each call and stored exclusively as a GC-visible static member
// of Database — no interpreter-global C statics, no GC-invisible raw pointers.
Value* LoadCoreSqlite(Interpreter* interp) {
	Value* stmtClass = _BuildClass(interp, "Statement", _StmtClassMethods);
	Value* dbClass	 = _BuildClass(interp, "Database", _DbClassMethods);

	// Store Statement class as a static (class-level) member of Database so
	// the GC keeps it alive as long as the Database class is reachable, and
	// so _DbPrepare can retrieve it without touching any C global.
	ClassDefineMemberByString(CoerceToUserClass(dbClass),
							  PROP_STMT_CLASS,
							  stmtClass,
							  true);

	Value*	 module = NewObjectValue(interp);
	HashMap* map	= CoerceToHashMap(module);
	HashMapSet(map, "Database", dbClass);
	HashMapSet(map, "Statement", stmtClass);
	return module;
}