import { println } from "core:io";

fn  topLevel() async {
    println("From top level");
    return "Hello";
}

fn asyncFn() async {
    await topLevel();
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
println(callMe());

fn awaitable() async {
    return "Hola!";
}

const v = awaitable()
    .then(fn(v) {
        println(">>>>>>>>>>>>>>> From then", v);
        return 4;
    })
    .then(fn(v) {
        println("waiting for>>", v);
        return "foocers";
    })
    .then(println);

println(">>", v);



fn toCall() async {
    return 3;
}

fn callMeMaybe() async {
    const r = await toCall();
    println(r,r,r,r,1000);
    return 1;
}


callMeMaybe();