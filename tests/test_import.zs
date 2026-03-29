import { greet } from "lib:request";
import "lib:nested/mod";
import { Dog, println } from "./test_class";

greet();

println(new Dog());
