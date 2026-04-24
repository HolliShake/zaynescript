import { Database } from "core:mysql";
import { args, getPid } from "core:os";
import { println } from "core:io";

/*
  MySQL CRUD integration test.

  Usage:
  ./dist/zscript.exe --run ./tests/test_mysql.zs <host> <user> <password> <database> [port]

  Example:
  ./dist/zscript.exe --run ./tests/test_mysql.zs 127.0.0.1 root secret zscript_test 3306
*/

const argv = args();
if (argv.length() < 4) {
    println("Skipping MySQL test: provide <host> <user> <password> <database> [port]");
    println("Example: --run ./tests/test_mysql.zs 127.0.0.1 root secret zscript_test 3306");
} else {
    const host = argv[0];
    const user = argv[1];
    const password = argv[2];
    const database = argv[3];
    var port = 3306;
    if (argv.length() > 4) {
        port = argv[4];
    }

    const db = new Database({
        host: host,
        user: user,
        password: password,
        database: database,
        port: port
    });

    const suffix = "" + getPid();
    const table = "zscript_mysql_crud_" + suffix;
    const nameA = "alice_" + suffix;
    const nameB = "alice_updated_" + suffix;

    // CREATE
    db.exec("CREATE TABLE IF NOT EXISTS " + table + " (id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(128) NOT NULL)");

    // INSERT
    const ins = db.exec("INSERT INTO " + table + " (name) VALUES ('" + nameA + "')");
    assert ins.affectedRows >= 1, "mysql insert should affect at least one row";

    // READ
    const rows = db.query("SELECT id, name FROM " + table + " WHERE name = '" + nameA + "'");
    assert rows.length() == 1, "mysql select should return one row";

    // mysql.query currently maps columns by index keys "0", "1"
    const insertedId = rows[0]["0"];
    assert rows[0]["1"] == nameA, "mysql selected row should match inserted value";

    // UPDATE
    const upd = db.exec("UPDATE " + table + " SET name = '" + nameB + "' WHERE id = " + insertedId);
    assert upd.affectedRows == 1, "mysql update should affect one row";

    const updated = db.query("SELECT id, name FROM " + table + " WHERE id = " + insertedId);
    assert updated.length() == 1, "mysql select by id should return one row";
    assert updated[0]["1"] == nameB, "mysql row should reflect updated value";

    // DELETE
    const del = db.exec("DELETE FROM " + table + " WHERE id = " + insertedId);
    assert del.affectedRows == 1, "mysql delete should affect one row";

    const afterDelete = db.query("SELECT id FROM " + table + " WHERE id = " + insertedId);
    assert afterDelete.length() == 0, "mysql row should be deleted";

    db.exec("DROP TABLE IF EXISTS " + table);
    db.close();

    println("MySQL CRUD test passed:", table);
}
