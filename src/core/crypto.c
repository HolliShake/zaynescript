#include "./crypto.h"
#include "./crypto_algo.h"

/**
 * @file crypto.c
 * @brief Pure-C digests, HMAC, encoders, and the @c core:crypto module.
 *
 * The @c Crypto export and WordArray / @c enc layout follow the public CryptoJS
 * API documented at https://github.com/brix/crypto-js — this is an original C
 * implementation of those algorithms and helpers, not a translation of the
 * CryptoJS JavaScript sources.
 */

/** One WordArray class per interpreter (digest + @c toString, see @c file). */
static Value*		s_word_array_class	= NULL;
static Interpreter* s_word_array_interp = NULL;

/* utf8proc-backed helpers (defined in utf8.c, not exported from utf8.h) */
Rune* Utf8Core_Utf8ToRunes(const char* utf8);

/* -------------------------------------------------------------------------- */
/* 32-bit helpers (RFC 1321 MD5-style uint32 arithmetic)                      */
/* -------------------------------------------------------------------------- */

static inline uint32_t u32(uint64_t x) {
	return (uint32_t) (x & 0xffffffffu);
}

static inline uint32_t rol32(uint32_t x, int n) {
	return u32(((uint64_t) x << n) | ((uint64_t) x >> (32 - n)));
}

static inline uint32_t ror32(uint32_t x, int n) {
	return u32(((uint64_t) x >> n) | ((uint64_t) x << (32 - n)));
}

static inline uint32_t shr32(uint32_t x, int n) {
	return x >> n;
}

/* -------------------------------------------------------------------------- */
/* MD5 (RFC 1321)                                                             */
/* -------------------------------------------------------------------------- */

void crypto_md5(const uint8_t* data,
				size_t		   len,
				uint8_t		   out[CRYPTO_MD5_DIGEST_LEN]) {
	uint32_t a0 = 0x67452301u;
	uint32_t b0 = 0xefcdab89u;
	uint32_t c0 = 0x98badcfeu;
	uint32_t d0 = 0x10325476u;

	static const uint32_t K[64] = {
		0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu, 0xf57c0fafu,
		0x4787c62au, 0xa8304613u, 0xfd469501u, 0x698098d8u, 0x8b44f7afu,
		0xffff5bb1u, 0x895cd7beu, 0x6b901122u, 0xfd987193u, 0xa679438eu,
		0x49b40821u, 0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau,
		0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u, 0x21e1cde6u,
		0xc33707d6u, 0xf4d50d87u, 0x455a14edu, 0xa9e3e905u, 0xfcefa3f8u,
		0x676f02d9u, 0x8d2a4c8au, 0xfffa3942u, 0x8771f681u, 0x6d9d6122u,
		0xfde5380cu, 0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
		0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u, 0xd9d4d039u,
		0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u, 0xf4292244u, 0x432aff97u,
		0xab9423a7u, 0xfc93a039u, 0x655b59c3u, 0x8f0ccc92u, 0xffeff47du,
		0x85845dd1u, 0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u,
		0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u
	};
	static const int s[64] = { 7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17,
							   22, 7,  12, 17, 22, 5,  9,  14, 20, 5,  9,
							   14, 20, 5,  9,  14, 20, 5,  9,  14, 20, 4,
							   11, 16, 23, 4,  11, 16, 23, 4,  11, 16, 23,
							   4,  11, 16, 23, 6,  10, 15, 21, 6,  10, 15,
							   21, 6,  10, 15, 21, 6,  10, 15, 21 };

	size_t	 new_len = ((len + 8u) / 64u + 1u) * 64u;
	uint8_t* msg	 = Allocate(new_len);
	memcpy(msg, data, len);
	msg[len] = 0x80;
	memset(msg + len + 1, 0, new_len - len - 1);

	uint64_t bit_len = (uint64_t) len * 8u;
	for (int i = 0; i < 8; i++) {
		msg[new_len - 8 + (size_t) i] = (uint8_t) (bit_len >> (8 * i));
	}

	for (size_t off = 0; off < new_len; off += 64) {
		uint32_t M[16];
		for (int j = 0; j < 16; j++) {
			const uint8_t* p = msg + off + (size_t) j * 4u;
			M[j] = (uint32_t) p[0] | ((uint32_t) p[1] << 8)
				   | ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
		}

		uint32_t A = a0, B = b0, C = c0, D = d0;
		for (int i = 0; i < 64; i++) {
			uint32_t F, g;
			if (i < 16) {
				F = u32((B & C) | ((~B) & D));
				g = (uint32_t) i;
			} else if (i < 32) {
				F = u32((D & B) | ((~D) & C));
				g = u32((5u * (uint32_t) i + 1u) % 16u);
			} else if (i < 48) {
				F = u32(B ^ C ^ D);
				g = u32((3u * (uint32_t) i + 5u) % 16u);
			} else {
				F = u32(C ^ (B | (~D)));
				g = u32((7u * (uint32_t) i) % 16u);
			}
			uint32_t T = D;
			D		   = C;
			C		   = B;
			B		   = u32(B + rol32(u32(A + F + K[i] + M[g]), s[i]));
			A		   = T;
		}
		a0 = u32(a0 + A);
		b0 = u32(b0 + B);
		c0 = u32(c0 + C);
		d0 = u32(d0 + D);
	}

	free(msg);

	uint32_t words[4] = { a0, b0, c0, d0 };
	for (int i = 0; i < 4; i++) {
		out[i * 4 + 0] = (uint8_t) (words[i] & 0xffu);
		out[i * 4 + 1] = (uint8_t) ((words[i] >> 8) & 0xffu);
		out[i * 4 + 2] = (uint8_t) ((words[i] >> 16) & 0xffu);
		out[i * 4 + 3] = (uint8_t) ((words[i] >> 24) & 0xffu);
	}
}

/* -------------------------------------------------------------------------- */
/* SHA-1 (FIPS 180-1)                                                         */
/* -------------------------------------------------------------------------- */

void crypto_sha1(const uint8_t* data,
				 size_t			len,
				 uint8_t		out[CRYPTO_SHA1_DIGEST_LEN]) {
	uint32_t h0 = 0x67452301u;
	uint32_t h1 = 0xefcdab89u;
	uint32_t h2 = 0x98badcfeu;
	uint32_t h3 = 0x10325476u;
	uint32_t h4 = 0xc3d2e1f0u;

	size_t	 new_len = ((len + 8u) / 64u + 1u) * 64u;
	uint8_t* msg	 = Allocate(new_len);
	memcpy(msg, data, len);
	msg[len] = 0x80;
	memset(msg + len + 1, 0, new_len - len - 1);

	uint64_t bit_len = (uint64_t) len * 8u;
	for (int i = 0; i < 8; i++) {
		msg[new_len - 8 + (size_t) i] = (uint8_t) (bit_len >> (56 - 8 * i));
	}

	for (size_t off = 0; off < new_len; off += 64) {
		uint32_t w[80];
		for (int i = 0; i < 16; i++) {
			const uint8_t* p = msg + off + (size_t) i * 4u;
			w[i]			 = ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16)
							   | ((uint32_t) p[2] << 8) | (uint32_t) p[3];
		}
		for (int i = 16; i < 80; i++) {
			w[i] = rol32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
		}

		uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
		for (int i = 0; i < 80; i++) {
			uint32_t f, k;
			if (i < 20) {
				f = u32((b & c) | ((~b) & d));
				k = 0x5a827999u;
			} else if (i < 40) {
				f = u32(b ^ c ^ d);
				k = 0x6ed9eba1u;
			} else if (i < 60) {
				f = u32((b & c) | (b & d) | (c & d));
				k = 0x8f1bbcdcu;
			} else {
				f = u32(b ^ c ^ d);
				k = 0xca62c1d6u;
			}
			uint32_t t = u32(rol32(a, 5) + f + e + k + w[i]);
			e		   = d;
			d		   = c;
			c		   = rol32(b, 30);
			b		   = a;
			a		   = t;
		}
		h0 = u32(h0 + a);
		h1 = u32(h1 + b);
		h2 = u32(h2 + c);
		h3 = u32(h3 + d);
		h4 = u32(h4 + e);
	}

	free(msg);

	uint32_t H[5] = { h0, h1, h2, h3, h4 };
	for (int i = 0; i < 5; i++) {
		out[i * 4 + 0] = (uint8_t) ((H[i] >> 24) & 0xffu);
		out[i * 4 + 1] = (uint8_t) ((H[i] >> 16) & 0xffu);
		out[i * 4 + 2] = (uint8_t) ((H[i] >> 8) & 0xffu);
		out[i * 4 + 3] = (uint8_t) (H[i] & 0xffu);
	}
}

/* -------------------------------------------------------------------------- */
/* SHA-256 (FIPS 180-4)                                                       */
/* -------------------------------------------------------------------------- */

static inline uint32_t sha256_ch(uint32_t x, uint32_t y, uint32_t z) {
	return u32((x & y) ^ ((~x) & z));
}

static inline uint32_t sha256_maj(uint32_t x, uint32_t y, uint32_t z) {
	return u32((x & y) ^ (x & z) ^ (y & z));
}

static inline uint32_t sha256_bsig0(uint32_t x) {
	return u32(ror32(x, 2) ^ ror32(x, 13) ^ ror32(x, 22));
}

static inline uint32_t sha256_bsig1(uint32_t x) {
	return u32(ror32(x, 6) ^ ror32(x, 11) ^ ror32(x, 25));
}

static inline uint32_t sha256_ssig0(uint32_t x) {
	return u32(ror32(x, 7) ^ ror32(x, 18) ^ shr32(x, 3));
}

static inline uint32_t sha256_ssig1(uint32_t x) {
	return u32(ror32(x, 17) ^ ror32(x, 19) ^ shr32(x, 10));
}

void crypto_sha256(const uint8_t* data,
				   size_t		  len,
				   uint8_t		  out[CRYPTO_SHA256_DIGEST_LEN]) {
	static const uint32_t K[64] = {
		0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu,
		0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u,
		0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u,
		0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
		0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u,
		0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
		0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
		0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
		0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u,
		0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u, 0x1e376c08u,
		0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu,
		0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
		0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
	};

	uint32_t h0 = 0x6a09e667u;
	uint32_t h1 = 0xbb67ae85u;
	uint32_t h2 = 0x3c6ef372u;
	uint32_t h3 = 0xa54ff53au;
	uint32_t h4 = 0x510e527fu;
	uint32_t h5 = 0x9b05688cu;
	uint32_t h6 = 0x1f83d9abu;
	uint32_t h7 = 0x5be0cd19u;

	size_t	 new_len = ((len + 8u) / 64u + 1u) * 64u;
	uint8_t* msg	 = Allocate(new_len);
	memcpy(msg, data, len);
	msg[len] = 0x80;
	memset(msg + len + 1, 0, new_len - len - 1);

	uint64_t bit_len = (uint64_t) len * 8u;
	for (int i = 0; i < 8; i++) {
		msg[new_len - 8 + (size_t) i] = (uint8_t) (bit_len >> (56 - 8 * i));
	}

	for (size_t off = 0; off < new_len; off += 64) {
		uint32_t w[64];
		for (int i = 0; i < 16; i++) {
			const uint8_t* p = msg + off + (size_t) i * 4u;
			w[i]			 = ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16)
							   | ((uint32_t) p[2] << 8) | (uint32_t) p[3];
		}
		for (int i = 16; i < 64; i++) {
			w[i] = u32(sha256_ssig1(w[i - 2]) + w[i - 7]
					   + sha256_ssig0(w[i - 15]) + w[i - 16]);
		}

		uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6,
				 hh = h7;
		for (int i = 0; i < 64; i++) {
			uint32_t t1 =
				u32(hh + sha256_bsig1(e) + sha256_ch(e, f, g) + K[i] + w[i]);
			uint32_t t2 = u32(sha256_bsig0(a) + sha256_maj(a, b, c));
			hh			= g;
			g			= f;
			f			= e;
			e			= u32(d + t1);
			d			= c;
			c			= b;
			b			= a;
			a			= u32(t1 + t2);
		}
		h0 = u32(h0 + a);
		h1 = u32(h1 + b);
		h2 = u32(h2 + c);
		h3 = u32(h3 + d);
		h4 = u32(h4 + e);
		h5 = u32(h5 + f);
		h6 = u32(h6 + g);
		h7 = u32(h7 + hh);
	}

	free(msg);

	uint32_t H[8] = { h0, h1, h2, h3, h4, h5, h6, h7 };
	for (int i = 0; i < 8; i++) {
		out[i * 4 + 0] = (uint8_t) ((H[i] >> 24) & 0xffu);
		out[i * 4 + 1] = (uint8_t) ((H[i] >> 16) & 0xffu);
		out[i * 4 + 2] = (uint8_t) ((H[i] >> 8) & 0xffu);
		out[i * 4 + 3] = (uint8_t) (H[i] & 0xffu);
	}
}

/* -------------------------------------------------------------------------- */
/* HMAC (FIPS 198-1)                                                          */
/* -------------------------------------------------------------------------- */

static void hmac_inner(size_t block_len,
					   void (*hash)(const uint8_t*, size_t, uint8_t*),
					   size_t		  digest_len,
					   const uint8_t* key,
					   size_t		  key_len,
					   const uint8_t* msg,
					   size_t		  msg_len,
					   uint8_t*		  out) {
	uint8_t* kbuf = Allocate(block_len);
	memset(kbuf, 0, block_len);
	if (key_len > block_len) {
		hash(key, key_len, kbuf);
		key_len = digest_len;
	} else {
		memcpy(kbuf, key, key_len);
	}

	uint8_t* ipad = Allocate(block_len);
	uint8_t* opad = Allocate(block_len);
	for (size_t i = 0; i < block_len; i++) {
		ipad[i] = (uint8_t) (kbuf[i] ^ 0x36u);
		opad[i] = (uint8_t) (kbuf[i] ^ 0x5cu);
	}
	free(kbuf);

	size_t	 inner_len = block_len + msg_len;
	uint8_t* inner	   = Allocate(inner_len);
	memcpy(inner, ipad, block_len);
	memcpy(inner + block_len, msg, msg_len);
	free(ipad);

	uint8_t inner_hash[32];
	hash(inner, inner_len, inner_hash);
	free(inner);

	size_t	 outer_len = block_len + digest_len;
	uint8_t* outer	   = Allocate(outer_len);
	memcpy(outer, opad, block_len);
	memcpy(outer + block_len, inner_hash, digest_len);
	free(opad);

	hash(outer, outer_len, out);
	free(outer);
}

void crypto_hmac_md5(const uint8_t* key,
					 size_t			key_len,
					 const uint8_t* msg,
					 size_t			msg_len,
					 uint8_t		out[CRYPTO_MD5_DIGEST_LEN]) {
	hmac_inner(64,
			   crypto_md5,
			   CRYPTO_MD5_DIGEST_LEN,
			   key,
			   key_len,
			   msg,
			   msg_len,
			   out);
}

void crypto_hmac_sha1(const uint8_t* key,
					  size_t		 key_len,
					  const uint8_t* msg,
					  size_t		 msg_len,
					  uint8_t		 out[CRYPTO_SHA1_DIGEST_LEN]) {
	hmac_inner(64,
			   crypto_sha1,
			   CRYPTO_SHA1_DIGEST_LEN,
			   key,
			   key_len,
			   msg,
			   msg_len,
			   out);
}

void crypto_hmac_sha256(const uint8_t* key,
						size_t		   key_len,
						const uint8_t* msg,
						size_t		   msg_len,
						uint8_t		   out[CRYPTO_SHA256_DIGEST_LEN]) {
	hmac_inner(64,
			   crypto_sha256,
			   CRYPTO_SHA256_DIGEST_LEN,
			   key,
			   key_len,
			   msg,
			   msg_len,
			   out);
}

/* -------------------------------------------------------------------------- */
/* Hex / Base64                                                               */
/* -------------------------------------------------------------------------- */

void crypto_hex_stringify(const uint8_t* data, size_t len, char* out) {
	static const char* hexd = "0123456789abcdef";
	for (size_t i = 0; i < len; i++) {
		uint8_t b	   = data[i];
		out[i * 2]	   = hexd[b >> 4];
		out[i * 2 + 1] = hexd[b & 15];
	}
	out[len * 2] = '\0';
}

static int hex_digit(int c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

uint8_t* crypto_hex_parse(const char* hex, size_t* out_len) {
	if (hex == NULL) {
		return NULL;
	}
	size_t n = strlen(hex);
	while (n > 0
		   && (hex[n - 1] == ' ' || hex[n - 1] == '\t' || hex[n - 1] == '\n'
			   || hex[n - 1] == '\r')) {
		n--;
	}
	size_t start = 0;
	while (start < n && (hex[start] == ' ' || hex[start] == '\t')) {
		start++;
	}
	size_t m = n - start;
	if (m == 0) {
		uint8_t* z = Allocate(1);
		*out_len   = 0;
		return z;
	}

	size_t	 blen = (m + 1u) / 2u;
	uint8_t* buf  = Allocate(blen ? blen : 1);
	size_t	 i	  = 0;
	size_t	 p	  = start;
	if (m % 2u == 1u) {
		int v = hex_digit((unsigned char) hex[p++]);
		if (v < 0) {
			free(buf);
			return NULL;
		}
		buf[i++] = (uint8_t) v;
		m--;
	}
	for (; m >= 2; m -= 2, p += 2) {
		int hi = hex_digit((unsigned char) hex[p]);
		int lo = hex_digit((unsigned char) hex[p + 1]);
		if (hi < 0 || lo < 0) {
			free(buf);
			return NULL;
		}
		buf[i++] = (uint8_t) ((hi << 4) | lo);
	}
	*out_len = i;
	return buf;
}

size_t crypto_base64_encode(const uint8_t* data, size_t len, char* out) {
	static const char* b64 =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	size_t o = 0;
	for (size_t i = 0; i < len; i += 3) {
		uint32_t n	  = (uint32_t) data[i] << 16;
		int		 has1 = (i + 1 < len);
		int		 has2 = (i + 2 < len);
		if (has1) {
			n |= (uint32_t) data[i + 1] << 8;
		}
		if (has2) {
			n |= (uint32_t) data[i + 2];
		}
		out[o++] = b64[(n >> 18) & 63];
		out[o++] = b64[(n >> 12) & 63];
		out[o++] = has1 ? b64[(n >> 6) & 63] : '=';
		out[o++] = has2 ? b64[n & 63] : '=';
	}
	out[o] = '\0';
	return o;
}

static int b64_index(int c) {
	if (c >= 'A' && c <= 'Z') {
		return c - 'A';
	}
	if (c >= 'a' && c <= 'z') {
		return c - 'a' + 26;
	}
	if (c >= '0' && c <= '9') {
		return c - '0' + 52;
	}
	if (c == '+') {
		return 62;
	}
	if (c == '/') {
		return 63;
	}
	return -1;
}

uint8_t* crypto_base64_decode(const char* b64, size_t* out_len) {
	if (b64 == NULL) {
		return NULL;
	}
	size_t cap	 = strlen(b64) + 4;
	char*  clean = Allocate(cap);
	size_t c	 = 0;
	for (const char* p = b64; *p; p++) {
		if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') {
			continue;
		}
		clean[c++] = *p;
	}
	clean[c] = '\0';

	if (c % 4u != 0u) {
		free(clean);
		return NULL;
	}

	size_t	 maxout = (c / 4u) * 3u;
	uint8_t* out	= Allocate(maxout ? maxout : 1);
	size_t	 o		= 0;

	for (size_t j = 0; j + 4 <= c; j += 4) {
		int n0 = b64_index((unsigned char) clean[j]);
		int n1 = b64_index((unsigned char) clean[j + 1]);
		int n2 =
			(clean[j + 2] == '=') ? 0 : b64_index((unsigned char) clean[j + 2]);
		int n3 =
			(clean[j + 3] == '=') ? 0 : b64_index((unsigned char) clean[j + 3]);
		if (n0 < 0 || n1 < 0 || n2 < 0 || n3 < 0) {
			free(clean);
			free(out);
			return NULL;
		}
		uint32_t n = ((uint32_t) n0 << 18) | ((uint32_t) n1 << 12)
					 | ((uint32_t) n2 << 6) | (uint32_t) n3;
		out[o++]   = (uint8_t) ((n >> 16) & 0xffu);
		if (clean[j + 2] != '=') {
			out[o++] = (uint8_t) ((n >> 8) & 0xffu);
		}
		if (clean[j + 3] != '=') {
			out[o++] = (uint8_t) (n & 0xffu);
		}
	}
	free(clean);
	*out_len = o;
	return out;
}

/* -------------------------------------------------------------------------- */
/* Interpreter: WordArray + core:crypto                                       */
/* -------------------------------------------------------------------------- */

#define LX_ENC_TAG "__lxEnc"

static Value*
_CryptoWordArrayToString(Interpreter* interpreter, int argc, Value** arguments);

static void _ensure_word_array_class(Interpreter* interp) {
	if (s_word_array_class != NULL && s_word_array_interp == interp) {
		return;
	}
	Value* waCls =
		NewClassValue(interp, CreateUserClass("WordArray", interp->Object));
	Class* cls = CoerceToUserClass(waCls);
	ClassDefineMemberByString(
		cls,
		"toString",
		NewNativeFunctionValue(
			interp,
			CreateNativeFunctionMeta("toString",
									 VARARG,
									 _CryptoWordArrayToString)),
		false);
	s_word_array_class	= waCls;
	s_word_array_interp = interp;
}

static Value*
_make_word_array(Interpreter* interp, const uint8_t* data, size_t len) {
	_ensure_word_array_class(interp);

	Value* bytesArr = NewArrayValue(interp);
	Array* arr		= CoerceToArray(bytesArr);
	for (size_t i = 0; i < len; i++) {
		ArrayPush(arr, NewIntValue(interp, (int) data[i]));
	}

	ClassInstance* inst = CreateClassInstance(s_word_array_class);
	HashMapSet(inst->Members, "bytes", bytesArr);
	HashMapSet(inst->Members, "sigBytes", NewIntValue(interp, (int) len));
	return NewClassInstanceValue(interp, inst);
}

static int _bytes_from_value(Interpreter* interp,
							 Value*		  v,
							 uint8_t**	  out,
							 size_t*	  len,
							 String*	  err) {
	*out = NULL;
	*len = 0;
	*err = NULL;

	if (v == NULL || ValueIsNull(v)) {
		uint8_t* z = Allocate(1);
		z[0]	   = 0;
		*out	   = z;
		*len	   = 0;
		return 0;
	}

	if (ValueIsStr(v)) {
		String u8 = ValueToString(v);
		if (u8 == NULL) {
			*err =
				FormatString("%s: crypto message out of memory", MEMORY_ERROR);
			return -1;
		}
		size_t l = strlen(u8);
		*out	 = Allocate(l + 1);
		memcpy(*out, u8, l + 1);
		free(u8);
		*len = l;
		return 0;
	}

	if (ValueIsArray(v)) {
		Array* a = CoerceToArray(v);
		*out	 = Allocate(a->Count + 1);
		for (size_t i = 0; i < a->Count; i++) {
			Value* it = (Value*) ArrayGet(a, i);
			if (!ValueIsInt(it) && !ValueIsNum(it)) {
				free(*out);
				*out = NULL;
				*err =
					FormatString("%s: crypto byte array must contain numbers",
								 TYPE_ERROR);
				return -1;
			}
			(*out)[i] = (uint8_t) (CoerceToI32(it) & 0xff);
		}
		*len = a->Count;
		return 0;
	}

	if (ValueIsClassInstance(v)) {
		ClassInstance* ci = CoerceToClassInstance(v);
		Value*		   bv = (Value*) HashMapGet(ci->Members, "bytes");
		if (bv != NULL) {
			return _bytes_from_value(interp, bv, out, len, err);
		}
	}

	if (ValueIsObject(v)) {
		HashMap* hm = CoerceToHashMap(v);
		Value*	 bv = (Value*) HashMapGet(hm, "bytes");
		if (bv != NULL) {
			return _bytes_from_value(interp, bv, out, len, err);
		}
	}

	String s = ValueToString(v);
	if (s == NULL) {
		*err = FormatString("%s: crypto coerce out of memory", MEMORY_ERROR);
		return -1;
	}
	size_t l = strlen(s);
	*out	 = Allocate(l + 1);
	memcpy(*out, s, l + 1);
	free(s);
	*len = l;
	return 0;
}

static Value* _CryptoWordArrayToString(Interpreter* interpreter,
									   int			argc,
									   Value**		arguments) {
	if (argc < 1 || argc > 2) {
		return NewErrorFValue(
			interpreter,
			"%s: WordArray.toString expects this and optional encoder",
			ARGUMENT_ERROR);
	}
	/* Same layout as RegExp.search: arguments[0] is @c this. */
	Value* thisWa = arguments[0];
	Value* encv	  = (argc >= 2) ? arguments[1] : NULL;

	if (!ValueIsClassInstance(thisWa)) {
		return NewErrorFValue(
			interpreter,
			"%s: WordArray.toString requires WordArray instance",
			TYPE_ERROR);
	}
	ClassInstance* ci = CoerceToClassInstance(thisWa);

	int mode = 0;
	if (encv != NULL && !ValueIsNull(encv) && ValueIsObject(encv)) {
		HashMap* em	 = CoerceToHashMap(encv);
		Value*	 tag = (Value*) HashMapGet(em, LX_ENC_TAG);
		if (tag != NULL && ValueIsInt(tag)) {
			mode = CoerceToI32(tag);
		}
	}

	uint8_t* raw	= NULL;
	size_t	 rawlen = 0;
	String	 err	= NULL;
	Value*	 bv		= (Value*) HashMapGet(ci->Members, "bytes");
	if (bv == NULL
		|| _bytes_from_value(interpreter, bv, &raw, &rawlen, &err) != 0) {
		if (err) {
			Value* e = NewErrorValue(interpreter, err);
			free(err);
			return e;
		}
		return NewErrorFValue(interpreter,
							  "%s: WordArray missing bytes",
							  ATTRIBUTE_ERROR);
	}

	Value* outv = NULL;
	if (encv == NULL || ValueIsNull(encv) || mode == 0) {
		size_t hlen = rawlen * 2 + 1;
		char*  hex	= Allocate(hlen);
		crypto_hex_stringify(raw, rawlen, hex);
		outv = NewStrValue(interpreter, hex);
		free(hex);
	} else if (mode == 1) {
		char* tmp = Allocate(rawlen + 1);
		memcpy(tmp, raw, rawlen);
		tmp[rawlen] = '\0';
		Rune* runes = Utf8Core_Utf8ToRunes(tmp);
		free(tmp);
		if (runes == NULL) {
			free(raw);
			return NewErrorFValue(interpreter,
								  "%s: invalid UTF-8 in WordArray",
								  ARGUMENT_ERROR);
		}
		String u8 = RunesStrToString(runes);
		free(runes);
		if (u8 == NULL) {
			free(raw);
			return NewErrorFValue(interpreter,
								  "%s: UTF-8 encode failed",
								  RUNTIME_ERROR);
		}
		outv = NewStrValue(interpreter, u8);
		free(u8);
	} else if (mode == 2) {
		size_t cap = 4 * ((rawlen + 2u) / 3u) + 4;
		char*  b64 = Allocate(cap);
		crypto_base64_encode(raw, rawlen, b64);
		outv = NewStrValue(interpreter, b64);
		free(b64);
	} else if (mode == 3) {
		size_t cap = 4 * ((rawlen + 2u) / 3u) + 4;
		char*  b64u = Allocate(cap);
		crypto_base64url_encode(raw, rawlen, b64u);
		outv = NewStrValue(interpreter, b64u);
		free(b64u);
	} else {
		size_t hlen = rawlen * 2 + 1;
		char*  hex	= Allocate(hlen);
		crypto_hex_stringify(raw, rawlen, hex);
		outv = NewStrValue(interpreter, hex);
		free(hex);
	}

	free(raw);
	return outv;
}

static Value*
_CryptoMd5(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: MD5 expects 1 argument",
							  ARGUMENT_ERROR);
	}
	uint8_t* msg = NULL;
	size_t	 len = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &msg, &len, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	uint8_t digest[CRYPTO_MD5_DIGEST_LEN];
	crypto_md5(msg, len, digest);
	free(msg);
	return _make_word_array(interpreter, digest, CRYPTO_MD5_DIGEST_LEN);
}

static Value*
_CryptoSha1(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: SHA1 expects 1 argument",
							  ARGUMENT_ERROR);
	}
	uint8_t* msg = NULL;
	size_t	 len = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &msg, &len, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	uint8_t digest[CRYPTO_SHA1_DIGEST_LEN];
	crypto_sha1(msg, len, digest);
	free(msg);
	return _make_word_array(interpreter, digest, CRYPTO_SHA1_DIGEST_LEN);
}

static Value*
_CryptoSha256(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: SHA256 expects 1 argument",
							  ARGUMENT_ERROR);
	}
	uint8_t* msg = NULL;
	size_t	 len = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &msg, &len, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	uint8_t digest[CRYPTO_SHA256_DIGEST_LEN];
	crypto_sha256(msg, len, digest);
	free(msg);
	return _make_word_array(interpreter, digest, CRYPTO_SHA256_DIGEST_LEN);
}

static Value*
_CryptoHmacMd5(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: HmacMD5 expects 2 arguments",
							  ARGUMENT_ERROR);
	}
	uint8_t *msg = NULL, *key = NULL;
	size_t	 mlen = 0, klen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &msg, &mlen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &key, &klen, &err) != 0) {
		free(msg);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	uint8_t digest[CRYPTO_MD5_DIGEST_LEN];
	crypto_hmac_md5(key, klen, msg, mlen, digest);
	free(msg);
	free(key);
	return _make_word_array(interpreter, digest, CRYPTO_MD5_DIGEST_LEN);
}

static Value*
_CryptoHmacSha1(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: HmacSHA1 expects 2 arguments",
							  ARGUMENT_ERROR);
	}
	uint8_t *msg = NULL, *key = NULL;
	size_t	 mlen = 0, klen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &msg, &mlen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &key, &klen, &err) != 0) {
		free(msg);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	uint8_t digest[CRYPTO_SHA1_DIGEST_LEN];
	crypto_hmac_sha1(key, klen, msg, mlen, digest);
	free(msg);
	free(key);
	return _make_word_array(interpreter, digest, CRYPTO_SHA1_DIGEST_LEN);
}

static Value*
_CryptoHmacSha256(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: HmacSHA256 expects 2 arguments",
							  ARGUMENT_ERROR);
	}
	uint8_t *msg = NULL, *key = NULL;
	size_t	 mlen = 0, klen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &msg, &mlen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &key, &klen, &err) != 0) {
		free(msg);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	uint8_t digest[CRYPTO_SHA256_DIGEST_LEN];
	crypto_hmac_sha256(key, klen, msg, mlen, digest);
	free(msg);
	free(key);
	return _make_word_array(interpreter, digest, CRYPTO_SHA256_DIGEST_LEN);
}

#define _CRYPTO_ONE_SHOT(fname, label, digest_fn, digest_len)                                  \
	static Value* fname(Interpreter* interpreter, int argc, Value** arguments) {               \
		if (argc != 1) {                                                                       \
			return NewErrorFValue(interpreter,                                                 \
								  "%s: " label " expects 1 argument",                          \
								  ARGUMENT_ERROR);                                             \
		}                                                                                      \
		uint8_t* msg = NULL;                                                                   \
		size_t	  len = 0;                                                                     \
		String	  err = NULL;                                                                  \
		if (_bytes_from_value(interpreter, arguments[0], &msg, &len, &err) != 0) {              \
			Value* e = NewErrorValue(interpreter, err);                                        \
			free(err);                                                                         \
			return e;                                                                          \
		}                                                                                      \
		uint8_t digest[digest_len];                                                            \
		digest_fn(msg, len, digest);                                                           \
		free(msg);                                                                             \
		return _make_word_array(interpreter, digest, digest_len);                              \
	}

_CRYPTO_ONE_SHOT(_CryptoSha224, "SHA224", crypto_sha224, CRYPTO_SHA224_DIGEST_LEN)
_CRYPTO_ONE_SHOT(_CryptoSha384, "SHA384", crypto_sha384, CRYPTO_SHA384_DIGEST_LEN)
_CRYPTO_ONE_SHOT(_CryptoSha512, "SHA512", crypto_sha512, CRYPTO_SHA512_DIGEST_LEN)
_CRYPTO_ONE_SHOT(_CryptoSha3_224, "SHA3_224", crypto_sha3_224, CRYPTO_SHA3_224_DIGEST_LEN)
_CRYPTO_ONE_SHOT(_CryptoSha3_256, "SHA3_256", crypto_sha3_256, CRYPTO_SHA3_256_DIGEST_LEN)
_CRYPTO_ONE_SHOT(_CryptoSha3_384, "SHA3_384", crypto_sha3_384, CRYPTO_SHA3_384_DIGEST_LEN)
_CRYPTO_ONE_SHOT(_CryptoSha3_512, "SHA3_512", crypto_sha3_512, CRYPTO_SHA3_512_DIGEST_LEN)
_CRYPTO_ONE_SHOT(_CryptoRipemd160, "RIPEMD160", crypto_ripemd160, CRYPTO_RIPEMD160_DIGEST_LEN)

#undef _CRYPTO_ONE_SHOT

static Value*
_CryptoHmacSha384(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: HmacSHA384 expects 2 arguments",
							  ARGUMENT_ERROR);
	}
	uint8_t *msg = NULL, *key = NULL;
	size_t	 mlen = 0, klen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &msg, &mlen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &key, &klen, &err) != 0) {
		free(msg);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	uint8_t digest[CRYPTO_SHA384_DIGEST_LEN];
	crypto_hmac_sha384(key, klen, msg, mlen, digest);
	free(msg);
	free(key);
	return _make_word_array(interpreter, digest, CRYPTO_SHA384_DIGEST_LEN);
}

static Value*
_CryptoHmacSha512(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: HmacSHA512 expects 2 arguments",
							  ARGUMENT_ERROR);
	}
	uint8_t *msg = NULL, *key = NULL;
	size_t	 mlen = 0, klen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &msg, &mlen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &key, &klen, &err) != 0) {
		free(msg);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	uint8_t digest[CRYPTO_SHA512_DIGEST_LEN];
	crypto_hmac_sha512(key, klen, msg, mlen, digest);
	free(msg);
	free(key);
	return _make_word_array(interpreter, digest, CRYPTO_SHA512_DIGEST_LEN);
}

static Value*
_CryptoHmacRipemd160(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: HmacRIPEMD160 expects 2 arguments",
							  ARGUMENT_ERROR);
	}
	uint8_t *msg = NULL, *key = NULL;
	size_t	 mlen = 0, klen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &msg, &mlen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &key, &klen, &err) != 0) {
		free(msg);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	uint8_t digest[CRYPTO_RIPEMD160_DIGEST_LEN];
	crypto_hmac_ripemd160(key, klen, msg, mlen, digest);
	free(msg);
	free(key);
	return _make_word_array(interpreter, digest, CRYPTO_RIPEMD160_DIGEST_LEN);
}

static Value*
_CryptoPBKDF2(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 5) {
		return NewErrorFValue(
			interpreter,
			"%s: PBKDF2 expects 5 arguments (password, salt, iterations, dkLen, prf)",
			ARGUMENT_ERROR);
	}
	uint8_t *pass = NULL, *salt = NULL;
	size_t	 plen = 0, slen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &pass, &plen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &salt, &slen, &err) != 0) {
		free(pass);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (!ValueIsInt(arguments[2]) && !ValueIsNum(arguments[2])) {
		free(pass);
		free(salt);
		return NewErrorFValue(interpreter,
							  "%s: PBKDF2 iterations must be a number",
							  TYPE_ERROR);
	}
	int iti = CoerceToI32(arguments[2]);
	if (iti < 1) {
		free(pass);
		free(salt);
		return NewErrorFValue(interpreter,
							  "%s: PBKDF2 iterations must be positive",
							  ARGUMENT_ERROR);
	}
	if (!ValueIsInt(arguments[3]) && !ValueIsNum(arguments[3])) {
		free(pass);
		free(salt);
		return NewErrorFValue(interpreter,
							  "%s: PBKDF2 dkLen must be a number",
							  TYPE_ERROR);
	}
	int dkli = CoerceToI32(arguments[3]);
	if (dkli < 1 || dkli > 65536) {
		free(pass);
		free(salt);
		return NewErrorFValue(interpreter,
							  "%s: PBKDF2 dkLen out of range",
							  ARGUMENT_ERROR);
	}
	if (!ValueIsInt(arguments[4]) && !ValueIsNum(arguments[4])) {
		free(pass);
		free(salt);
		return NewErrorFValue(interpreter,
							  "%s: PBKDF2 prf must be 0 (SHA256), 1 (SHA1), or 2 (MD5)",
							  TYPE_ERROR);
	}
	int prf = CoerceToI32(arguments[4]);
	if (prf < 0 || prf > 2) {
		free(pass);
		free(salt);
		return NewErrorFValue(interpreter,
							  "%s: PBKDF2 prf must be 0, 1, or 2",
							  ARGUMENT_ERROR);
	}
	uint8_t* dk = Allocate((size_t) dkli);
	if (crypto_pbkdf2(pass,
					  plen,
					  salt,
					  slen,
					  (uint32_t) iti,
					  prf,
					  dk,
					  (size_t) dkli)
		!= 0) {
		free(pass);
		free(salt);
		free(dk);
		return NewErrorFValue(interpreter, "%s: PBKDF2 failed", RUNTIME_ERROR);
	}
	free(pass);
	free(salt);
	Value* wa = _make_word_array(interpreter, dk, (size_t) dkli);
	free(dk);
	return wa;
}

static Value*
_CryptoEvpKDF(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 5) {
		return NewErrorFValue(
			interpreter,
			"%s: EvpKDF expects 5 arguments (password, salt, iterations, keyLen, ivLen)",
			ARGUMENT_ERROR);
	}
	uint8_t *pass = NULL, *salt = NULL;
	size_t	 plen = 0, slen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &pass, &plen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &salt, &slen, &err) != 0) {
		free(pass);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (!ValueIsInt(arguments[2]) && !ValueIsNum(arguments[2])) {
		free(pass);
		free(salt);
		return NewErrorFValue(interpreter,
							  "%s: EvpKDF iterations must be a number",
							  TYPE_ERROR);
	}
	int iti = CoerceToI32(arguments[2]);
	if (iti < 1) {
		free(pass);
		free(salt);
		return NewErrorFValue(interpreter,
							  "%s: EvpKDF iterations must be positive",
							  ARGUMENT_ERROR);
	}
	int kli = CoerceToI32(arguments[3]);
	int ivli = CoerceToI32(arguments[4]);
	if (kli < 0 || ivli < 0 || kli > 4096 || ivli > 4096) {
		free(pass);
		free(salt);
		return NewErrorFValue(interpreter,
							  "%s: EvpKDF keyLen/ivLen out of range",
							  ARGUMENT_ERROR);
	}
	size_t	 tot = (size_t) kli + (size_t) ivli;
	uint8_t* buf = Allocate(tot ? tot : 1);
	uint8_t* keyp = buf;
	uint8_t* ivp  = buf + (size_t) kli;
	if (crypto_evpkdf_md5(pass,
						  plen,
						  salt,
						  slen,
						  (uint32_t) iti,
						  (size_t) kli,
						  (size_t) ivli,
						  keyp,
						  ivp)
		!= 0) {
		free(pass);
		free(salt);
		free(buf);
		return NewErrorFValue(interpreter, "%s: EvpKDF failed", RUNTIME_ERROR);
	}
	free(pass);
	free(salt);
	Value* wa = _make_word_array(interpreter, buf, tot);
	free(buf);
	return wa;
}

static Value*
_CryptoRC4(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 3) {
		return NewErrorFValue(interpreter,
							  "%s: RC4 expects 3 arguments (key, data, dropN)",
							  ARGUMENT_ERROR);
	}
	uint8_t *key = NULL, *data = NULL;
	size_t	 klen = 0, dlen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &key, &klen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &data, &dlen, &err) != 0) {
		free(key);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (!ValueIsInt(arguments[2]) && !ValueIsNum(arguments[2])) {
		free(key);
		free(data);
		return NewErrorFValue(interpreter,
							  "%s: RC4 dropN must be a number",
							  TYPE_ERROR);
	}
	int		 dropi = CoerceToI32(arguments[2]);
	unsigned drop = dropi < 0 ? 0u : (unsigned) dropi;
	uint8_t* out = Allocate(dlen ? dlen : 1);
	crypto_rc4(key, klen, drop, data, dlen, out);
	free(key);
	free(data);
	Value* wa = _make_word_array(interpreter, out, dlen);
	free(out);
	return wa;
}

static Value*
_CryptoAesEcbEncrypt(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: AesEcbEncrypt expects 2 arguments (key, plaintext)",
							  ARGUMENT_ERROR);
	}
	uint8_t *key = NULL, *pt = NULL;
	size_t	 klen = 0, plen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &key, &klen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &pt, &plen, &err) != 0) {
		free(key);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (klen != 16) {
		free(key);
		free(pt);
		return NewErrorFValue(interpreter,
							  "%s: AES-128 requires a 16-byte key",
							  ARGUMENT_ERROR);
	}
	uint8_t* ct	 = NULL;
	size_t	 ctlen = 0;
	if (crypto_aes_ecb_encrypt(key, klen, pt, plen, &ct, &ctlen) != 0) {
		free(key);
		free(pt);
		return NewErrorFValue(interpreter, "%s: AES-ECB encrypt failed", RUNTIME_ERROR);
	}
	free(key);
	free(pt);
	Value* wa = _make_word_array(interpreter, ct, ctlen);
	free(ct);
	return wa;
}

static Value*
_CryptoAesEcbDecrypt(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 2) {
		return NewErrorFValue(interpreter,
							  "%s: AesEcbDecrypt expects 2 arguments (key, ciphertext)",
							  ARGUMENT_ERROR);
	}
	uint8_t *key = NULL, *ct = NULL;
	size_t	 klen = 0, clen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &key, &klen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &ct, &clen, &err) != 0) {
		free(key);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (klen != 16) {
		free(key);
		free(ct);
		return NewErrorFValue(interpreter,
							  "%s: AES-128 requires a 16-byte key",
							  ARGUMENT_ERROR);
	}
	uint8_t* pt	 = NULL;
	size_t	 ptlen = 0;
	if (crypto_aes_ecb_decrypt(key, klen, ct, clen, &pt, &ptlen) != 0) {
		free(key);
		free(ct);
		return NewErrorFValue(interpreter,
							  "%s: AES-ECB decrypt failed (bad padding?)",
							  ARGUMENT_ERROR);
	}
	free(key);
	free(ct);
	Value* wa = _make_word_array(interpreter, pt, ptlen);
	free(pt);
	return wa;
}

static Value*
_CryptoAesCbcEncrypt(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 3) {
		return NewErrorFValue(
			interpreter,
			"%s: AesCbcEncrypt expects 3 arguments (key, iv, plaintext)",
			ARGUMENT_ERROR);
	}
	uint8_t *key = NULL, *iv = NULL, *pt = NULL;
	size_t	 klen = 0, ivlen = 0, plen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &key, &klen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &iv, &ivlen, &err) != 0) {
		free(key);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[2], &pt, &plen, &err) != 0) {
		free(key);
		free(iv);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (klen != 16 || ivlen != 16) {
		free(key);
		free(iv);
		free(pt);
		return NewErrorFValue(interpreter,
							  "%s: AES-128 CBC requires 16-byte key and 16-byte IV",
							  ARGUMENT_ERROR);
	}
	uint8_t* ct	 = NULL;
	size_t	 ctlen = 0;
	if (crypto_aes_cbc_encrypt(key, klen, iv, pt, plen, &ct, &ctlen) != 0) {
		free(key);
		free(iv);
		free(pt);
		return NewErrorFValue(interpreter, "%s: AES-CBC encrypt failed", RUNTIME_ERROR);
	}
	free(key);
	free(iv);
	free(pt);
	Value* wa = _make_word_array(interpreter, ct, ctlen);
	free(ct);
	return wa;
}

static Value*
_CryptoAesCbcDecrypt(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 3) {
		return NewErrorFValue(
			interpreter,
			"%s: AesCbcDecrypt expects 3 arguments (key, iv, ciphertext)",
			ARGUMENT_ERROR);
	}
	uint8_t *key = NULL, *iv = NULL, *ct = NULL;
	size_t	 klen = 0, ivlen = 0, clen = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &key, &klen, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[1], &iv, &ivlen, &err) != 0) {
		free(key);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (_bytes_from_value(interpreter, arguments[2], &ct, &clen, &err) != 0) {
		free(key);
		free(iv);
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	if (klen != 16 || ivlen != 16) {
		free(key);
		free(iv);
		free(ct);
		return NewErrorFValue(interpreter,
							  "%s: AES-128 CBC requires 16-byte key and 16-byte IV",
							  ARGUMENT_ERROR);
	}
	uint8_t* pt	 = NULL;
	size_t	 ptlen = 0;
	if (crypto_aes_cbc_decrypt(key, klen, iv, ct, clen, &pt, &ptlen) != 0) {
		free(key);
		free(iv);
		free(ct);
		return NewErrorFValue(interpreter,
							  "%s: AES-CBC decrypt failed (bad padding?)",
							  ARGUMENT_ERROR);
	}
	free(key);
	free(iv);
	free(ct);
	Value* wa = _make_word_array(interpreter, pt, ptlen);
	free(pt);
	return wa;
}

static Value*
_EncHexStringify(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: enc.Hex.stringify expects 1 argument",
							  ARGUMENT_ERROR);
	}
	uint8_t* raw = NULL;
	size_t	 len = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &raw, &len, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	char* hex = Allocate(len * 2 + 1);
	crypto_hex_stringify(raw, len, hex);
	free(raw);
	Value* s = NewStrValue(interpreter, hex);
	free(hex);
	return s;
}

static Value*
_EncHexParse(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: enc.Hex.parse expects 1 string",
							  ARGUMENT_ERROR);
	}
	String	 h	  = ValueToString(arguments[0]);
	size_t	 blen = 0;
	uint8_t* buf  = crypto_hex_parse(h, &blen);
	free(h);
	if (buf == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: invalid hex string",
							  ARGUMENT_ERROR);
	}
	Value* wa = _make_word_array(interpreter, buf, blen);
	free(buf);
	return wa;
}

static Value*
_EncUtf8Stringify(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: enc.Utf8.stringify expects 1 argument",
							  ARGUMENT_ERROR);
	}
	uint8_t* raw = NULL;
	size_t	 len = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &raw, &len, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	char* tmp = Allocate(len + 1);
	memcpy(tmp, raw, len);
	tmp[len] = '\0';
	free(raw);
	Rune* runes = Utf8Core_Utf8ToRunes(tmp);
	free(tmp);
	if (runes == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: invalid UTF-8 in WordArray",
							  ARGUMENT_ERROR);
	}
	String u8 = RunesStrToString(runes);
	free(runes);
	if (u8 == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: UTF-8 encode failed",
							  RUNTIME_ERROR);
	}
	Value* s = NewStrValue(interpreter, u8);
	free(u8);
	return s;
}

static Value*
_EncUtf8Parse(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: enc.Utf8.parse expects 1 string",
							  ARGUMENT_ERROR);
	}
	String u8 = ValueToString(arguments[0]);
	if (u8 == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: enc.Utf8.parse out of memory",
							  MEMORY_ERROR);
	}
	size_t len = strlen(u8);
	Value* wa  = _make_word_array(interpreter, (const uint8_t*) u8, len);
	free(u8);
	return wa;
}

static Value*
_EncB64Stringify(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: enc.Base64.stringify expects 1 argument",
							  ARGUMENT_ERROR);
	}
	uint8_t* raw = NULL;
	size_t	 len = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &raw, &len, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	size_t cap = 4 * ((len + 2u) / 3u) + 4;
	char*  b64 = Allocate(cap);
	crypto_base64_encode(raw, len, b64);
	free(raw);
	Value* s = NewStrValue(interpreter, b64);
	free(b64);
	return s;
}

static Value*
_EncB64Parse(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: enc.Base64.parse expects 1 string",
							  ARGUMENT_ERROR);
	}
	String	 s	  = ValueToString(arguments[0]);
	size_t	 blen = 0;
	uint8_t* buf  = crypto_base64_decode(s, &blen);
	free(s);
	if (buf == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: invalid base64",
							  ARGUMENT_ERROR);
	}
	Value* wa = _make_word_array(interpreter, buf, blen);
	free(buf);
	return wa;
}

static Value*
_EncB64UrlStringify(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1) {
		return NewErrorFValue(interpreter,
							  "%s: enc.Base64Url.stringify expects 1 argument",
							  ARGUMENT_ERROR);
	}
	uint8_t* raw = NULL;
	size_t	 len = 0;
	String	 err = NULL;
	if (_bytes_from_value(interpreter, arguments[0], &raw, &len, &err) != 0) {
		Value* e = NewErrorValue(interpreter, err);
		free(err);
		return e;
	}
	size_t cap = 4 * ((len + 2u) / 3u) + 4;
	char*  b64u = Allocate(cap);
	crypto_base64url_encode(raw, len, b64u);
	free(raw);
	Value* s = NewStrValue(interpreter, b64u);
	free(b64u);
	return s;
}

static Value*
_EncB64UrlParse(Interpreter* interpreter, int argc, Value** arguments) {
	if (argc != 1 || !ValueIsStr(arguments[0])) {
		return NewErrorFValue(interpreter,
							  "%s: enc.Base64Url.parse expects 1 string",
							  ARGUMENT_ERROR);
	}
	String	 s	  = ValueToString(arguments[0]);
	size_t	 blen = 0;
	uint8_t* buf  = crypto_base64url_decode(s, &blen);
	free(s);
	if (buf == NULL) {
		return NewErrorFValue(interpreter,
							  "%s: invalid base64url",
							  ARGUMENT_ERROR);
	}
	Value* wa = _make_word_array(interpreter, buf, blen);
	free(buf);
	return wa;
}

static Value* _build_encoders(Interpreter* interp) {
	Value*	 hex  = NewObjectValue(interp);
	HashMap* hexm = CoerceToHashMap(hex);
	HashMapSet(hexm, LX_ENC_TAG, NewIntValue(interp, 0));
	HashMapSet(hexm,
			   "stringify",
			   NewNativeFunctionValue(
				   interp,
				   CreateNativeFunctionMeta("stringify", 1, _EncHexStringify)));
	HashMapSet(hexm,
			   "parse",
			   NewNativeFunctionValue(
				   interp,
				   CreateNativeFunctionMeta("parse", 1, _EncHexParse)));

	Value*	 utf8  = NewObjectValue(interp);
	HashMap* utf8m = CoerceToHashMap(utf8);
	HashMapSet(utf8m, LX_ENC_TAG, NewIntValue(interp, 1));
	HashMapSet(
		utf8m,
		"stringify",
		NewNativeFunctionValue(
			interp,
			CreateNativeFunctionMeta("stringify", 1, _EncUtf8Stringify)));
	HashMapSet(utf8m,
			   "parse",
			   NewNativeFunctionValue(
				   interp,
				   CreateNativeFunctionMeta("parse", 1, _EncUtf8Parse)));

	Value*	 b64  = NewObjectValue(interp);
	HashMap* b64m = CoerceToHashMap(b64);
	HashMapSet(b64m, LX_ENC_TAG, NewIntValue(interp, 2));
	HashMapSet(b64m,
			   "stringify",
			   NewNativeFunctionValue(
				   interp,
				   CreateNativeFunctionMeta("stringify", 1, _EncB64Stringify)));
	HashMapSet(b64m,
			   "parse",
			   NewNativeFunctionValue(
				   interp,
				   CreateNativeFunctionMeta("parse", 1, _EncB64Parse)));

	Value*	 b64u  = NewObjectValue(interp);
	HashMap* b64um = CoerceToHashMap(b64u);
	HashMapSet(b64um, LX_ENC_TAG, NewIntValue(interp, 3));
	HashMapSet(b64um,
			   "stringify",
			   NewNativeFunctionValue(
				   interp,
				   CreateNativeFunctionMeta("stringify", 1, _EncB64UrlStringify)));
	HashMapSet(b64um,
			   "parse",
			   NewNativeFunctionValue(
				   interp,
				   CreateNativeFunctionMeta("parse", 1, _EncB64UrlParse)));

	Value*	 enc  = NewObjectValue(interp);
	HashMap* encm = CoerceToHashMap(enc);
	HashMapSet(encm, "Hex", hex);
	HashMapSet(encm, "Utf8", utf8);
	HashMapSet(encm, "Base64", b64);
	HashMapSet(encm, "Base64Url", b64u);
	return enc;
}

static ModuleFunction _CryptoModuleFunctions[] = {
	{ .Name = "MD5", .Argc = 1, .CFunction = _CryptoMd5, .Value = NULL },
	{ .Name = "SHA1", .Argc = 1, .CFunction = _CryptoSha1, .Value = NULL },
	{ .Name = "SHA256", .Argc = 1, .CFunction = _CryptoSha256, .Value = NULL },
	{ .Name		 = "HmacMD5",
	  .Argc		 = 2,
	  .CFunction = _CryptoHmacMd5,
	  .Value	 = NULL },
	{ .Name		 = "HmacSHA1",
	  .Argc		 = 2,
	  .CFunction = _CryptoHmacSha1,
	  .Value	 = NULL },
	{ .Name		 = "HmacSHA256",
	  .Argc		 = 2,
	  .CFunction = _CryptoHmacSha256,
	  .Value	 = NULL },
	{ .Name = "SHA224", .Argc = 1, .CFunction = _CryptoSha224, .Value = NULL },
	{ .Name = "SHA384", .Argc = 1, .CFunction = _CryptoSha384, .Value = NULL },
	{ .Name = "SHA512", .Argc = 1, .CFunction = _CryptoSha512, .Value = NULL },
	{ .Name = "SHA3_224", .Argc = 1, .CFunction = _CryptoSha3_224, .Value = NULL },
	{ .Name = "SHA3_256", .Argc = 1, .CFunction = _CryptoSha3_256, .Value = NULL },
	{ .Name = "SHA3_384", .Argc = 1, .CFunction = _CryptoSha3_384, .Value = NULL },
	{ .Name = "SHA3_512", .Argc = 1, .CFunction = _CryptoSha3_512, .Value = NULL },
	{ .Name = "RIPEMD160", .Argc = 1, .CFunction = _CryptoRipemd160, .Value = NULL },
	{ .Name		 = "HmacSHA384",
	  .Argc		 = 2,
	  .CFunction = _CryptoHmacSha384,
	  .Value	 = NULL },
	{ .Name		 = "HmacSHA512",
	  .Argc		 = 2,
	  .CFunction = _CryptoHmacSha512,
	  .Value	 = NULL },
	{ .Name		 = "HmacRIPEMD160",
	  .Argc		 = 2,
	  .CFunction = _CryptoHmacRipemd160,
	  .Value	 = NULL },
	{ .Name = "PBKDF2", .Argc = 5, .CFunction = _CryptoPBKDF2, .Value = NULL },
	{ .Name = "EvpKDF", .Argc = 5, .CFunction = _CryptoEvpKDF, .Value = NULL },
	{ .Name = "RC4", .Argc = 3, .CFunction = _CryptoRC4, .Value = NULL },
	{ .Name		 = "AesEcbEncrypt",
	  .Argc		 = 2,
	  .CFunction = _CryptoAesEcbEncrypt,
	  .Value	 = NULL },
	{ .Name		 = "AesEcbDecrypt",
	  .Argc		 = 2,
	  .CFunction = _CryptoAesEcbDecrypt,
	  .Value	 = NULL },
	{ .Name		 = "AesCbcEncrypt",
	  .Argc		 = 3,
	  .CFunction = _CryptoAesCbcEncrypt,
	  .Value	 = NULL },
	{ .Name		 = "AesCbcDecrypt",
	  .Argc		 = 3,
	  .CFunction = _CryptoAesCbcDecrypt,
	  .Value	 = NULL },
	{ .Name = NULL }
};

Value* LoadCoreCrypto(Interpreter* interpreter) {
	_ensure_word_array_class(interpreter);

	Value*	 module = NewObjectValue(interpreter);
	HashMap* map	= CoerceToHashMap(module);

	for (int i = 0; _CryptoModuleFunctions[i].Name != NULL; i++) {
		ModuleFunction func = _CryptoModuleFunctions[i];
		HashMapSet(map,
				   func.Name,
				   NewNativeFunctionValue(
					   interpreter,
					   CreateNativeFunctionMeta((const String) func.Name,
												func.Argc,
												func.CFunction)));
	}

	HashMapSet(map, "enc", _build_encoders(interpreter));
	/* Aggregate for `import { Crypto } from "core:crypto"` (see file header). */
	HashMapSet(map, "Crypto", module);

	return module;
}
