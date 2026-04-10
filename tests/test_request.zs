import { println  }  from  "core:io";
import { request, Method } from "lib:http";



request("http://localhost:3001/todos", {
        method: Method.GET
    })
    .then(fn(data) {
        println("1>>", data);
    });


request("http://localhost:3002/todos", {
        method: Method.GET
    })
    .then(fn(data) {
        println("2>>", data);
    });