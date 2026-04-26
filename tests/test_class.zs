import { println } from "core:io";
import { Object } from "core:object";
import { Array } from "core:array";


class BaseClass {
    fn init(a) {
        this.Type = typeof(this);
        println("From BaseClass!", this.Type);
    }

    fn greet() {
        println("greet::From BaseClass!");
    }
}

class DerivedClass (BaseClass) {
    fn init() {
        base(this, 2);
        println( typeof base, typeof this);
        println("From DerivedClass!");
        println(base.greet(this), this.greet());
    }

    fn greet() {
        println("greet::From DerivedClass!");
    }
}


new DerivedClass();

fn test(a, b) {
    println(a, b);
}

test("A", "B");
