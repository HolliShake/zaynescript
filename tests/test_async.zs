import { println, decompile } from "core:io";

fn  topLevel() async {
    println("From top level");
    return "Hello";
}

fn asyncFn() async {
    {
        await topLevel();
        await topLevel();
    }
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

var cop = null;
const rej = callMeMaybe()
    .then(fn (v) {
        v + "2323";
    })
    .error(fn(e) {
        println(e);
    });


const fun =  fn() {
    try  {
        (fn() {
            try {
                const x = 2;
                println(x + "asdasdasd");
            }  catch (e) {
                println("internal", e);
            }
        })();
    } catch (e) {
        println("external", e);
    }

   
};

println(decompile(fun));

fun();