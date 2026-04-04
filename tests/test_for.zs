import { println, format } from "core:io";
 
var arr = [];

for (i:=0;i<5;i++) {
    arr.push(fn(j) {
        println(i + j);
    });
}

arr.each(fn(e,i) {
    e(i);
});

println("Done!");