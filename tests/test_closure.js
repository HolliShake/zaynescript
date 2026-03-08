function println(...args) {
    console.log(...args);
}

function f1() {
    let v1 = "V1";
    println("Running f1");
    return function() {
        println("Running f1.fn1");
        let v2 = "v2";
        return function() {
            println("Running f1.fn1.fn2");
            let v3 = "v3";
            return function() {
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
// V1 v2 v3
// >> null
println(">>", f1()()()());

// Test: Closure with mutable captured variable
function counter() {
    let count = 0;
    return function() {
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
function makeCounters() {
    let shared = 0;
    let increment = function() {
        shared = shared + 1;
    };
    let decrement = function() {
        shared = shared - 1;
    };
    let getCount = function() {
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
function multiplier(factor) {
    return function(value) {
        return value * factor;
    };
}

var double = multiplier(2);
var triple = multiplier(3);
println("Double 5:", double(5)); // Double 5: 10
println("Triple 5:", triple(5)); // Triple 5: 15

// Test: Closure returning closure with captured loop variable
function makeAdders() {
    let adders = [];
    println("Called");
    for (let i = 0; i < 3; i++) {
        println(i);
        adders.push(function(x) {
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
function outer(a) {
    return function(b) {
        return function(c) {
            return function(d) {
                return a + b + c + d;
            };
        };
    };
}

println("Nested sum:", outer(1)(2)(3)(4)); // Nested sum: 10

// Test: Closure modifying captured object
function objectCapture() {
    let obj = { value: 100 };
    return function(delta) {
        obj.value = obj.value + delta;
        return obj.value;
    };
}

var modifier = objectCapture();
println("Modified:", modifier(50)); // Modified: 150
println("Modified:", modifier(-30)); // Modified: 120

function fact(n) {
    if (n <= 1) return 1;
    return n * fact(n - 1);
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