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

callMeMaybe()
    .then(fn (v) {
        v + "xx";
    })
    .error(fn(e) {
        println("Catch", e);
    });

callMeMaybe();

const fun = fn() {
    while (1) {
        try {
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

fn asyncReject() async {
    raise "awaited-error";
}

var awaitErrCaughtCount = 0;
var awaitErrSuccessCount = 0;
var awaitErrLastValue = "";

fn awaitErrorInTryCatchStress() async {
    awaitErrCaughtCount = 0;
    awaitErrSuccessCount = 0;
    awaitErrLastValue = "";

    // JS-like behavior: awaiting a rejected async should throw into catch.
    for (i := 0; i < 200; i++) {
        try {
            await asyncReject();
            awaitErrSuccessCount++;
        } catch (e) {
            awaitErrCaughtCount++;
            awaitErrLastValue = e;
        }
    }

    assert awaitErrCaughtCount == 200, "async/await try-catch: all awaited errors are caught";
    assert awaitErrSuccessCount == 0, "async/await try-catch: no success path after await reject";
    assert awaitErrLastValue != "", "async/await try-catch: caught error value is present";

    return true;
}

awaitErrorInTryCatchStress().then(fn(v) {
    assert v, "async/await try-catch stress completed";
});
