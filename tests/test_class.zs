
import { println} from "core:io";

class Animal {
    fn greet() {
        println("From animal!");
    }
}

class Dog (Animal) {
    fn init() {
        println(this);
    }

    fn t() {
        this.greet();
        return this;
    }
}

const dg = new Dog();
println(dg);

const a = [18];
a.push(19);

const ob = {
    cls:  Dog
};


import { Object } from "core:object";
import { Array } from "core:array";

println(a, dg.t(), new Dog, new ob.cls, new Object, new Array);

