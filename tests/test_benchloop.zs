import { println } from "core:io";

var arr = [];

for (i := 0;i < 1000000;i++) {
    arr.push(i);
}

println(arr);

//real    0m3.715s
//user    0m1.331s
//sys     0m0.161s