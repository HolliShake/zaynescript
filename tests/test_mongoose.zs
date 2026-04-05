import { println } from "core:io";
import { Server  } from "core:mongoose";

const app = new Server();

app.use(fn (req, res) { 
    /* middleware */
});

app.get("/hello", fn (req, res) {
    res.status(201).json({ received: "asdasdaw22" });
});

app.post("/api/data", fn (req, res) {
    println(req.body, "asdasda");
    res.status(201).json({ received: req.body });
});

app.listen(3001, fn (msg) {
    println(msg);
});