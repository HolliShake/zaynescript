import { println } from "core:io";
 
println(println);

for (x := 0; x < 10000; x++) {
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