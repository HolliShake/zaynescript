import { println }  from "core:io";

1 + 2;

const a = 2, b = 2, c = 100;

fn fact(n) {
    if (n <= 1) {
        return 1;
    }
    return n * fact(n - 1);
}

println(">>", fact(5));
println(">>", fact(5));

try {
    "hello" + a;
} catch(e)  {
    println("error>", e);
}

var i = 0;
while (i < 120000) {
    println(i++);
}

//JIT  : min 381 | max 433
//nojit: min 396 | max 455