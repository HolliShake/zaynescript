// filepath: d:\language-x\tests\test_variable.zs
import { println } from "core:io";

// Test var - global scope only
var globalVar = 10;
println("globalVar initial:", globalVar);

globalVar = 20;
println("globalVar after reassignment:", globalVar);

// Test const - immutable
const globalConst = 100;
println("globalConst:", globalConst);

// Attempting to reassign const should error (commented out)
// globalConst = 200;

// Test let in function scope
fn testLetScope() {
    let localLet = 30;
    println("localLet inside function:", localLet);
    
    localLet = 40;
    println("localLet reassigned:", localLet);
}

testLetScope();

// Test let in block scope (if statement)
fn testLetInBlock() {
    if (true) {
        let blockLet = 50;
        println("blockLet inside if:", blockLet);
    }
    // blockLet not accessible here
}

testLetInBlock();

// Test let in loop
fn testLetInLoop() {
    for (i := 0; i < 3; i = i + 1) {
        let loopLet = i * 10;
        println("loopLet in iteration:", loopLet);
    }
    // loopLet and i not accessible here
}

testLetInLoop();

// Test shadowing
var shadowTest = 1;

fn testShadowing() {
    let shadowTest = 2;
    println("shadowTest inside function:", shadowTest);
    
    if (true) {
        let shadowTest = 3;
        println("shadowTest inside block:", shadowTest);
    }
    
    println("shadowTest after block:", shadowTest);
}

testShadowing();
println("shadowTest global:", shadowTest);

// Test multiple declarations
const a = 1, b = 2, c = 3;
println("Multiple const:", a, b, c);

var d = 4, e = 5;
println("Multiple var:", d, e);

fn testMultipleLet() {
    let f = 6, g = 7;
    println("Multiple let:", f, g);
}

testMultipleLet();

println("Variable tests complete!");