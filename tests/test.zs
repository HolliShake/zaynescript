import { println, scan, parseNum } from "core:io";
import "core:math";

var x = parseNum(scan("input> "));

println("Hello", 123, x);

var counter = 0;

fn a() {
    let x = 2;
    b();
    println(x);
}

fn b() {
    for (xx := 0; xx < x; xx = xx + 1) {
        if (xx == 3) {
            continue;
        }
        if (xx == 5) {
            break;
        }
        println("|>>", xx);
    }
    while (yy := 0; yy <= 5 /*; yy = yy + 1 optional */) {
        println(yy);
        yy = yy + 1;
    }
    println("Done", io);
}

a();

fn fact(n) {
    if (n <= 1) {
        return 1;
    }
    return n * fact(n - 1);
}

const g = false, h = g;

println(g && true);

println(
    fact(
        parseNum(
            scan("Fact> ")
        )
    )
);

const msg = "Hello";

try {
    let x = 2;
    try {
        2 + msg;
        println("Exec??");
    } catch (err) {
        println("inside >>", err);
    }
    println("Exec??");
} catch (e) {
    println("error >>", e);
}

println("Dec>>");

fn runnable(local) {
    println("Hello!");
    return fn() {
        const a = 2 << 2;
        println("Word!", a, local + local);
    };
}

runnable(69420)();
runnable(69421)();

const obj = {
    value: 123
};

println(obj);
++obj["value"] + 23 - 1;
println(obj);

println(obj);
println(obj.value++ + 23 - 1);
println(obj, 2, 3, 2);

var hhj = 0;

println(hhj++);
println(hhj, obj.value, obj["value"]);

fn inc() {
    hhj++;
}

inc();

println("HERE!");
println(hhj, [1, 2, 3, ...[4,5,6], 7, 8, 9]);
println(math);

class Tao {
    fn GetAge() {
        return 20;
    }
}

class Person(Tao) {
    fn GetName() {
        this.name = "Andy";
        println(">>", this);
    }

    static fn StaticMethod() {
        println("Static Method Called");
    }
}

const p = new Person();

println(p, p.GetName(), p.GetAge(), fact, Person.StaticMethod);

fn order(a,b) {
    println(a, b);
}

order(100, 200);