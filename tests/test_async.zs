import { println, decompile } from "core:io";

fn topLevel() async {
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
        v + "xx";
    })
    .error(fn(e) {
        println("Catch", e);
    });


const fun = fn() {
    while (1) {
        try  {
            (fn() {
                try {
                    const x = 2;
                    println(x + "asdasdasd");
                }  catch (e) {
                    println("internal", e);
                }
            })();

            if (x:=1;x) {
                if (y:=x+1;y) {
                    println(">>>>>>>>>>>>|||", x  + y);
                    break;
                }
            }
            
        } catch (e) {
            println("external", e);
        }
    }
   
};


println(decompile(fun));

