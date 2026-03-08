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

// Test local in function scope
fn testLetScope() {
    local localLet = 30;
    println("localLet inside function:", localLet);
    
    localLet = 40;
    println("localLet reassigned:", localLet);
}

testLetScope();

// Test local in block scope (if statement)
fn testLocalInBlock() {
    if (true) {
        local blockLet = 50;
        println("blockLet inside if:", blockLet);
    }
    // blockLet not accessible here
}

testLocalInBlock();

// Test local in loop
fn testLetInLoop() {
    for (i := 0; i < 3; i = i + 1) {
        local loopLet = i * 10;
        println("loopLet in iteration:", loopLet);
    }
    // loopLet and i not accessible here
}

testLetInLoop();

// Test shadowing
var shadowTest = 1;

fn testShadowing() {
    local shadowTest = 2;
    println("shadowTest inside function:", shadowTest);
    
    if (true) {
        local shadowTest = 3;
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
    local f = 6, g = 7;
    println("Multiple local:", f, g);
}

testMultipleLet();

println("Variable tests complete!");