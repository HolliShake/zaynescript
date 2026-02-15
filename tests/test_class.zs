
import { println} from "core:io";


class Dog {
    fn init() {
        println(this);
    }

    fn t() {
        return this;
    }
}
const dg = new Dog();
println(dg);

const a = [];
a.push(19);

println(a, dg.t());

