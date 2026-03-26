const println = console.log;

async function topLevel() {
    println("From top level");
    return "Hello";
}

async function asyncFn() {
    await topLevel();
    println("Called!!");
    return "Resolve me!";
}

async function callMe() {
    println(await asyncFn());
    println(await asyncFn());
    println(await asyncFn());
    return 1;
}

println(callMe());
println("AUTO");
println(callMe());