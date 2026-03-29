
import { println} from "core:io";
import "./test_import";

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

println(a, dg.t());

