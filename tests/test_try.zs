
import { println } from "core:io";

const x = "asd";

try {
    2 + x;
    println("Not executed");
} catch (e) {
    println("Catch", e);
}