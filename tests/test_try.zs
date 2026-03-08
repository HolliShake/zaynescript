
import { println } from "core:io";

const x = "asd";

try {
    2 + x;
    println("Not executed");
} catch (e) {
    println("Catch", e);
}

const obj = {
    value: 123
};

println(obj);
++obj["value"] + 23 - 1;
println(obj);

println(obj);
println(obj.value++ + 23 - 1);
println(obj, 2, 3, 2);