import { Object } from  "core:object";
import { println } from  "core:io";


const frzn = Object.freeze({
    Age: 23,
    Name: "Philipp Andrew"
});

Object.keys(frzn).each(println);

println(frzn);

