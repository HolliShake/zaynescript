"[#jit:always]";
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

loop(3);

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

const ob = {a: 65};

class Jit {
    fn init(a,b) {
        this.a=a;
        this.b=b;
    }
}

import {Server} from "core:mongoose";

try {
    println([1,2,3, ...([4])]);
    //ok: (2).a  = 3;
    println(new Jit(2, 3,4), Server);
} catch (e) {
    println("ERROR", e);
}
