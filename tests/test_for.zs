import { println } from "core:io";
 
const arr = [];

for (x := 0; x < 10000; x++) {
    //println(x);
    arr.push(x);
}

println(arr);
