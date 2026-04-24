import { Object } from "core:object";
import { abs, sin } from "core:math";
import { len } from "core:utf8";

const _HEX = "0123456789abcdef";
const _B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const _PRINTABLE = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

fn _u32(x) {
    local y = x % 4294967296;
    if (y < 0) { y += 4294967296; }
    return y;
}
fn _urshift(x, n) {
    if (n == 0) { return _u32(x); }
    if (x >= 0) { return x >> n; }
    return ((x & 2147483647) >> n) | (1 << (31 - n));
}
fn _rotl(x, n) { return _u32((x << n) | _urshift(x, 32 - n)); }
fn _rotr(x, n) { return _u32((_urshift(x, n)) | (x << (32 - n))); }
fn _not32(x) { return _u32(x ^ 4294967295); }

fn _ord(ch) {
    if (ch == "\n") { return 10; }
    if (ch == "\r") { return 13; }
    if (ch == "\t") { return 9; }
    for (i := 0; i < len(_PRINTABLE); i++) {
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

fn _strToBytes(s) {
    local out = [];
    for (i := 0; i < len(s); i++) { out.push(_ord(s[i])); }
    return out;
}
fn _bytesToStr(bytes) {
    local s = "";
    for (i := 0; i < bytes.length(); i++) { s += _chr(bytes[i] & 255); }
    return s;
}

fn _bytesToHex(bytes) {
    local out = "";
    for (i := 0; i < bytes.length(); i++) {
        local b = bytes[i] & 255;
        out += _HEX[_urshift(b, 4)];
        out += _HEX[b & 15];
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
    local n = len(hex);
    if (n % 2 == 1) { hex = "0" + hex; n = n + 1; }
    for (i := 0; i < n; i += 2) {
        out.push((_hexNibble(hex[i]) << 4) | _hexNibble(hex[i + 1]));
    }
    return out;
}

fn _b64Index(ch) {
    for (i := 0; i < len(_B64); i++) { if (_B64[i] == ch) { return i; } }
    return -1;
}
fn _bytesToBase64(bytes) {
    local out = "";
    for (i := 0; i < bytes.length(); i += 3) {
        local b0 = bytes[i] & 255;
        local b1 = i + 1 < bytes.length() ? (bytes[i + 1] & 255) : -1;
        local b2 = i + 2 < bytes.length() ? (bytes[i + 2] & 255) : -1;
        local n = (b0 << 16) | ((b1 < 0 ? 0 : b1) << 8) | (b2 < 0 ? 0 : b2);
        out += _B64[_urshift(n, 18) & 63];
        out += _B64[_urshift(n, 12) & 63];
        out += b1 < 0 ? "=" : _B64[_urshift(n, 6) & 63];
        out += b2 < 0 ? "=" : _B64[n & 63];
    }
    return out;
}
fn _base64ToBytes(s) {
    local clean = "";
    for (i := 0; i < len(s); i++) {
        local ch = s[i];
        if (ch != " " && ch != "\n" && ch != "\r" && ch != "\t") { clean += ch; }
    }
    local out = [];
    for (j := 0; j < len(clean); j += 4) {
        local c0 = clean[j];
        local c1 = j + 1 < len(clean) ? clean[j + 1] : "=";
        local c2 = j + 2 < len(clean) ? clean[j + 2] : "=";
        local c3 = j + 3 < len(clean) ? clean[j + 3] : "=";
        local n0 = _b64Index(c0);
        local n1 = _b64Index(c1);
        local n2 = c2 == "=" ? -1 : _b64Index(c2);
        local n3 = c3 == "=" ? -1 : _b64Index(c3);
        local n = (n0 << 18) | (n1 << 12) | ((n2 < 0 ? 0 : n2) << 6) | (n3 < 0 ? 0 : n3);
        out.push(_urshift(n, 16) & 255);
        if (n2 >= 0) { out.push(_urshift(n, 8) & 255); }
        if (n3 >= 0) { out.push(n & 255); }
    }
    return out;
}

fn _toBytes(input) {
    if (input == null) { return []; }
    if (input.sigBytes != null && input.bytes != null) { return input.bytes; }
    if (input.bytes != null) { return input.bytes; }
    return _strToBytes("" + input);
}
fn _wordArrayFromBytes(bytes) {
    local ownBytes = bytes;
    return {
        words: [],
        sigBytes: bytes.length(),
        bytes: bytes,
        toString: fn(encoder) {
            if (encoder == null || encoder.stringify == null) {
                return _bytesToHex(ownBytes);
            }
            return encoder.stringify({ bytes: ownBytes, sigBytes: ownBytes.length() });
        },
        clone: fn() {
            local cp = [];
            ownBytes.each(fn(b, i) { cp.push(b); });
            return _wordArrayFromBytes(cp);
        }
    };
}

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
    for (k := 0; k < 64; k++) {
        K.push(_u32(abs(sin(k + 1)) * 4294967296.0));
    }

    local a0 = 1732584193;
    local b0 = 4023233417;
    local c0 = 2562383102;
    local d0 = 271733878;

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
            local t = D;
            D = C;
            C = B;
            B = _u32(B + _rotl(_u32(A + F + K[i] + M[g]), s[i]));
            A = t;
        }
        a0 = _u32(a0 + A); b0 = _u32(b0 + B); c0 = _u32(c0 + C); d0 = _u32(d0 + D);
    }
    local out = [];
    [a0,b0,c0,d0].each(fn(w, i) {
        out.push(w & 255); out.push(_urshift(w, 8) & 255); out.push(_urshift(w, 16) & 255); out.push(_urshift(w, 24) & 255);
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

    local h0 = 1732584193, h1 = 4023233417, h2 = 2562383102, h3 = 271733878, h4 = 3285377520;
    for (off := 0; off < bytes.length(); off += 64) {
        local w = [];
        for (i := 0; i < 16; i++) {
            local j = off + i * 4;
            w.push((bytes[j] << 24) | (bytes[j + 1] << 16) | (bytes[j + 2] << 8) | bytes[j + 3]);
        }
        for (i := 16; i < 80; i++) { w.push(_rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1)); }
        local a = h0, b = h1, c = h2, d = h3, e = h4;
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
    [h0,h1,h2,h3,h4].each(fn(w, i) {
        out.push(_urshift(w, 24) & 255); out.push(_urshift(w, 16) & 255); out.push(_urshift(w, 8) & 255); out.push(w & 255);
    });
    return out;
}

fn _sha256Bytes(msgBytes) {
    local bytes = [];
    msgBytes.each(fn(b, i) { bytes.push(b & 255); });
    local bitLen = bytes.length() * 8;
    bytes.push(128);
    while ((bytes.length() % 64) != 56) { bytes.push(0); }
    for (i := 7; i >= 0; i--) { bytes.push(_urshift(bitLen, i * 8) & 255); }

    local K = [
        1116352408,1899447441,3049323471,3921009573,961987163,1508970993,2453635748,2870763221,
        3624381080,310598401,607225278,1426881987,1925078388,2162078206,2614888103,3248222580,
        3835390401,4022224774,264347078,604807628,770255983,1249150122,1555081692,1996064986,
        2554220882,2821834349,2952996808,3210313671,3336571891,3584528711,113926993,338241895,
        666307205,773529912,1294757372,1396182291,1695183700,1986661051,2177026350,2456956037,
        2730485921,2820302411,3259730800,3345764771,3516065817,3600352804,4094571909,275423344,
        430227734,506948616,659060556,883997877,958139571,1322822218,1537002063,1747873779,
        1955562222,2024104815,2227730452,2361852424,2428436474,2756734187,3204031479,3329325298
    ];
    local H = [1779033703,3144134277,1013904242,2773480762,1359893119,2600822924,528734635,1541459225];

    for (off := 0; off < bytes.length(); off += 64) {
        local w = [];
        for (i := 0; i < 16; i++) {
            local j = off + i * 4;
            w.push((bytes[j] << 24) | (bytes[j + 1] << 16) | (bytes[j + 2] << 8) | bytes[j + 3]);
        }
        for (i := 16; i < 64; i++) {
            local s0 = _rotr(w[i - 15], 7) ^ _rotr(w[i - 15], 18) ^ _urshift(w[i - 15], 3);
            local s1 = _rotr(w[i - 2], 17) ^ _rotr(w[i - 2], 19) ^ _urshift(w[i - 2], 10);
            w.push(_u32(w[i - 16] + s0 + w[i - 7] + s1));
        }
        local a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];
        for (i := 0; i < 64; i++) {
            local S1 = _rotr(e, 6) ^ _rotr(e, 11) ^ _rotr(e, 25);
            local ch = (e & f) ^ (_not32(e) & g);
            local t1 = _u32(h + S1 + ch + K[i] + w[i]);
            local S0 = _rotr(a, 2) ^ _rotr(a, 13) ^ _rotr(a, 22);
            local maj = (a & b) ^ (a & c) ^ (b & c);
            local t2 = _u32(S0 + maj);
            h = g; g = f; f = e; e = _u32(d + t1); d = c; c = b; b = a; a = _u32(t1 + t2);
        }
        H[0] = _u32(H[0] + a); H[1] = _u32(H[1] + b); H[2] = _u32(H[2] + c); H[3] = _u32(H[3] + d);
        H[4] = _u32(H[4] + e); H[5] = _u32(H[5] + f); H[6] = _u32(H[6] + g); H[7] = _u32(H[7] + h);
    }
    local out = [];
    H.each(fn(w, i) { out.push(_urshift(w, 24) & 255); out.push(_urshift(w, 16) & 255); out.push(_urshift(w, 8) & 255); out.push(w & 255); });
    return out;
}

fn _hmac(hashFn, blockSize, message, key) {
    local k = _toBytes(key);
    if (k.length() > blockSize) { k = hashFn(k); }
    while (k.length() < blockSize) { k.push(0); }
    local oKeyPad = []; local iKeyPad = [];
    for (i := 0; i < blockSize; i++) { oKeyPad.push((k[i] ^ 92) & 255); iKeyPad.push((k[i] ^ 54) & 255); }
    local msg = _toBytes(message);
    local inner = [];
    oKeyPad.each(fn(v, i) {});
    iKeyPad.each(fn(v, i) { inner.push(v); });
    msg.each(fn(v, i) { inner.push(v); });
    local innerHash = hashFn(inner);
    local outer = [];
    oKeyPad.each(fn(v, i) { outer.push(v); });
    innerHash.each(fn(v, i) { outer.push(v); });
    return _wordArrayFromBytes(hashFn(outer));
}

const enc = Object.freeze({
    Hex: Object.freeze({
        stringify: fn(wordArray) { return _bytesToHex(_toBytes(wordArray)); },
        parse: fn(hex) { return _wordArrayFromBytes(_hexToBytes("" + hex)); }
    }),
    Utf8: Object.freeze({
        stringify: fn(wordArray) { return _bytesToStr(_toBytes(wordArray)); },
        parse: fn(str) { return _wordArrayFromBytes(_strToBytes("" + str)); }
    }),
    Base64: Object.freeze({
        stringify: fn(wordArray) { return _bytesToBase64(_toBytes(wordArray)); },
        parse: fn(str) { return _wordArrayFromBytes(_base64ToBytes("" + str)); }
    })
});

const lib = Object.freeze({
    WordArray: Object.freeze({
        create: fn(words, sigBytes) {
            if (words == null) { return _wordArrayFromBytes([]); }
            if (words.bytes != null) { return _wordArrayFromBytes(words.bytes); }
            if (words.length() != null) {
                local bytes = [];
                words.each(fn(w, i) { bytes.push(w & 255); });
                return _wordArrayFromBytes(bytes);
            }
            return _wordArrayFromBytes(_toBytes(words));
        }
    })
});

const algo = Object.freeze({
    MD5: Object.freeze({ create: fn() { return { finalize: fn(msg) { return _wordArrayFromBytes(_md5Bytes(_toBytes(msg))); } }; } }),
    SHA1: Object.freeze({ create: fn() { return { finalize: fn(msg) { return _wordArrayFromBytes(_sha1Bytes(_toBytes(msg))); } }; } }),
    SHA256: Object.freeze({ create: fn() { return { finalize: fn(msg) { return _wordArrayFromBytes(_sha256Bytes(_toBytes(msg))); } }; } })
});

fn _unsupportedCipher(name) {
    return {
        encrypt: fn(message, key, cfg) { return null; },
        decrypt: fn(ciphertext, key, cfg) { return null; }
    };
}

const mode = Object.freeze({ CBC: "CBC", CFB: "CFB", CTR: "CTR", ECB: "ECB", OFB: "OFB" });
const pad = Object.freeze({ Pkcs7: "Pkcs7", AnsiX923: "AnsiX923", Iso10126: "Iso10126", Iso97971: "Iso97971", ZeroPadding: "ZeroPadding", NoPadding: "NoPadding" });

const CryptoJS = Object.freeze({
    enc: enc,
    lib: lib,
    algo: algo,
    mode: mode,
    pad: pad,
    MD5: fn(msg) { return _wordArrayFromBytes(_md5Bytes(_toBytes(msg))); },
    SHA1: fn(msg) { return _wordArrayFromBytes(_sha1Bytes(_toBytes(msg))); },
    SHA256: fn(msg) { return _wordArrayFromBytes(_sha256Bytes(_toBytes(msg))); },
    HmacMD5: fn(msg, key) { return _hmac(_md5Bytes, 64, msg, key); },
    HmacSHA1: fn(msg, key) { return _hmac(_sha1Bytes, 64, msg, key); },
    HmacSHA256: fn(msg, key) { return _hmac(_sha256Bytes, 64, msg, key); },
    AES: _unsupportedCipher("AES"),
    DES: _unsupportedCipher("DES"),
    TripleDES: _unsupportedCipher("TripleDES"),
    Rabbit: _unsupportedCipher("Rabbit"),
    RC4: _unsupportedCipher("RC4")
});
