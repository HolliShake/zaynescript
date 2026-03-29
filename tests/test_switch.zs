import { println } from "core:io";

println(100 switch {
    case 10  => "Ten"
    case 20  => "Twenty"
    default  => "Nah!!"
    case 10, 30, 100 => "A Hundred"
});

const num = 0;

switch (num)  {
    case 0, 1: {
        println("1st case");
    }
    case 2, 3: {
        println("2nd case");
    }
    default: {
        println("None of them matched!");
    }
}