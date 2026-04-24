import { println } from "core:io";
import { CryptoJS } from "lib:crypto";

assert CryptoJS.MD5("abc").toString(null) == "900150983cd24fb0d6963f7d28e17f72", "md5 abc";
assert CryptoJS.SHA1("abc").toString(null) == "a9993e364706816aba3e25717850c26c9cd0d89d", "sha1 abc";
assert CryptoJS.SHA256("abc").toString(null) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256 abc";
assert CryptoJS.HmacSHA256("The quick brown fox jumps over the lazy dog", "key").toString(null) == "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8", "hmac sha256";

const wa = CryptoJS.enc.Hex.parse("48656c6c6f");
assert CryptoJS.enc.Utf8.stringify(wa) == "Hello", "hex->utf8";
assert CryptoJS.enc.Base64.stringify(CryptoJS.enc.Utf8.parse("Hello")) == "SGVsbG8=", "utf8->base64";

println("crypto tests ok");
