import { println } from "core:io";

fn f1() {
    let v1 = "V1";
    return fn() {
        let v2 = "v2";
        return fn() {
            let v3 = "v3";
            return fn() {
                println(v1, v2, v3);
            };
        };
    };
}

println(">>", f1()()()());

// Test nested closures
fn outer() {
    let outerVar = "outer";
    return fn() {
        let middleVar = "middle";
        return fn() {
            return outerVar + "-" + middleVar + "-inner";
        };
    };
}

const nestedResult = outer()()();
println("\nNested closure test:");
println("Result:", nestedResult);
