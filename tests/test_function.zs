

import { println } from "core:io";


println("Mama!");

fn prom() async {
    return "Ok";
}

fn wait() async {
    const res = await prom();
    println(">>", res);
    return 2;
}

println("Outside:", wait().then(fn(data) {
    println("Resolved:", data);
}));