import { println } from "core:io";

// Test basic closure
fn createCounter() {
    let count = 0;
    return fn() {
        count = count + 1;
        return count;
    };
}

const counter = createCounter();
const result1 = counter();
const result2 = counter();
const result3 = counter();

println("Counter test:");
println("First call:", result1);
println("Second call:", result2);
println("Third call:", result3);

if (result1 != 1) {
    println("ERROR: Expected first call to return 1, got", result1);
}
if (result2 != 2) {
    println("ERROR: Expected second call to return 2, got", result2);
}
if (result3 != 3) {
    println("ERROR: Expected third call to return 3, got", result3);
}

// Test closure capturing multiple variables
fn createAdder(x) {
    return fn(y) {
        return x + y;
    };
}

const add5 = createAdder(5);
const addResult = add5(10);
println("\nAdder test:");
println("add5(10):", addResult);

if (addResult != 15) {
    println("ERROR: Expected add5(10) to return 15, got", addResult);
}

// Test nested closures
fn outer() {
    let outerVar = "outer";
    return fn() {
        let middleVar = "middle";
        return fn() {
            return outerVar + "-" + middleVar + "-inner";
        };
    };
}

const nestedResult = outer()()();
println("\nNested closure test:");
println("Result:", nestedResult);

if (nestedResult != "outer-middle-inner") {
    println("ERROR: Expected 'outer-middle-inner', got", nestedResult);
}

// Test closure with loop
fn createMultipliers() {
    const multipliers = [];
    for (i := 1; i <= 3; i = i + 1) {
        const capturedI = i;
        multipliers.push(fn(x) {
            return x * capturedI;
        });
    }
    return multipliers;
}

const multipliers = createMultipliers();
println("\nLoop closure test:");
println("multipliers[0](10):", multipliers[0](10));
println("multipliers[1](10):", multipliers[1](10));
println("multipliers[2](10):", multipliers[2](10));

if (multipliers[0](10) != 10) {
    println("ERROR: Expected multipliers[0](10) to return 10, got", multipliers[0](10));
}
if (multipliers[1](10) != 20) {
    println("ERROR: Expected multipliers[1](10) to return 20, got", multipliers[1](10));
}
if (multipliers[2](10) != 30) {
    println("ERROR: Expected multipliers[2](10) to return 30, got", multipliers[2](10));
}

// Test closure modifying captured variable
fn createToggle() {
    let state = false;
    return fn() {
        state = !state;
        return state;
    };
}

const toggle = createToggle();
println("\nToggle test:");
println("First toggle:", toggle());
println("Second toggle:", toggle());
println("Third toggle:", toggle());

if (toggle() != false) {
    println("ERROR: Expected fourth toggle to return false");
}

println("\nClosure tests complete!");
