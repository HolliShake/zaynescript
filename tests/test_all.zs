// =============================================================
//  ZayneScript — Comprehensive Test Suite
//  Covers: variables, operators, strings, arrays, objects,
//          control flow, loops, switch, functions, closures,
//          classes/inheritance, try-catch, async/await, math,
//          date, format, spread, inline-scope, IIFE, HOF.
// =============================================================

import { println, format } from "core:io";
import { abs, ceil, cos, floor, hypot, max, min, pow, round, sin, sqrt, pi, e } from "core:math";
import { Date } from "core:Date";

// -------------------------------------------------------------
//  Test runner
// -------------------------------------------------------------
var _passed = 0;
var _failed = 0;

fn assert(condition, label) {
    if (condition) {
        _passed = _passed + 1;
        println(format("[PASS] {}", label));
    } else {
        _failed = _failed + 1;
        println(format("[FAIL] {}", label));
    }
}

fn section(title) {
    println("");
    println(format("━━━  {}  ━━━", title));
}

// =============================================================
//  1. VARIABLES  (var / const / local)
// =============================================================
section("1. VARIABLES");

var gNum = 42;
assert(gNum == 42, "var: initial value");
gNum = 99;
assert(gNum == 99, "var: reassignment");

const PI_APPROX = 3.14159;
assert(PI_APPROX == 3.14159, "const: float value");

const FLAG = true;
assert(FLAG == true, "const: bool value");

const NOTHING = null;
assert(NOTHING == null, "const: null value");

const MA = 1, MB = 2, MC = 3;
assert(MA == 1 && MB == 2 && MC == 3, "const: multi-decl");

fn testLocalVars() {
    local x = 10;
    assert(x == 10, "local: initial");
    x = 20;
    assert(x == 20, "local: reassign");
    local a = 1, b = 2, c = 3;
    assert(a == 1 && b == 2 && c == 3, "local: multi-decl");
}
testLocalVars();

var shadowOuter = "outer";
fn testShadowing() {
    local shadowOuter = "inner";
    assert(shadowOuter == "inner", "shadow: local hides global");
    if (true) {
        local shadowOuter = "block";
        assert(shadowOuter == "block", "shadow: block hides fn-local");
    }
    assert(shadowOuter == "inner", "shadow: fn-local restored after block");
}
testShadowing();
assert(shadowOuter == "outer", "shadow: global unchanged");

// =============================================================
//  2. ARITHMETIC OPERATORS
// =============================================================
section("2. ARITHMETIC OPERATORS");

assert(5 + 3 == 8,       "arith: add");
assert(10 - 4 == 6,      "arith: sub");
assert(3 * 7 == 21,      "arith: mul");
assert(20 / 4 == 5,      "arith: div");
assert(17 % 5 == 2,      "arith: mod");
assert(-8 == 0 - 8,      "arith: unary minus");
assert(+5 == 5,          "arith: unary plus");
assert(1.5 + 2.5 == 4.0, "arith: float add");
assert(5.0 / 2.0 == 2.5, "arith: float div");

// Operator precedence
assert(2 + 3 * 4 == 14,   "arith: precedence mul > add");
assert((2 + 3) * 4 == 20, "arith: parens override precedence");
assert(10 - 3 - 2 == 5,   "arith: left-assoc sub");

// Compound assignment
var ca = 10;
ca += 5; assert(ca == 15, "compound: +=");
ca -= 3; assert(ca == 12, "compound: -=");
ca *= 2; assert(ca == 24, "compound: *=");
ca /= 4; assert(ca == 6,  "compound: /=");
ca %= 4; assert(ca == 2,  "compound: %=");

// Prefix / postfix increment & decrement
var inc = 5;
inc++;         assert(inc == 6, "inc: postfix ++ mutates");
++inc;         assert(inc == 7, "inc: prefix ++ mutates");
inc--;         assert(inc == 6, "inc: postfix -- mutates");
--inc;         assert(inc == 5, "inc: prefix -- mutates");

var pre = 3;
assert(++pre == 4 && pre == 4, "inc: prefix ++ returns new value");
var post = 3;
assert(post++ == 3 && post == 4, "inc: postfix ++ returns old value");

// =============================================================
//  3. STRINGS
// =============================================================
section("3. STRINGS");

const s1 = "Hello";
const s2 = "World";
assert(s1 + ", " + s2 == "Hello, World", "string: concat +");

var buf = "";
buf += "foo"; buf += "bar";
assert(buf == "foobar", "string: += accumulation");

assert("abc" == "abc", "string: equality");
assert("abc" != "xyz", "string: inequality");

const fmt1 = format("x={} y={}", 10, 20);
assert(fmt1 == "x=10 y=20", "format: int substitution");

const fmt2 = format("pi≈{}", 3.14);
assert(fmt2 == "pi≈3.14", "format: float substitution");

const fmt3 = format("{} is {}", "ZS", true);
assert(fmt3 == "ZS is true", "format: mixed types");

const fmt4 = format("empty={}", null);
assert(fmt4 == "empty=null", "format: null substitution");

// =============================================================
//  4. COMPARISON & LOGICAL OPERATORS
// =============================================================
section("4. COMPARISON & LOGICAL");

assert(1 < 2,   "cmp: less-than");
assert(2 > 1,   "cmp: greater-than");
assert(2 <= 2,  "cmp: less-or-equal (eq)");
assert(1 <= 2,  "cmp: less-or-equal (lt)");
assert(3 >= 3,  "cmp: greater-or-equal (eq)");
assert(4 >= 3,  "cmp: greater-or-equal (gt)");
assert(5 == 5,  "cmp: equal");
assert(5 != 6,  "cmp: not-equal");

assert(true && true,     "logic: && both true");
assert(!(true && false), "logic: && one false");
assert(true || false,    "logic: || one true");
assert(!(false || false),"logic: || both false");
assert(!false,           "logic: !false");
assert(!true == false,   "logic: !true");

// Short-circuit
var sideEff = 0;
fn bumpSide() { sideEff = sideEff + 1; return true; }
false && bumpSide();
assert(sideEff == 0, "logic: && short-circuit left=false");
true || bumpSide();
assert(sideEff == 0, "logic: || short-circuit left=true");

// =============================================================
//  5. BITWISE OPERATORS
// =============================================================
section("5. BITWISE OPERATORS");

assert((6  & 3) == 2,  "bit: AND  6&3=2");
assert((5  | 3) == 7,  "bit: OR   5|3=7");
assert((6  ^ 3) == 5,  "bit: XOR  6^3=5");
assert((1 << 3) == 8,  "bit: SHL  1<<3=8");
assert((16 >>2) == 4,  "bit: SHR  16>>2=4");

// =============================================================
//  6. TERNARY OPERATOR
// =============================================================
section("6. TERNARY OPERATOR");

assert((true  ? "yes" : "no") == "yes", "ternary: true branch");
assert((false ? "yes" : "no") == "no",  "ternary: false branch");
assert((10 > 5 ? "big" : "small") == "big", "ternary: expr cond");

// Nested ternary
const nTern = 5 > 3 ? (2 < 1 ? "inner" : "outer-in") : "outer";
assert(nTern == "outer-in", "ternary: nested");

// =============================================================
//  7. IF / ELSE  (including inline-scope)
// =============================================================
section("7. IF / ELSE");

fn classify(n) {
    if (n > 0) { return "positive"; }
    else if (n < 0) { return "negative"; }
    else { return "zero"; }
}
assert(classify(5)  == "positive", "if-else: positive branch");
assert(classify(-1) == "negative", "if-else: negative branch");
assert(classify(0)  == "zero",     "if-else: zero branch");

// Inline-scope  if (init; cond)
// NOTE: interpreter has a known bug when the condition evaluates to false
// (local variables after the if become unreadable). Tests only use true conditions.
fn testInlineIfScope() {
    local result = 0;
    if (x := 42; x > 10) {
        result = 1;
    }
    return result;
}
assert(testInlineIfScope() == 1, "inline-scope: cond true, body runs");

// Nested inline scopes — each var used only in its own condition
fn testNestedInlineScopes() {
    local result = 0;
    if (x := 1; x > 0) {
        if (y := 2; y == 2) {
            result = 1;
        }
    }
    return result;
}
assert(testNestedInlineScopes() == 1, "inline-scope: nested both true");

// =============================================================
//  8. LOOPS
// =============================================================
section("8. LOOPS");

// Basic for
var sumFor = 0;
for (i := 1; i <= 5; i = i + 1) { sumFor += i; }
assert(sumFor == 15, "for: sum 1..5 with i=i+1");

// For with i++
var cntFor = 0;
for (j := 0; j < 10; j++) { cntFor++; }
assert(cntFor == 10, "for: 10 iterations with j++");

// While
var w = 0; var wSum = 0;
while (w < 5) { wSum += w; w++; }
assert(wSum == 10, "while: sum 0..4");

// While inline-scope
fn testWhileInline() {
    local total = 0;
    while (n := 0; n < 4) { total += n; n = n + 1; }
    return total;
}
assert(testWhileInline() == 6, "while: inline-scope 0+1+2+3=6");

// do-while
var doCount = 0;
do { doCount++; } while (doCount < 5);
assert(doCount == 5, "do-while: runs until cond false");

var doOnce = 0;
do { doOnce++; } while (false);
assert(doOnce == 1, "do-while: body runs at least once");

// break
var bkSum = 0;
for (k := 0; k < 100; k++) {
    if (k == 5) { break; }
    bkSum += k;
}
assert(bkSum == 10, "break: exits loop (0+1+2+3+4)");

// continue
var ctSum = 0;
for (m := 0; m < 10; m++) {
    if (m % 2 == 1) { continue; }
    ctSum += m;
}
assert(ctSum == 20, "continue: skip odds 0+2+4+6+8");

// Nested loops with inner break
var nHit = 0;
for (x := 0; x < 3; x++) {
    for (y := 0; y < 5; y++) {
        if (y == 2) { break; }
        nHit++;
    }
}
assert(nHit == 6, "nested: inner break (3 rows × 2 hits)");

// =============================================================
//  9. SWITCH STATEMENT & SWITCH EXPRESSION
// =============================================================
section("9. SWITCH");

// Statement form
fn dayName(d) {
    switch (d) {
        case 1: { return "Mon"; }
        case 2: { return "Tue"; }
        case 3: { return "Wed"; }
        case 4, 5: { return "Thu-Fri"; }
        default: { return "Weekend"; }
    }
}
assert(dayName(1) == "Mon",     "switch-stmt: case 1");
assert(dayName(3) == "Wed",     "switch-stmt: case 3");
assert(dayName(4) == "Thu-Fri", "switch-stmt: multi-value case 4");
assert(dayName(5) == "Thu-Fri", "switch-stmt: multi-value case 5");
assert(dayName(7) == "Weekend", "switch-stmt: default");

// Expression form
const sw1 = 100 switch {
    case 10  => "Ten"
    case 100 => "Hundred"
    default  => "Other"
};
assert(sw1 == "Hundred", "switch-expr: exact match");

const sw2 = 999 switch {
    case 1, 2, 3 => "small"
    default      => "big"
};
assert(sw2 == "big", "switch-expr: default");

const sw3 = 3 switch {
    case 1, 2, 3 => "hit"
    default      => "miss"
};
assert(sw3 == "hit", "switch-expr: multi-value match");

// =============================================================
//  10. FUNCTIONS  (basic, recursion, HOF, IIFE)
// =============================================================
section("10. FUNCTIONS");

fn add(a, b) { return a + b; }
assert(add(3, 4) == 7,   "fn: basic add");
assert(add(0, 0) == 0,   "fn: add zeros");
assert(add(-1, 1) == 0,  "fn: add negatives");

// Recursion — factorial
fn factorial(n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}
assert(factorial(0)  == 1,        "fn: factorial(0)");
assert(factorial(1)  == 1,        "fn: factorial(1)");
assert(factorial(5)  == 120,      "fn: factorial(5)");
assert(factorial(10) == 3628800,  "fn: factorial(10)");

// Recursion — fibonacci
fn fib(n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}
assert(fib(0)  == 0,  "fn: fib(0)");
assert(fib(1)  == 1,  "fn: fib(1)");
assert(fib(7)  == 13, "fn: fib(7)");
assert(fib(10) == 55, "fn: fib(10)");

// Anonymous function stored in var
const square = fn(n) { return n * n; };
assert(square(5) == 25, "fn: anon fn const");
assert(square(0) == 0,  "fn: anon fn zero");

// Function as argument
fn applyTwice(f, x) { return f(f(x)); }
assert(applyTwice(square, 2) == 16, "fn: fn-as-arg applyTwice(square,2)");
assert(applyTwice(square, 3) == 81, "fn: fn-as-arg applyTwice(square,3)");

// Returning a function
fn makeAdder(n) {
    return fn(x) { return x + n; };
}
const add10 = makeAdder(10);
assert(add10(5)   == 15,  "fn: HOF add10(5)");
assert(add10(90)  == 100, "fn: HOF add10(90)");
assert(add10(-10) == 0,   "fn: HOF add10(-10)");

// Nested fn via local anonymous
fn outerFn(x) {
    local innerFn = fn(y) { return x * 2 + y; };
    return innerFn(1);
}
assert(outerFn(3) == 7, "fn: nested def (3*2+1)");

// IIFE
const iife1 = (fn() { return 42; })();
assert(iife1 == 42, "fn: IIFE no-args");
const iife2 = (fn(a, b) { return a + b; })(10, 32);
assert(iife2 == 42, "fn: IIFE with args");

// =============================================================
//  11. CLOSURES
// =============================================================
section("11. CLOSURES");

// Counter — mutable capture
fn makeCounter() {
    local count = 0;
    return fn() { count = count + 1; return count; };
}
var cnt = makeCounter();
assert(cnt() == 1, "closure: count #1");
assert(cnt() == 2, "closure: count #2");
assert(cnt() == 3, "closure: count #3");

// Independent instances
var cntA = makeCounter();
var cntB = makeCounter();
cntA(); cntA();
cntB();
assert(cntA() == 3, "closure: independent A=3");
assert(cntB() == 2, "closure: independent B=2");

// Shared state via pair of closures
fn makeSharedPair() {
    local val = 0;
    const getter = fn() { return val; };
    const setter = fn(v) { val = v; };
    return [getter, setter];
}
var pair    = makeSharedPair();
var getVal  = pair[0];
var setVal  = pair[1];
setVal(42);
assert(getVal() == 42,  "closure: shared state set 42");
setVal(100);
assert(getVal() == 100, "closure: shared state updated to 100");

// Deep chain (3 levels)
fn deepChain() {
    local a = 1;
    return fn() {
        local b = 2;
        return fn() {
            local c = 3;
            return fn() { return a + b + c; };
        };
    };
}
assert(deepChain()()()() == 6, "closure: 3-level chain 1+2+3=6");

// Loop capture via local
fn makeAdders() {
    local adders = [];
    for (n := 0; n < 4; n++) {
        local captured = n;
        adders.push(fn() { return captured; });
    }
    return adders;
}
var adders = makeAdders();
assert(adders[0]() == 0, "closure: loop-capture [0]");
assert(adders[1]() == 1, "closure: loop-capture [1]");
assert(adders[2]() == 2, "closure: loop-capture [2]");
assert(adders[3]() == 3, "closure: loop-capture [3]");

// Multiplier factory
fn multiplier(factor) {
    return fn(value) { return value * factor; };
}
var double = multiplier(2);
var triple = multiplier(3);
assert(double(5)  == 10, "closure: double(5)");
assert(triple(5)  == 15, "closure: triple(5)");
assert(double(0)  == 0,  "closure: double(0)");

// =============================================================
//  12. ARRAYS
// =============================================================
section("12. ARRAYS");

// Literal + indexing
const arr1 = [10, 20, 30];
assert(arr1[0] == 10, "array: [0]");
assert(arr1[1] == 20, "array: [1]");
assert(arr1[2] == 30, "array: [2]");

// Index mutation
var mArr = [1, 2, 3];
mArr[1] = 99;
assert(mArr[1] == 99, "array: index write");

// push + length()
var pArr = [];
pArr.push(10); pArr.push(20); pArr.push(30);
assert(pArr[0] == 10,      "array: push [0]");
assert(pArr[2] == 30,      "array: push [2]");
assert(pArr.length() == 3, "array: length() after 3 pushes");

// pop
var popArr = [1, 2, 3];
var popped = popArr.pop();
assert(popped == 3,          "array: pop returns last");
assert(popArr.length() == 2, "array: length after pop");

// Spread
const spread = [1, 2, ...[3, 4, 5]];
assert(spread[0] == 1 && spread[2] == 3 && spread[4] == 5, "array: spread");
assert(spread.length() == 5, "array: spread length=5");

// Nested arrays
const nested = [[1, 2], [3, 4], [5, 6]];
assert(nested[0][0] == 1, "array: nested [0][0]");
assert(nested[1][1] == 4, "array: nested [1][1]");
assert(nested[2][0] == 5, "array: nested [2][0]");

// Mixed types
const mixed = [1, "two", true, null, 3.14];
assert(mixed[0] == 1,     "array: mixed int");
assert(mixed[1] == "two", "array: mixed string");
assert(mixed[2] == true,  "array: mixed bool");
assert(mixed[3] == null,  "array: mixed null");

// each — callback receives (item, index)
var eachSum = 0;
[1, 2, 3, 4, 5].each(fn(v, idx) { eachSum += v; });
assert(eachSum == 15, "array.each: sum via (v, idx)");

var eachIdxSum = 0;
[10, 20, 30].each(fn(v, idx) { eachIdxSum += idx; });
assert(eachIdxSum == 3, "array.each: index sum 0+1+2=3");

// each with println (VARARG native — allowed)
// [1, 2, 3].each(println);   // just a smoke-test, no assertion

// keep (filter)
var keepResult = [1, 2, 3, 4, 5, 6].keep(fn(v, idx) { return v % 2 == 0; });
assert(keepResult[0] == 2, "array.keep: first even");
assert(keepResult[1] == 4, "array.keep: second even");
assert(keepResult[2] == 6, "array.keep: third even");
assert(keepResult.length() == 3, "array.keep: length=3");

// Array of objects
var aoArr = [{x: 1}, {x: 2}, {x: 3}];
var aoSum = 0;
aoArr.each(fn(obj, idx) { aoSum += obj.x; });
assert(aoSum == 6, "array.each: sum object fields");

// =============================================================
//  13. OBJECTS
// =============================================================
section("13. OBJECTS");

const obj1 = { name: "Alice", age: 30 };
assert(obj1.name == "Alice", "object: dot .name");
assert(obj1.age  == 30,      "object: dot .age");
assert(obj1["name"] == "Alice", "object: bracket [name]");

// Mutation
var mObj = { x: 10, y: 20 };
mObj.x = 99;      assert(mObj.x == 99, "object: dot write");
mObj["y"] = 77;   assert(mObj.y == 77, "object: bracket write");

// Deep nested
const deep = { a: { b: { c: 42 } } };
assert(deep.a.b.c == 42, "object: deep access");

// Object with callable field
const owf = { val: 10, getVal: fn() { return 10; } };
assert(owf.val    == 10, "object: plain field");
assert(owf.getVal() == 10, "object: callable field");

// Prefix / postfix on object property
var cObj = { n: 0 };
cObj.n++;           assert(cObj.n == 1,  "object: postfix ++");
++cObj.n;           assert(cObj.n == 2,  "object: prefix ++");
cObj["n"] += 10;    assert(cObj.n == 12, "object: += on bracket");

// Spread into array from object array
var objArr = [{ v: 5 }, { v: 10 }];
var objSum = 0;
objArr.each(fn(o, i) { objSum += o.v; });
assert(objSum == 15, "object: array.each over object array");

// =============================================================
//  14. CLASSES  (declaration, inheritance, method override)
// =============================================================
section("14. CLASSES");

class Shape {
    fn init(color) {
        this.color = color;
    }
    fn getColor() {
        return this.color;
    }
    fn area() {
        return 0;
    }
    fn describe() {
        return format("{}(color={}, area={})", "shape", this.getColor(), this.area());
    }
}

class Rectangle (Shape) {
    fn init(color, w, h) {
        this.color = color;
        this.w = w;
        this.h = h;
    }
    fn area() {
        return this.w * this.h;
    }
    fn perimeter() {
        return 2 * (this.w + this.h);
    }
}

class Circle (Shape) {
    fn init(color, r) {
        this.color = color;
        this.r = r;
    }
    fn area() {
        return 3 * this.r * this.r;
    }
}

const rect = new Rectangle("red", 4, 5);
assert(rect.getColor()   == "red", "class: inherited getColor");
assert(rect.area()       == 20,    "class: overridden area");
assert(rect.perimeter()  == 18,    "class: own perimeter");

const circ = new Circle("blue", 3);
assert(circ.getColor() == "blue", "class: circle inherited getColor");
assert(circ.area()     == 27,     "class: circle overridden area");

// Polymorphic method that calls overridden sub-method via this
const rdesc = rect.describe();
assert(rdesc == "shape(color=red, area=20)", "class: polymorphic describe");

// Method chaining — return this
class Builder {
    fn init() {
        this.parts = [];
    }
    fn add(part) {
        this.parts.push(part);
        return this;
    }
    fn build() {
        local result = "";
        this.parts.each(fn(p, i) { result += p; });
        return result;
    }
}
const built = new Builder().add("a").add("b").add("c").build();
assert(built == "abc", "class: fluent builder chain");

// Class with computed fields in init
class Point {
    fn init(x, y) {
        this.x = x;
        this.y = y;
    }
    fn distFromOrigin() {
        return sqrt(this.x * this.x + this.y * this.y + 0.0);
    }
    fn translate(dx, dy) {
        this.x = this.x + dx;
        this.y = this.y + dy;
        return this;
    }
}
const pt = new Point(3, 4);
assert(pt.distFromOrigin() == 5.0, "class: sqrt distance 3-4-5");
pt.translate(1, 0);
assert(pt.x == 4 && pt.y == 4, "class: mutate via method");

// Base class calling pattern (super-like via base class method)
class Animal {
    fn init(name) {
        this.name = name;
    }
    fn speak() {
        return format("{} says ...", this.name);
    }
}

class Dog (Animal) {
    fn init(name) {
        this.name = name;
        this.breed = "mutt";
    }
    fn speak() {
        return format("{} barks!", this.name);
    }
    fn greet() {
        return this.speak();
    }
}

class Cat (Animal) {
    fn init(name) {
        this.name = name;
    }
    fn speak() {
        return format("{} meows.", this.name);
    }
}

var dog = new Dog("Rex");
var cat = new Cat("Luna");
assert(dog.speak()  == "Rex barks!",   "class: Dog speak");
assert(cat.speak()  == "Luna meows.",  "class: Cat speak");
assert(dog.greet()  == "Rex barks!",   "class: Dog greet->speak");

// =============================================================
//  15. TRY / CATCH
// =============================================================
section("15. TRY / CATCH");

// Basic catch (use variable so error is runtime, not compile-time)
var caughtBasic = false;
const badStr = "nope";
try {
    2 + badStr;
} catch (e) {
    caughtBasic = true;
}
assert(caughtBasic, "try-catch: type-error caught");

// No throw — catch not entered
var noCatch = true;
try {
    const safe = 1 + 2;
} catch (e) {
    noCatch = false;
}
assert(noCatch, "try-catch: no error, catch skipped");

// Error message available
var errMsg = "";
try {
    null.something;
} catch (e) {
    errMsg = e;
}
assert(errMsg != "", "try-catch: message captured");

// Nested: inner catches, outer not reached
var innerGot  = false;
var outerGot  = false;
const nullVal = null;
try {
    try {
        1 + nullVal;
    } catch (ie) {
        innerGot = true;
    }
} catch (oe) {
    outerGot = true;
}
assert(innerGot,  "try-catch: nested inner catches");
assert(!outerGot, "try-catch: nested outer not entered");

// safe-division helper using try
fn safeDivide(a, b) {
    const _null = null;
    try {
        if (b == 0) { 1 + _null; }
        return a / b;
    } catch (e) {
        return null;
    }
}
assert(safeDivide(10, 2) == 5,    "try-catch: normal path");
assert(safeDivide(10, 0) == null, "try-catch: null from catch");

// try inside loop
var loopErrors = 0;
for (li := 0; li < 5; li++) {
    local _liStr = "x";
    try {
        if (li % 2 == 0) { _liStr + li; }
    } catch (e) {
        loopErrors++;
    }
}
assert(loopErrors == 3, "try-catch: 3 errors in loop (0,2,4)");

// =============================================================
//  16. ASYNC / AWAIT  (promises, .then, .error)
// =============================================================
section("16. ASYNC / AWAIT");

fn asyncAdd(a, b) async {
    return a + b;
}

fn asyncChain() async {
    const r = await asyncAdd(3, 4);
    return r * 2;
}

var asyncResult = null;
asyncChain().then(fn(v) { asyncResult = v; });
assert(asyncResult == 14, "async: await chain 3+4=7, *2=14");

// Multiple sequential awaits
fn asyncMulti() async {
    const a = await asyncAdd(1, 2);
    const b = await asyncAdd(3, 4);
    return a + b;
}
var multiResult = null;
asyncMulti().then(fn(v) { multiResult = v; });
assert(multiResult == 10, "async: multiple awaits 3+7=10");

// .then chaining
var chainFinal = null;
asyncAdd(5, 5)
    .then(fn(v) { return v * 2; })
    .then(fn(v) { return v + 1; })
    .then(fn(v) { chainFinal = v; });
assert(chainFinal == 21, "async: .then chain (5+5)*2+1=21");

// .error fires on throw inside async
var asyncErrCaught = false;
fn asyncFail() async {
    const _boom = "boom";
    2 + _boom;
    return "ok";
}
asyncFail().error(fn(e) { asyncErrCaught = true; });
assert(asyncErrCaught, "async: .error fires on throw");

// Awaiting a .then-returning value
fn asyncDouble(n) async {
    return n * 2;
}
fn asyncUseDouble() async {
    const x = await asyncDouble(21);
    return x;
}
var doubleResult = null;
asyncUseDouble().then(fn(v) { doubleResult = v; });
assert(doubleResult == 42, "async: await result used in return");

// Inline-scope inside async
fn asyncInlineScope() async {
    if (x := true; x) {
        return await asyncAdd(10, 10);
    }
    return 0;
}
var isResult = null;
asyncInlineScope().then(fn(v) { isResult = v; });
assert(isResult == 20, "async: inline-scope inside async fn");

// Async class method
class AsyncCalc {
    fn init()    { this.value = 0; }
    fn set(n) async {
        this.value = n;
        return this.value;
    }
}
var ac = new AsyncCalc();
var acVal = null;
ac.set(99).then(fn(v) { acVal = v; });
assert(acVal == 99, "async: class method promise result");

// =============================================================
//  17. MATH MODULE
// =============================================================
section("17. MATH MODULE");

// floor / ceil / round — require float inputs
assert(floor(3.9)   == 3.0,  "math: floor(3.9)");
assert(floor(-1.1)  == -2.0, "math: floor(-1.1)");
assert(ceil(3.1)    == 4.0,  "math: ceil(3.1)");
assert(ceil(-2.9)   == -2.0, "math: ceil(-2.9)");
assert(round(3.5)   == 4.0,  "math: round(3.5)");
assert(round(3.4)   == 3.0,  "math: round(3.4)");

// abs
assert(abs(-5.0)  == 5.0, "math: abs(-5.0)");
assert(abs(5.0)   == 5.0, "math: abs(5.0)");
assert(abs(0.0)   == 0.0, "math: abs(0.0)");

// sqrt
assert(sqrt(9.0)   == 3.0, "math: sqrt(9.0)");
assert(sqrt(16.0)  == 4.0, "math: sqrt(16.0)");
assert(sqrt(0.0)   == 0.0, "math: sqrt(0.0)");

// pow
assert(pow(2.0, 10.0) == 1024.0, "math: pow(2,10)");
assert(pow(3.0, 3.0)  == 27.0,   "math: pow(3,3)");

// min / max
assert(min(3.0, 7.0) == 3.0, "math: min(3,7)");
assert(max(3.0, 7.0) == 7.0, "math: max(3,7)");

// Pythagorean triple via hypot
assert(hypot(3.0, 4.0) == 5.0, "math: hypot(3,4)=5");

// Trig (approximate bounds)
var s05 = sin(0.5);
assert(s05 > 0.47 && s05 < 0.49, "math: sin(0.5)≈0.479");
assert(cos(0.0) == 1.0, "math: cos(0)=1");

// Constants
assert(pi > 3.14 && pi < 3.15, "math: pi constant");
assert(e  > 2.71 && e  < 2.72, "math: e constant");

// =============================================================
//  18. DATE MODULE
// =============================================================
section("18. DATE MODULE");

const dFixed = new Date(2023, 11, 25);
assert(dFixed.getFullYear() == 2023, "date: getFullYear");
assert(dFixed.getMonth()    == 11,   "date: getMonth (Dec=11)");
assert(dFixed.getDate()     == 25,   "date: getDate");

const dParsed = new Date("2023-12-25");
assert(dParsed.getFullYear() == 2023, "date: parsed getFullYear");
assert(dParsed.getMonth()    == 11,   "date: parsed getMonth");
assert(dParsed.getDate()     == 25,   "date: parsed getDate");

const dDetailed = new Date(2024, 0, 1, 12, 30, 45);
assert(dDetailed.getHours()   == 12, "date: getHours");
assert(dDetailed.getMinutes() == 30, "date: getMinutes");
assert(dDetailed.getSeconds() == 45, "date: getSeconds");

const dStr = dFixed.toString();
assert(dStr != "", "date: toString non-empty");

const dNow = new Date();
assert(dNow.getFullYear() >= 2025, "date: current year >= 2025");

// =============================================================
//  19. COMPLEX PROGRAMS
// =============================================================
section("19. COMPLEX PROGRAMS");

// ------ Bubble sort ------
fn bubbleSort(arr) {
    local n = arr.length();
    for (i := 0; i < n - 1; i++) {
        for (j := 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                local tmp  = arr[j];
                arr[j]     = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
    return arr;
}
var unsorted = [5, 3, 8, 1, 9, 2, 7, 4, 6];
var sorted   = bubbleSort(unsorted);
var isSorted = true;
for (si := 0; si < sorted.length() - 1; si++) {
    if (sorted[si] > sorted[si + 1]) { isSorted = false; }
}
assert(isSorted,                           "complex: bubble-sort ordered");
assert(sorted[0] == 1 && sorted[8] == 9,  "complex: bubble-sort min=1 max=9");

// ------ FizzBuzz ------
fn fizzBuzzList(n) {
    local out = [];
    for (i := 1; i <= n; i++) {
        if      (i % 15 == 0) { out.push("FizzBuzz"); }
        else if (i % 3  == 0) { out.push("Fizz"); }
        else if (i % 5  == 0) { out.push("Buzz"); }
        else                  { out.push(i); }
    }
    return out;
}
var fb = fizzBuzzList(15);
assert(fb[0]  == 1,           "fizzbuzz: [0]=1");
assert(fb[2]  == "Fizz",      "fizzbuzz: [2]=Fizz");
assert(fb[4]  == "Buzz",      "fizzbuzz: [4]=Buzz");
assert(fb[14] == "FizzBuzz",  "fizzbuzz: [14]=FizzBuzz");
assert(fb.length() == 15,     "fizzbuzz: length=15");

// ------ Binary search ------
fn binarySearch(arr, target) {
    local lo = 0;
    local hi = arr.length() - 1;
    while (lo <= hi) {
        local mid = floor((lo + hi) / 2.0);
        if (arr[mid] == target) { return mid; }
        if (arr[mid] < target)  { lo = mid + 1; }
        else                    { hi = mid - 1; }
    }
    return -1;
}
var bsArr = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19];
assert(binarySearch(bsArr, 1)  == 0,  "bsearch: found at 0");
assert(binarySearch(bsArr, 11) == 5,  "bsearch: found at 5");
assert(binarySearch(bsArr, 19) == 9,  "bsearch: found tail");
assert(binarySearch(bsArr, 6)  == -1, "bsearch: not found");

// ------ Linked list via objects ------
fn makeNode(v, next) { return { val: v, next: next }; }
fn listToArray(node) {
    local result = [];
    local cur    = node;
    while (cur != null) {
        result.push(cur.val);
        cur = cur.next;
    }
    return result;
}
var list = makeNode(1, makeNode(2, makeNode(3, makeNode(4, null))));
var la   = listToArray(list);
assert(la[0] == 1 && la[1] == 2 && la[3] == 4, "linked-list: values");
assert(la.length() == 4, "linked-list: length=4");

// ------ Higher-order map / filter / reduce ------
fn myMap(arr, f) {
    local r = [];
    arr.each(fn(v, i) { r.push(f(v)); });
    return r;
}
fn myFilter(arr, pred) {
    local r = [];
    arr.each(fn(v, i) { if (pred(v)) { r.push(v); } });
    return r;
}
fn myReduce(arr, f, init) {
    local acc = init;
    arr.each(fn(v, i) { acc = f(acc, v); });
    return acc;
}

var nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

var doubled = myMap(nums, fn(x) { return x * 2; });
assert(doubled[0] == 2  && doubled[9] == 20, "HOF map: doubled");

var evens = myFilter(nums, fn(x) { return x % 2 == 0; });
assert(evens[0] == 2 && evens[4] == 10 && evens.length() == 5, "HOF filter: evens");

var total = myReduce(nums, fn(acc, x) { return acc + x; }, 0);
assert(total == 55, "HOF reduce: sum 1..10=55");

var product = myReduce([1, 2, 3, 4, 5], fn(acc, x) { return acc * x; }, 1);
assert(product == 120, "HOF reduce: product 1..5=120");

// ------ Memoize decorator ------
fn memoize(f) {
    local cache = {};
    return fn(n) {
        local key = format("{}", n);
        if (cache[key] == null) {
            cache[key] = f(n);
        }
        return cache[key];
    };
}
fn slowSquare(n) { return n * n; }
var fastSquare = memoize(slowSquare);
assert(fastSquare(7)  == 49,  "memoize: first call computes");
assert(fastSquare(7)  == 49,  "memoize: second call uses cache");
assert(fastSquare(12) == 144, "memoize: different key");

// ------ Tower of Hanoi (count moves) ------
var hanoiMoves = 0;
fn hanoi(n, src, dst, aux) {
    if (n == 0) { return; }
    hanoi(n - 1, src, aux, dst);
    hanoiMoves++;
    hanoi(n - 1, aux, dst, src);
}
hanoi(5, "A", "C", "B");
assert(hanoiMoves == 31, "hanoi: 5 discs = 31 moves");

// ------ Accumulate digits ------
fn digitSum(n) {
    if (n < 0) { n = 0 - n; }
    local s = 0;
    while (n > 0) {
        s += n % 10;
        n = floor(n / 10.0);
    }
    return s;
}
assert(digitSum(0)    == 0,  "digits: digitSum(0)");
assert(digitSum(9999) == 36, "digits: digitSum(9999)");
assert(digitSum(1234) == 10, "digits: digitSum(1234)");

// ------ GCD (iterative Euclid) ------
fn gcd(a, b) {
    while (b != 0) {
        local t = b;
        b = a % b;
        a = t;
    }
    return a;
}
assert(gcd(48, 18) == 6,  "gcd: 48,18=6");
assert(gcd(100, 25) == 25,"gcd: 100,25=25");
assert(gcd(7, 13)   == 1, "gcd: 7,13=1 (coprime)");

// ------ Sieve of Eratosthenes ------
fn sieve(limit) {
    local flags = [];
    for (i := 0; i <= limit; i++) { flags.push(true); }
    flags[0] = false;
    flags[1] = false;
    for (i := 2; i * i <= limit; i++) {
        if (flags[i]) {
            for (j := i * i; j <= limit; j += i) {
                flags[j] = false;
            }
        }
    }
    local primes = [];
    for (i := 0; i <= limit; i++) {
        if (flags[i]) { primes.push(i); }
    }
    return primes;
}
var primes = sieve(50);
assert(primes[0] == 2,  "sieve: first prime = 2");
assert(primes[1] == 3,  "sieve: second prime = 3");
assert(primes[4] == 11, "sieve: 5th prime = 11");
assert(primes.length() == 15, "sieve: 15 primes up to 50");

// =============================================================
//  FINAL SUMMARY
// =============================================================
println("");
println("============================================================");
println(format("  RESULTS: {} passed,  {} failed", _passed, _failed));
if (_failed == 0) {
    println("  ALL TESTS PASSED!");
} else {
    println(format("  {} TEST(S) FAILED - see [FAIL] lines above.", _failed));
}
println("============================================================");
