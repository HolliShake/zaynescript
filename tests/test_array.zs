import { println } from "core:io";

println("Testing...");

const arr = [1,2,3, ...[4,5,6], {a: 2}];

println(arr);

arr.each(println);

arr.each(fn(e, i) {
    println("e is", e);

    try {
        e + "22";
    } catch (err) {
        println(err);
    }
});