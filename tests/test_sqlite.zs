import { Database } from "core:sqlite";
import { println } from "core:io";

// Database class
const db = new Database("mydb.sqlite"); // or ":memory:"
db.exec("CREATE TABLE IF NOT EXISTS t (id INTEGER PRIMARY KEY, name TEXT)");
db.run("INSERT INTO t (name) VALUES (?)", "Alice");    // → {lastInsertRowid, changes}
const rows = db.query("SELECT * FROM t WHERE name = ?", "Alice"); // → array of objects
db.lastInsertRowid();
db.changes();

// Prepared statements
const stmt = db.prepare("SELECT * FROM t WHERE id >= ?");
stmt.bind(1, 1); // 1-based index
while (stmt.step()) {          // true = row available, false = done
    const row = stmt.getRow(); // → object with column names as keys
    println(row);
}
stmt.reset();     // rewind for re-use
stmt.bind(1, 1);
stmt.columnCount();
stmt.columnName(0);
stmt.finalize();

db.close();