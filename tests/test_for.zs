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
