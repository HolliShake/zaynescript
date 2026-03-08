import { println } from "core:io";



fn fact(n) {
    if (n <= 1) return 1;
    return n * fact(n - 1);
}

println(fact(5) + "Hello");