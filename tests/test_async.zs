import { println } from "core:io";

fn asyncFn() async {
    println("Called!!");
    return "Resolve me!";
}

fn callMe() async {
    println(await asyncFn());
    println(await asyncFn());
    println(await asyncFn());
    return 1;
}

println(callMe());
println("AUTO");