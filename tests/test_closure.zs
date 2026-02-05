import { println } from "core:io";

fn f1() {
    let v1 = "V1";
    println("Running f1");
    return fn() {
        println("Running f1.fn1");
        let v2 = "v2";
        return fn() {
            println("Running f1.fn1.fn2");
            let v3 = "v3";
            return fn() {
                println("Running f1.fn2.fn3");
                println(v1, v2, v3);
            };
        };
    };
}

println(">>", f1()()()());

