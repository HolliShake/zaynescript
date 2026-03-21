import { println } from "core:io";
import { type, getuser } from "core:os";
import { Array } from "core:Array";

fn A() async {
    return "Hello";
}

fn B() async {
    A();
    return "World";
}

println(B());


const g = [2,4,6].each(fn(i, e) {
    return  e + i;
});

Array.sum = fn(arr) {
    local total = 0;
    arr.each(fn(i, e) {
        total += e;
    });
    return total;
};

println(g, Array.sum(g), g.length(), g.pop());

println([1,2,3,4,5,6,7,8,9].keep(fn(i, e) {
    return e % 2 != 0;
}));

println(Array, getuser(), type());


println(1000000000000000000000000000000n);