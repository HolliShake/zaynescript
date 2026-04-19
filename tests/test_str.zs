import { println, format } from "core:io";

const a = "Hello";
const b = "World";
assert a + ", " + b == "Hello, World", "str: concat";

var buf = "";
buf += "foo";
buf += "bar";
assert buf == "foobar", "str: +=";

assert format("{}={}", "x", 42) == "x=42", "str: format";

println("str tests ok");
