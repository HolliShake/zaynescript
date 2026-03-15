import { println } from "core:io";




fn A() async {
    return "Hello";
}

fn B() async {
    A();
    return "World";
}

println(B());


[2,4,6].each(fn(i, e) {
    e + "asd";
});