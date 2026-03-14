import { println } from "core:io";

fn f1() {
    local v1 = "v1";
    println("Running f1");
    return fn() {
        println("Running f1.fn1");
        local v2 = "v2";
        return fn() {
            println("Running f1.fn1.fn2");
            local v3 = "v3";
            return fn() {
                println("Running f1.fn2.fn3");
                println(v1, v2, v3);
            };
        };
    };
}

// Expected output:
// Running f1
// Running f1.fn1
// Running f1.fn1.fn2
// Running f1.fn2.fn3
// v1 v2 v3
// >> null
println(">>", f1()()()());

// Test: Closure with mutable captured variable
fn counter() {
    local count = 0;
    return fn() {
        count = count + 1;
        return count;
    };
}

var c1 = counter();
println(">>", c1); // >> <function>
println(">> Done"); // >> Done
println("Counter:", c1()); // Counter: 1
println("Counter:", c1()); // Counter: 2
println("Counter:", c1()); // Counter: 3

// Test: Multiple closures sharing same outer scope
fn makeCounters() {
    local shared = 0;
    local increment = fn() {
        shared = shared + 1;
    };
    local decrement = fn() {
        shared = shared - 1;
    };
    local getCount = fn() {
        return shared;
    };
    return [increment, decrement, getCount];
}

var counters = makeCounters();
var inc = counters[0];
var dec = counters[1];
var get = counters[2];
inc();
inc();
println("Shared count:", get()); // Shared count: 2
dec();
println("Shared count:", get()); // Shared count: 1

// Test: Closure with parameters
fn multiplier(factor) {
    return fn(value) {
        return value * factor;
    };
}

var double = multiplier(2);
var triple = multiplier(3);
println("Double 5:", double(5)); // Double 5: 10
println("Triple 5:", triple(5)); // Triple 5: 15

// Test: Closure returning closure with captured loop variable
fn makeAdders() {
    local adders = [];
    println("Called");
    for (i := 0; i < 3; i++) {
        println(i);
        adders.push(fn(x) {
            println("x>", x, i);
            return x + i;
        });
    }
    return adders;
}

// Expected output:
// HERE!
// Called
// 0
// 1
// 2
// Here!
// <function> <function> <function>
// x> 2 3
// 5
// x> 10 3
// x> 11 3
// x> 12 3
// Adder results: 13 14 15
println("HERE!");
var adders = makeAdders();
var fn1 = adders[0];
var fn2 = adders[1];
var fn3 = adders[2];
println("Here!");
println(fn1, fn2, fn3);
println(fn1(2));
println("Adder results:", fn1(10), fn2(11), fn3(12));

// Test: Deep closure nesting with multiple captures
fn outer(a) {
    return fn(b) {
        return fn(c) {
            return fn(d) {
                return a + b + c + d;
            };
        };
    };
}

println("Nested sum:", outer(1)(2)(3)(4)); // Nested sum: 10

// Test: Closure modifying captured object
fn objectCapture() {
    local obj = { value: 100 };
    return fn(delta) {
        obj.value = obj.value + delta;
        return obj.value;
    };
}

var modifier = objectCapture();
println("Modified:", modifier(50)); // Modified: 150
println("Modified:", modifier(-30)); // Modified: 120


fn fact(n) {
    if (n <= 1) return 1;
    return n * fact(n - 1);
}

println(fact(5));


var x = 2;

x += 2;

println(x);

var y = {
    val: 0
};

println(">>", y.val += 2);
y["val"] += 2;

println(y, 100 if (true == false) else 0, true ? 100 : 0);

if (true) {
    println(100);
} else {
    println(0);
}
println(true, false);

const fun = fn() async {
    return 2;
};