import { println } from "core:io";

println(100 switch {
    case 10  => "Ten"
    case 20  => "Twenty"
    default  => "Nah!!"
    case 10, 30, 100 => "A Hundred"
});