const println = console.log;

async function topLevel() {
  println("From top level");
  return "Hello";
}

async function asyncFn() {
  await topLevel();
  println("Called!!");
  return "Resolve me!";
}

async function callMe() {
  println(await asyncFn());
  println(await asyncFn());
  println(await asyncFn());
  return 1;
}

println(callMe());
println("AUTO");
println(callMe());

async function awaitable() {
  return "Hola!";
}

const v = awaitable()
  .then(function (v) {
    println(">>>>>>>>>>>>>>> From then", v);
    return 4;
  })
  .then(function (v) {
    println("waiting for>>", v);
    return "foocers";
  })
  .then(println);

println(">>", v);

async function toCall() {
  return 3;
}

async function callMeMaybe() {
  const r = await toCall();
  println(r, r, r, r, 1000);
  return 1;
}

callMeMaybe().then(function (v) {
  println("From callMeMaybe then", v);
});
