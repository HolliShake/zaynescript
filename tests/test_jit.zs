import { println }  from "core:io";

var i = 0;
while (i < 120) {
   i++;
}

println(i);

const add = fn(a, b) {
    return a + b;
};

const loop = fn(n) {
    for (i:=0;i<n;i++) {
        println(i);
    }
};


println(add(5, 16));

loop(300);

try {
    try {
        2 + add;
    } catch(e) {
        println("inner>>",e);
    }
    2 + add;
} catch (e) {
    println("outer>>",e);
}