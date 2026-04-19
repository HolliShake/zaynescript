import { println } from "core:io";

class Greeter {
    fn init(msg) {
        this.msg = msg;
    }
    fn say() {
        return this.msg;
    }
}

class LoudGreeter (Greeter) {
    fn init(msg) {
        this.msg = msg;
    }
    fn say() {
        return this.msg + "!";
    }
}

const g = new Greeter("hi");
println(g.say());

const lg = new LoudGreeter("yo");
println(lg.say());

const arr = [1, 2, 3];
var sum = 0;
arr.each(fn(v, i) {
    sum = sum + v;
});
println(sum);

class Scale {
    fn init(k) {
        this.k = k;
    }
    fn mul(n) {
        return n * this.k;
    }
}
println(new Scale(2).mul(21));
