// Core (Priority)
[x] Add anonymous function
[x] Add Array and Object
[x] Add BigInt and BigNum
[x] Add BigInt and BigNum operation
[x] Add ternary expression (condition ? a : b)
[x] Add if-else expression (python style: value if condition else other)
[x] Add for loop and while loop similar to golang
[x] Add do while
[x] Add Try/catch
[x] Add Switch Expression and Switch Statement
[x] Add async/await and state machine
[ ] Add null coalescing operator (??)
[ ] Add optional chaining (?.)
[x] Fix memory leak for bitfields/bignum implementation/operations

// Needs to be Checked
[ ] [Parser] if a "local", "var", "const" statement was followed by an identifier or group assignment
[ ] [Parser] if a "new" operator was followed by an expression then "(" args* ")"
[x] [Parser] if a "class" was followed by identifier



**BUGS**
[src/interpreter.c:869]::Panic: Invalid stack state: StckC (1) is less than StackBot (3)
   - REPRODUCE:
       APP.controller(UserController, fn(app, ctrl) {
            // health
            app.get("health", fn(req, res) async => await ctrl["health"](req, res));
        });
 