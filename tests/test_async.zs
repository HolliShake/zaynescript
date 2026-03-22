import { println } from "core:io";
import { getType, getUser } from "core:os";
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

println(Array, getUser(), getType());

println(10.56n, 2.2e2n);
println(100n + 2n, 5n / 2n, 5n * 5n, 1 << 2n, 4n >> 2, 3 > 2n);