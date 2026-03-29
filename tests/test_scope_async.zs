import { println } from "core:io";

println("Hello!");


fn nest(n) async {
    println("2", n);
}

fn callMe() async {
    if (x := true; x) {
        println("1", x);
        await nest(x);
    }
    return "Done!";
}

const prm = callMe()
    .then(fn(n) {
        println("foc>>", n);
    });