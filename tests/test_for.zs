import { println, format } from "core:io";
 
var arr = [];

for (i, j, k := 0, 1, 2; i < 5; i++) {
    println(i, j, k);
    arr.push(fn(j) {
        println(i + j);
    });
}

arr.each(fn(e, i) => e(i));

println("Done!");

const add = fn(a, b) => a + b;

println(add(5, 10), __dir, __file, __line);

fn getName() {
    return __func;
}

println(getName());

