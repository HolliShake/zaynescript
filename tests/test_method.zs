import { println, format } from "core:io";
import { abs, ceil, cos, floor, hypot, max, min, pow, round, sin, sqrt, pi, e } from "core:math";
import { Date } from "core:Date";

const arr = [1,2];

arr.push(3);

fn add(a,b) {
    return a + b;
}

println(arr, add(2,3));

arr.each(fn(a, b) {
    println(">>>>", a, b);
})


fn section(title) {
    println("");
    println(format("━━━  {}  ━━━", title));
}

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
println(dayName(1));

// Expression form
const sw1 = 100 switch {
    case 10  => "Ten"
    case 100 => "Hundred"
    default  => "Other"
};
assert sw1 == "Hundred", "switch-expr: exact match";

const sw2 = 999 switch {
    case 1, 2, 3 => "small"
    default      => "big"
};
assert sw2 == "big", "switch-expr: default";

const sw3 = 3 switch {
    case 1, 2, 3 => "hit"
    default      => "miss"
};
assert sw3 == "hit", "switch-expr: multi-value match";