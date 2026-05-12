import { println } from "core:io"; 

fn delay_err() async { raise "BOOM"; } 


fn parent() async { 
    try { 
        await delay_err(); 
    } catch(e) { 
        return 100; 
    } 
} 

parent()
    .then(fn(k) { 
        println("OK ", k); 
    })
    .error(fn(k) { 
        println("NOT OK ", k); 
    });
