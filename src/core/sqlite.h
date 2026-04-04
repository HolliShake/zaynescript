/**
 * @file sqlite.h
 * @brief Core SQLite module interface
 *
 * Exposes a Database class and a Statement class that wrap the
 * SQLite3 amalgamation bundled in sqlite/sqlite3.h.
 *
 * Usage from the interpreted language:
 *
 *   import sqlite from "sqlite";
 *
 *   const db = new sqlite.Database("path/to/db.sqlite");
 *   db.exec("CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, name
 * TEXT)");
 *
 *   // Parameterised INSERT, returns {lastInsertRowid, changes}
 *   db.run("INSERT INTO t (name) VALUES (?)", "Alice");
 *
 *   // SELECT, returns array of row objects
 *   const rows = db.query("SELECT * FROM t WHERE name = ?", "Alice");
 *
 *   // Prepared statement
 *   const stmt = db.prepare("SELECT * FROM t");
 *   while (stmt.step()) {
 *       const row = stmt.getRow();
 *   }
 *   stmt.finalize();
 *
 *   db.close();
 */

#include "../../sqlite/sqlite3.h"
#include "../function.h"
#include "../global.h"
#include "../value.h"

#ifndef CORE_SQLITE_H
#	define CORE_SQLITE_H

/**
 * @brief Loads the core SQLite module.
 *
 * Returns an object with two keys:
 *   - "Database"  – the Database class constructor
 *   - "Statement" – the Statement class constructor
 *
 * @param  interpreter The interpreter instance
 * @return Value* Pointer to the module object, or NULL on failure
 */
Value* LoadCoreSqlite(Interpreter* interpreter);

#endif /* CORE_SQLITE_H */
