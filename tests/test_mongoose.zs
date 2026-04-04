import { Server } from "core:mongoose";
import { println } from "core:io";

const app = new Server();

app.use(fn  (req, res) { /* middleware */ });

app.get("/hello", fn (req, res) {
    res.status(201).json({ received: "asdasd" });
});

app.post("/api/data", fn  (req, res) {
    res.status(201).json({ received: req.body });
});

app.listen(5043, fn (msg) {
    println(msg);
});