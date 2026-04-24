import { format } from "core:io";
import { Object } from "core:object";
import { abs, sin } from "core:math";
import { len } from "core:utf8";

const _HEX = "0123456789abcdef";
const _B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const _PRINTABLE = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

// --- Internal Utilities ---

fn _u32(x) {
    local y = x % 4294967296;
    if (y < 0) { y = y + 4294967296; }
    return y;
}

fn _urshift(x, n) {
    if (n == 0) { return _u32(x); }
    local val = _u32(x);
    // Explicitly handle logical shift for the interpreter
    if (val < 2147483648) { return val >> n; }
    return (val >> n) | _u32(2147483648 >> (n - 1));
}

fn _rotl(x, n) { return _u32((x << n) | _urshift(x, 32 - n)); }
fn _rotr(x, n) { return _u32((_urshift(x, n)) | (x << (32 - n))); }
fn _not32(x) { return _u32(x ^ 4294967295); }

fn _ord(ch) {
    if (ch == "\n") { return 10; }
    if (ch == "\r") { return 13; }
    if (ch == "\t") { return 9; }
    local pLen = len(_PRINTABLE);
    for (i := 0; i < pLen; i++) {
        if (_PRINTABLE[i] == ch) { return i + 32; }
    }
    return 63;
}

fn _chr(code) {
    if (code == 10) { return "\n"; }
    if (code == 13) { return "\r"; }
    if (code == 9) { return "\t"; }
    if (code >= 32 && code <= 126) { return _PRINTABLE[code - 32]; }
    return "?";
}

// --- Encoding / Decoding ---

fn _strToBytes(s) {
    local out = [];
    local sLen = len(s);
    for (i := 0; i < sLen; i++) { out.push(_ord(s[i])); }
    return out;
}

fn _bytesToStr(bytes) {
    local s = "";
    local bLen = bytes.length();
    for (i := 0; i < bLen; i++) {
        s = format("{}{}", s, _chr(bytes[i] & 255));
    }
    return s;
}

fn _bytesToHex(bytes) {
    local out = "";
    local bLen = bytes.length();
    for (i := 0; i < bLen; i++) {
        local b = bytes[i] & 255;
        local hi = _urshift(b, 4) & 15;
        local lo = b & 15;
        out = format("{}{}{}", out, _HEX[hi], _HEX[lo]);
    }
    return out;
}

fn _hexNibble(ch) {
    for (i := 0; i < 16; i++) { if (_HEX[i] == ch) { return i; } }
    local up = "ABCDEF";
    for (j := 0; j < 6; j++) { if (up[j] == ch) { return j + 10; } }
    return 0;
}

fn _hexToBytes(hex) {
    local out = [];
    local hStr = format("{}", hex);
    local n = len(hStr);
    if (n % 2 == 1) {
        hStr = format("0{}", hStr);
        n = n + 1;
    }
    for (i := 0; i < n; i += 2) {
        out.push((_hexNibble(hStr[i]) << 4) | _hexNibble(hStr[i + 1]));
    }
    return out;
}

fn _bytesToBase64(bytes) {
    local out = "";
    local bLen = bytes.length();
    for (i := 0; i < bLen; i += 3) {
        local b0 = bytes[i] & 255;
        local b1 = (i + 1 < bLen) ? (bytes[i + 1] & 255) : -1;
        local b2 = (i + 2 < bLen) ? (bytes[i + 2] & 255) : -1;
        local n = (b0 << 16) | ((b1 < 0 ? 0 : b1) << 8) | (b2 < 0 ? 0 : b2);
        out = format("{}{}{}{}{}", out, _B64[_urshift(n, 18) & 63], _B64[_urshift(n, 12) & 63],
                     (b1 < 0 ? "=" : _B64[_urshift(n, 6) & 63]), (b2 < 0 ? "=" : _B64[n & 63]));
    }
    return out;
}

fn _base64ToBytes(s) {
    local clean = "";
    local sLen = len(s);
    for (i := 0; i < sLen; i++) {
        local ch = s[i];
        if (ch != " " && ch != "\n" && ch != "\r" && ch != "\t") { clean = format("{}{}", clean, ch); }
    }
    local out = [];
    local cLen = len(clean);
    for (j := 0; j < cLen; j += 4) {
        local n0 = _b64Index(clean[j]);
        local n1 = (j + 1 < cLen) ? _b64Index(clean[j+1]) : 0;
        local n2 = (j + 2 < cLen && clean[j+2] != "=") ? _b64Index(clean[j+2]) : -1;
        local n3 = (j + 3 < cLen && clean[j+3] != "=") ? _b64Index(clean[j+3]) : -1;
        local n = (n0 << 18) | (n1 << 12) | ((n2 < 0 ? 0 : n2) << 6) | (n3 < 0 ? 0 : n3);
        out.push(_urshift(n, 16) & 255);
        if (n2 >= 0) { out.push(_urshift(n, 8) & 255); }
        if (n3 >= 0) { out.push(n & 255); }
    }
    return out;
}

fn _b64Index(ch) {
    for (i := 0; i < 64; i++) { if (_B64[i] == ch) { return i; } }
    return -1;
}

// --- Formatting & Type Normalization ---

fn _toBytes(input) {
    if (input == null) { return []; }
    if (input.bytes != null) { return input.bytes; }
    if (typeof(input) == "list") { return input; }
    return _strToBytes(format("{}", input));
}

fn _wordArrayFromBytes(bytes) {
    local ownBytes = bytes;
    return {
        sigBytes: bytes.length(),
        bytes: ownBytes,
        toString: fn(encoder) {
            if (encoder == null || encoder.stringify == null) { return _bytesToHex(ownBytes); }
            return encoder.stringify({ bytes: ownBytes, sigBytes: ownBytes.length() });
        }
    };
}

// --- Hashing Logic ---

fn _md5Bytes(msgBytes) {
    local bytes = [];
    msgBytes.each(fn(b, i) { bytes.push(b & 255); });
    local bitLen = bytes.length() * 8;
    bytes.push(128);
    while ((bytes.length() % 64) != 56) { bytes.push(0); }
    for (i := 0; i < 8; i++) { bytes.push(_urshift(bitLen, i * 8) & 255); }

    local s = [7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
               5,9,14,20, 5,9,14,20, 5,9,14,20, 5,9,14,20,
               4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
               6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21];
    local K = [];
    for (k := 0; k < 64; k++) { K.push(_u32(abs(sin(k + 1)) * 4294967296.0)); }

    local a0 = 1732584193; local b0 = 4023233417;
    local c0 = 2562383102; local d0 = 271733878;

    for (off := 0; off < bytes.length(); off += 64) {
        local M = [];
        for (j := 0; j < 64; j += 4) {
            M.push(bytes[off + j] | (bytes[off + j + 1] << 8) | (bytes[off + j + 2] << 16) | (bytes[off + j + 3] << 24));
        }
        local A = a0; local B = b0; local C = c0; local D = d0;
        for (i := 0; i < 64; i++) {
            local F = 0; local g = 0;
            if (i < 16) { F = (B & C) | (_not32(B) & D); g = i; }
            else if (i < 32) { F = (D & B) | (_not32(D) & C); g = (5 * i + 1) % 16; }
            else if (i < 48) { F = B ^ C ^ D; g = (3 * i + 5) % 16; }
            else { F = C ^ (B | _not32(D)); g = (7 * i) % 16; }
            local t = D; D = C; C = B;
            B = _u32(B + _rotl(_u32(A + F + K[i] + M[g]), s[i]));
            A = t;
        }
        a0 = _u32(a0 + A); b0 = _u32(b0 + B); c0 = _u32(c0 + C); d0 = _u32(d0 + D);
    }
    local out = [];
    [a0, b0, c0, d0].each(fn(w, i) {
        out.push(w & 255); out.push(_urshift(w, 8) & 255);
        out.push(_urshift(w, 16) & 255); out.push(_urshift(w, 24) & 255);
    });
    return out;
}

fn _sha1Bytes(msgBytes) {
    local bytes = [];
    msgBytes.each(fn(b, i) { bytes.push(b & 255); });
    local bitLen = bytes.length() * 8;
    bytes.push(128);
    while ((bytes.length() % 64) != 56) { bytes.push(0); }
    for (i := 7; i >= 0; i--) { bytes.push(_urshift(bitLen, i * 8) & 255); }

    local h0 = 1732584193; local h1 = 4023233417; local h2 = 2562383102;
    local h3 = 271733878; local h4 = 3285377520;

    for (off := 0; off < bytes.length(); off += 64) {
        local w = [];
        for (i := 0; i < 16; i++) {
            local j = off + i * 4;
            w.push((bytes[j] << 24) | (bytes[j+1] << 16) | (bytes[j+2] << 8) | bytes[j+3]);
        }
        for (i := 16; i < 80; i++) { w.push(_rotl(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1)); }
        local a = h0; local b = h1; local c = h2; local d = h3; local e = h4;
        for (i := 0; i < 80; i++) {
            local f = 0; local k = 0;
            if (i < 20) { f = (b & c) | (_not32(b) & d); k = 1518500249; }
            else if (i < 40) { f = b ^ c ^ d; k = 1859775393; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 2400959708; }
            else { f = b ^ c ^ d; k = 3395469782; }
            local t = _u32(_rotl(a, 5) + f + e + k + w[i]);
            e = d; d = c; c = _rotl(b, 30); b = a; a = t;
        }
        h0 = _u32(h0 + a); h1 = _u32(h1 + b); h2 = _u32(h2 + c); h3 = _u32(h3 + d); h4 = _u32(h4 + e);
    }
    local out = [];
    [h0, h1, h2, h3, h4].each(fn(w, i) {
        out.push(_urshift(w, 24) & 255); out.push(_urshift(w, 16) & 255);
        out.push(_urshift(w, 8) & 255); out.push(w & 255);
    });
    return out;
}

fn _hmac(hashFn, blockSize, message, key) {
    local k = _toBytes(key);
    if (k.length() > blockSize) { k = hashFn(k); }
    while (k.length() < blockSize) { k.push(0); }
    local oKeyPad = []; local iKeyPad = [];
    for (i := 0; i < blockSize; i++) {
        oKeyPad.push((k[i] ^ 92) & 255);
        iKeyPad.push((k[i] ^ 54) & 255);
    }
    local inner = [];
    iKeyPad.each(fn(v, i) { inner.push(v); });
    _toBytes(message).each(fn(v, i) { inner.push(v); });
    local innerHash = hashFn(inner);
    local outer = [];
    oKeyPad.each(fn(v, i) { outer.push(v); });
    innerHash.each(fn(v, i) { outer.push(v); });
    return _wordArrayFromBytes(hashFn(outer));
}

// --- Public Interface ---

const enc = Object.freeze({
    Hex: Object.freeze({
        stringify: fn(wa) { return _bytesToHex(_toBytes(wa)); },
        parse: fn(s) { return _wordArrayFromBytes(_hexToBytes(s)); }
    }),
    Utf8: Object.freeze({
        stringify: fn(wa) { return _bytesToStr(_toBytes(wa)); },
        parse: fn(s) { return _wordArrayFromBytes(_strToBytes(s)); }
    }),
    Base64: Object.freeze({
        stringify: fn(wa) { return _bytesToBase64(_toBytes(wa)); },
        parse: fn(s) { return _wordArrayFromBytes(_base64ToBytes(s)); }
    })
});

const CryptoJS = Object.freeze({
    enc: enc,
    MD5: fn(m) { return _wordArrayFromBytes(_md5Bytes(_toBytes(m))); },
    SHA1: fn(m) { return _wordArrayFromBytes(_sha1Bytes(_toBytes(m))); },
    HmacMD5: fn(m, k) { return _hmac(_md5Bytes, 64, m, k); },
    HmacSHA1: fn(m, k) { return _hmac(_sha1Bytes, 64, m, k); }
});
