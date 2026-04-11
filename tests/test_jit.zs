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