import { Server } from "core:mongoose";
import { println } from "core:io";

const app = new Server();

app.use(fn (req, res) { /* middleware */ });

app.get("/hello", fn (req, res) async {
    res.status(201).json({ received: "asdasdaw22" });
});

app.post("/api/data", fn  (req, res) {
    println(req.body);
    res.status(201).json({ received: req.body });
});

app.listen(3001, fn (msg) {
    println(msg);
});