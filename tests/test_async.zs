import { println } from "core:io";

fn sync() async {
    const a = 1;
    return a + 4;
}

fn suspend() async {
    const x = await sync();
    println("Hello, Wordl!");
    return x;
}

const xx = suspend();
println(xx);

xx
.then(fn(d) {
    println("then::", d);
})
.then(fn(d) {
    return d + 1;
})
.then(fn(d) {
    println("then::", d);
})
.error(fn(e) {
    println("error:", e); 
});

println("Hola!");