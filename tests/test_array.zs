import { println } from "core:io";

println("Testing...");

const arr = [1,2,3, ...[4,5,6], {a: 2}];

println(arr);

arr.each(println);