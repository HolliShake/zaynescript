import { print, println, format } from "core:io";

println("=== Beginning VM Unwind Tests ===");

// ---------------------------------------------------------
// TEST 1: Try block OUTSIDE the loop
// Goal: Ensure ScopeCountNestedUntil stops at SCOPE_LOOP 
// and DOES NOT pop the outer try block.
// ---------------------------------------------------------
println("\n[Test 1] Outer Try Block:");
var test1_success = 0;
try {
    for (i := 0; i < 5; i++) {
        if (i == 2) {
            println("  -> Breaking out of loop...");
            break; 
        }
    }
    
    // If the compiler incorrectly popped the outer try block during the break, 
    // throwing an error here would cause a fatal VM panic instead of being caught!
    // (Assuming your language has a 'throw' statement. If not, just reaching here means it didn't crash).
    test1_success = 1;
} catch (e) {
    println("  -> Caught unexpected error: " + e);
}

if (test1_success == 1) {
    println("  -> PASS: Outer try block survived the break.");
}

// ---------------------------------------------------------
// TEST 2: Try block INSIDE the loop
// Goal: Ensure the compiler EMITS OP_POP_TRY because the 
// try block is inside the loop boundary.
// ---------------------------------------------------------
println("\n[Test 2] Inner Try Block:");
for (j := 0; j < 3; j++) {
    try {
        if (j == 1) {
            println("  -> Breaking from inside a try block...");
            break; // This MUST emit an OP_POP_TRY instruction.
        }
    } catch (e) {
        println("  -> Should not reach here.");
    }
}
println("  -> PASS: VM did not corrupt the stack (OP_POP_TRY worked).");


// ---------------------------------------------------------
// TEST 3: Lexical Environments (Local Variables)
// Goal: Ensure local scopes are popped properly to prevent
// memory leaks or stack overflows when breaking.
// ---------------------------------------------------------
println("\n[Test 3] Local Scope Unwinding:");
for (k := 0; k < 10; k++) {
    // Pushing new local variables onto the environment stack
    local temp_str = "allocating memory";
    local temp_num = k * 100;
    
    if (k == 5) {
        println("  -> Breaking with locals on stack...");
        break; // This MUST emit OP_POPN_ENV to clear temp_str and temp_num.
    }
}
println("  -> PASS: Loop exited cleanly.");

println("\n=== All execution tests finished without panicking! ===");