import { println } from "core:io";
import { Server  } from "core:mongoose";
import { Database } from "core:sqlite";

// ── Database setup ────────────────────────────────────────────────────────────
const db = new Database(":memory:");
db.exec("CREATE TABLE IF NOT EXISTS todos (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT NOT NULL, completed INTEGER NOT NULL DEFAULT 0)");

// ── Prepared statements ───────────────────────────────────────────────────────
const stmtGetAll    = db.prepare("SELECT * FROM todos");
const stmtGetOne    = db.prepare("SELECT * FROM todos WHERE id = ?");
const stmtInsert    = db.prepare("INSERT INTO todos (title, completed) VALUES (@title, @completed)");
const stmtUpdate    = db.prepare("UPDATE todos SET title = @title, completed = @completed WHERE id = @id");
const stmtDelete    = db.prepare("DELETE FROM todos WHERE id = ?");

// ── HTTP server ───────────────────────────────────────────────────────────────
const app = new Server();

app.use(fn (req, res) {
    /* middleware – runs before every route */
});

// GET /todos – list all todos
app.get("/todos", fn (req, res) {
    const rows = stmtGetAll.all();
    res.status(200).json(rows);
});

// GET /todos/:id – get a single todo
app.get("/todos/:id", fn (req, res) {
    const todo = stmtGetOne.get(req.params.id);
    if (todo == null) {
        res.status(404).json({ error: "Todo not found" });
    } else {
        res.status(200).json(todo);
    }
});

// POST /todos – create a new todo
app.post("/todos", fn (req, res) async {
    const body  = req.body;
    const info  = stmtInsert.run({ title: body.title, completed: 0 });
    const todo  = stmtGetOne.get(info.lastInsertRowid);
    res.status(201).json(todo);
});

// PUT /todos/:id – update a todo (title and/or completed)
app.put("/todos/:id", fn (req, res) {
    const existing = stmtGetOne.get(req.params.id);
    if (existing == null) {
        res.status(404).json({ error: "Todo not found" });
    } else {
        const body = req.body;
        stmtUpdate.run({
            id:        req.params.id,
            title:     body.title,
            completed: body.completed
        });
        const updated = stmtGetOne.get(req.params.id);
        res.status(200).json(updated);
    }
});

// DELETE /todos/:id – remove a todo
app.delete("/todos/:id", fn (req, res) {
    const existing = stmtGetOne.get(req.params.id);
    if (existing == null) {
        res.status(404).json({ error: "Todo not found" });
    } else {
        stmtDelete.run(req.params.id);
        res.status(200).json({ message: "Todo deleted" });
    }
});

app.listen(3001, fn (msg) {
    println(msg);
});