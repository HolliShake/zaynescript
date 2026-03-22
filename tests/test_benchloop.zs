
import { println } from "core:io";

var arr = [];

for (i := 0;i < 100000;i++) {
    arr.push(i);
}

println(arr);

//real    0m0.376s
//user    0m0.226s
//sys     0m0.016s