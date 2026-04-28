import { println } from "core:io";


fn inner() async {
  raise ('Mother foccer');
}

fn outer() async {
  await inner()
  println("outer::ok!");
}

fn handler() async {
  await outer();
  println("handler::ok!");
}

handler();
println("Done!");