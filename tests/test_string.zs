import { println } from "core:io";
import {
    toUpper,
    toLower,
    capitalize,
    isIdentifier,
    isAlpha,
    isDigit,
    isAlnum,
    split,
    ord,
    chr,
    bytes,
    strip,
    join,
    replace,
    byteLength,
    encode,
    decode
} from "core:string";

assert toUpper("ab") == "AB", "toUpper";
assert toLower("AB") == "ab", "toLower";
assert capitalize("hello WORLD") == "Hello world", "capitalize";
assert isIdentifier("_x9") == true, "isIdentifier ok";
assert isIdentifier("9bad") == false, "isIdentifier bad";
assert isAlpha("abc") == true, "isAlpha";
assert isDigit("42") == true, "isDigit";
assert isAlnum("a1") == true, "isAlnum";
assert ord("X") == 88, "ord";
assert ord(chr(65)) == 65, "chr roundtrip";
assert byteLength("hello") == 5, "byteLength ascii";

var ws = split("  a  b  ");
assert ws != null, "split ws not null";

var parts = split("a,b,c", ",");
assert parts != null, "split delim not null";

var j = join(parts, "-");
assert j == "a-b-c", "join";

assert strip("  x  ") == "x", "strip";
assert replace("one one", "one", "two") == "two two", "replace";

var b = bytes("AB");
assert b != null, "bytes";

assert decode(encode("hello")) == "hello", "base64 roundtrip utf8";
assert encode(b) == "QUI=", "encode from byte array";

println("core:string ok");
