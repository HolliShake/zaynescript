import { Database } from "core:sqlite";
import { println } from "core:io";

// ── Open / schema ────────────────────────────────────────────────────────────
const db = new Database("mydb.sqlite"); // file; use ":memory:" for in-memory
db.exec("CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, name TEXT)");

// ── DML via stmt.run() ───────────────────────────────────────────────────────
// run() returns { changes, lastInsertRowid }
const info = db.prepare("INSERT INTO t (name) VALUES (?)").run("Alice");
println(info.changes);          // 1
println(info.lastInsertRowid);  // rowid of the inserted row

// ── SELECT all rows via stmt.all() ───────────────────────────────────────────
const rows = db.prepare("SELECT * FROM t WHERE name = ?").all("Alice");
rows.each(fn(row, i) {
    println(row); // { id: ..., name: "Alice" }
});

// ── SELECT first row via stmt.get() ─────────────────────────────────────────
const first = db.prepare("SELECT * FROM t WHERE id = ?").get(1);
println(first); // { id: 1, name: "Alice" } or null if not found

// ── Reusable prepared statement ──────────────────────────────────────────────
const stmt = db.prepare("SELECT * FROM t WHERE id >= ?");

// all() with per-call params
const all = stmt.all(1);
all.each(fn(row, i) { println(row); });

// get() with per-call params
const one = stmt.get(1);
println(one);

// ── stmt.columns() ───────────────────────────────────────────────────────────
// Returns array of { name, column, table, database, type }
const cols = stmt.columns();
println(cols.length()); // 2 (id, name)
cols.each(fn(c, i) { println(c.name); });

// ── pluck mode – returns first-column value instead of full row object ───────
const ids = stmt.pluck().all(1);
ids.each(fn(id, i) { println(id); }); // prints each id value
stmt.pluck(false); // turn pluck back off

// ── Permanent binding via stmt.bind() ────────────────────────────────────────
// bind() locks params for the lifetime of the statement; chainable.
const bound = db.prepare("SELECT * FROM t WHERE name = ?").bind("Alice");
println(bound.get()); // { id: 1, name: "Alice" }
bound.finalize();

// ── Named parameters (@name / :name / $name) ─────────────────────────────────
db.exec("CREATE TABLE IF NOT EXISTS people (id INTEGER PRIMARY KEY, name TEXT, age INTEGER)");
db.prepare("INSERT INTO people (name, age) VALUES (@name, @age)")
    .run({ name: "Bob", age: 30 });
const person = db.prepare("SELECT * FROM people WHERE name = @name")
    .get({ name: "Bob" });
println(person); // { id: 1, name: "Bob", age: 30 }

// ── Cleanup ───────────────────────────────────────────────────────────────────
stmt.finalize();
db.close();
