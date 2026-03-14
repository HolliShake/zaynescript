import { println, format } from "core:io";
 
println(println);

for (x := 0; x < 20000; x++) {
    println(x);
}

while (x := 2;x < 30) {
    x++;
    println(x);
}

for (i := 1; i <= 10; i++) {
    local line = "";
    for (j := 1; j <= 10 - i; j++) {
        line += " ";
    }
    for (k := 1; k <= 2 * i - 1; k++) {
        line += "*";
    }
    println(line);
}

while (x := 0; x < 10) {
    while (y := x; y < 20; y++) {
        println("Y:>", y);
        if (y >= 10) break;
    }
    println("X:>", x);
    if (x == 8) break;
    x++;
}

for (i := 1; i <= 10; i++) {
    local row = "";
    for (j := 1; j <= 10; j++) {
        row += format("{} \t", (i * j));
    }
    println(row);
}

var hx = 0;

do {
    if (hx == 5) {
        ++hx;
        continue;
    }
    println(hx++);
} while (hx < 10)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

println(hx);

const xx = [1,2,3].each(println);

println(xx);
