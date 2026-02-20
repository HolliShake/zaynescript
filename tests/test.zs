import { println, scan, parseNum } from "core:io";
import "core:math";


fn a() {
    let x = 2, y = x, z = y;
    b();
    println("Calling...");
    println("Called", x, y);
}

fn b() {
    for (xx := 0; xx < 5; xx = xx + 1) {
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
    println("Done b");
}

a();
