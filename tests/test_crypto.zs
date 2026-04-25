import { println } from "core:io";
import { Crypto } from "lib:crypto";

assert Crypto.MD5("abc").toString(null) == "900150983cd24fb0d6963f7d28e17f72", "md5 abc";
assert Crypto.SHA1("abc").toString(null) == "a9993e364706816aba3e25717850c26c9cd0d89d", "sha1 abc";
assert Crypto.SHA256("abc").toString(null) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256 abc";
assert Crypto.HmacSHA256("The quick brown fox jumps over the lazy dog", "key").toString(null) == "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8", "hmac sha256";

const wa = Crypto.enc.Hex.parse("48656c6c6f");
assert Crypto.enc.Utf8.stringify(wa) == "Hello", "hex->utf8";
assert Crypto.enc.Base64.stringify(Crypto.enc.Utf8.parse("Hello")) == "SGVsbG8=", "utf8->base64";

assert Crypto.SHA512("abc").toString(null) == "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f", "sha512 abc";
assert Crypto.SHA3_256("").toString(null) == "a7ffc6f8bf1ed76651c14756a061fd662d285e046586db322b13ffc0a88b31fe", "sha3-256 empty";
assert Crypto.RIPEMD160("").toString(null) == "9c1185a5c5e9fc54612808977ee8f548b2258d31", "ripemd160 empty";

const dk = Crypto.PBKDF2("password", "salt", 1, 20, 1);
assert dk.toString(null) == "0c60c80f961f0e71f3a9b524af603126389fac30", "pbkdf2 hmac-sha1 rfc6070";

const k16 = Crypto.enc.Utf8.parse("0123456789abcdef");
const pt = "hello-aes-ecb";
const ct = Crypto.AesEcbEncrypt(k16, pt);
assert Crypto.AesEcbDecrypt(k16, ct).toString(Crypto.enc.Utf8) == pt, "aes-128 ecb roundtrip";

const iv = Crypto.enc.Hex.parse("000102030405060708090a0b0c0d0e0f");
const cbcCt = Crypto.AesCbcEncrypt(k16, iv, pt);
assert Crypto.AesCbcDecrypt(k16, iv, cbcCt).toString(Crypto.enc.Utf8) == pt, "aes-128 cbc roundtrip";

const b64uWa = Crypto.enc.Utf8.parse("foo");
assert Crypto.enc.Base64Url.stringify(b64uWa) == "Zm9v", "base64url";

println("crypto tests ok");
