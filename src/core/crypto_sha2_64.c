
#include "./crypto.h"
#include "./crypto_algo.h"

static inline uint64_t u64(uint64_t x) {
	return x;
}

static inline uint64_t ror64(uint64_t x, int n) {
	return (x >> (unsigned) n) | (x << (unsigned) (64 - n));
}

static inline uint64_t shr64(uint64_t x, int n) {
	return x >> (unsigned) n;
}

static inline uint64_t Ch64(uint64_t x, uint64_t y, uint64_t z) {
	return (x & y) ^ (~x & z);
}

static inline uint64_t Maj64(uint64_t x, uint64_t y, uint64_t z) {
	return (x & y) ^ (x & z) ^ (y & z);
}

static inline uint64_t S0_64(uint64_t x) {
	return ror64(x, 28) ^ ror64(x, 34) ^ ror64(x, 39);
}

static inline uint64_t S1_64(uint64_t x) {
	return ror64(x, 14) ^ ror64(x, 18) ^ ror64(x, 41);
}

static inline uint64_t s0_64(uint64_t x) {
	return ror64(x, 1) ^ ror64(x, 8) ^ shr64(x, 7);
}

static inline uint64_t s1_64(uint64_t x) {
	return ror64(x, 19) ^ ror64(x, 61) ^ shr64(x, 6);
}

static void sha512_compress(uint64_t* h8, const uint8_t* block) {
	static const uint64_t K[80] = {
		0x428a2f98d728ae22ull, 0x7137449123ef65cdull, 0xb5c0fbcfec4d3b2full,
		0xe9b5dba58189dbbcull, 0x3956c25bf348b538ull, 0x59f111f1b605d019ull,
		0x923f82a4af194f9bull, 0xab1c5ed5da6d8118ull, 0xd807aa98a3030242ull,
		0x12835b0145706fbeull, 0x243185be4ee4b28cull, 0x550c7dc3d5ffb4e2ull,
		0x72be5d74f27b896full, 0x80deb1fe3b1696b1ull, 0x9bdc06a725c71235ull,
		0xc19bf174cf692694ull, 0xe49b69c19ef14ad2ull, 0xefbe4786384f25e3ull,
		0x0fc19dc68b8cd5b5ull, 0x240ca1cc77ac9c65ull, 0x2de92c6f592b0275ull,
		0x4a7484aa6ea6e483ull, 0x5cb0a9dcbd41fbd4ull, 0x76f988da831153b5ull,
		0x983e5152ee66dfabull, 0xa831c66d2db43210ull, 0xb00327c898fb213full,
		0xbf597fc7beef0ee4ull, 0xc6e00bf33da88fc2ull, 0xd5a79147930aa725ull,
		0x06ca6351e003826full, 0x142929670a0e6e70ull, 0x27b70a8546d22ffcull,
		0x2e1b21385c26c926ull, 0x4d2c6dfc5ac42aedull, 0x53380d139d95b3dfull,
		0x650a73548baf63deull, 0x766a0abb3c77b2a8ull, 0x81c2c92e47edaee6ull,
		0x92722c851482353bull, 0xa2bfe8a14cf10364ull, 0xa81a664bbc423001ull,
		0xc24b8b70d0f89791ull, 0xc76c51a30654be30ull, 0xd192e819d6ef5218ull,
		0xd69906245565a910ull, 0xf40e35855771202aull, 0x106aa07032bbd1b8ull,
		0x19a4c116b8d2d0c8ull, 0x1e376c085141ab53ull, 0x2748774cdf8eeb99ull,
		0x34b0bcb5e19b48a8ull, 0x391c0cb3c5c95a63ull, 0x4ed8aa4ae3418acbull,
		0x5b9cca4f7763e373ull, 0x682e6ff3d6b2b8a3ull, 0x748f82ee5defb2fcull,
		0x78a5636f43172f60ull, 0x84c87814a1f0ab72ull, 0x8cc702081a6439ecull,
		0x90befffa23631e28ull, 0xa4506cebde82bde9ull, 0xbef9a3f7b2c67915ull,
		0xc67178f2e372532bull, 0xca273eceea26619cull, 0xd186b8c721c0c207ull,
		0xeada7dd6cde0eb1eull, 0xf57d4f7fee6ed178ull, 0x06f067aa72176fbaull,
		0x0a637dc5a2c898a6ull, 0x113f9804bef90daeull, 0x1b710b35131c471bull,
		0x28db77f523047d84ull, 0x32caab7b40c72493ull, 0x3c9ebe0a15c9bebcull,
		0x431d67c49c100d4cull, 0x4cc5d4becb3e42b6ull, 0x597f299cfc657e2aull,
		0x5fcb6fab3ad6faecull, 0x6c44198c4a475817ull
	};

	uint64_t w[80];
	int		 i;
	for (i = 0; i < 16; i++) {
		const uint8_t* p = block + (size_t) i * 8;
		w[i]			 = ((uint64_t) p[0] << 56) | ((uint64_t) p[1] << 48)
						   | ((uint64_t) p[2] << 40) | ((uint64_t) p[3] << 32)
						   | ((uint64_t) p[4] << 24) | ((uint64_t) p[5] << 16)
						   | ((uint64_t) p[6] << 8) | (uint64_t) p[7];
	}
	for (; i < 80; i++) {
		w[i] = s1_64(w[i - 2]) + w[i - 7] + s0_64(w[i - 15]) + w[i - 16];
	}

	uint64_t a = h8[0], b = h8[1], c = h8[2], d = h8[3];
	uint64_t e = h8[4], f = h8[5], g = h8[6], hh = h8[7];
	for (i = 0; i < 80; i++) {
		uint64_t t1 = hh + S1_64(e) + Ch64(e, f, g) + K[i] + w[i];
		uint64_t t2 = S0_64(a) + Maj64(a, b, c);
		hh			= g;
		g			= f;
		f			= e;
		e			= d + t1;
		d			= c;
		c			= b;
		b			= a;
		a			= t1 + t2;
	}
	h8[0] += a;
	h8[1] += b;
	h8[2] += c;
	h8[3] += d;
	h8[4] += e;
	h8[5] += f;
	h8[6] += g;
	h8[7] += hh;
}

static void sha512_hash(const uint8_t* data,
						size_t		   len,
						const uint64_t iv[8],
						uint8_t*	   out,
						size_t		   out_bytes) {
	uint64_t h[8];
	memcpy(h, iv, sizeof(h));

	size_t	 k		 = (112u - (len + 1u) % 128u) % 128u;
	size_t	 new_len = len + 1u + k + 16u;
	uint8_t* msg	 = Allocate(new_len);
	memcpy(msg, data, len);
	msg[len] = 0x80;
	memset(msg + len + 1, 0, new_len - len - 1);

	uint64_t bit_lo = (uint64_t) len * 8u;
	uint64_t bit_hi = 0;
	for (int i = 0; i < 8; i++) {
		msg[new_len - 16 + (size_t) i] = (uint8_t) (bit_hi >> (56 - 8 * i));
	}
	for (int i = 0; i < 8; i++) {
		msg[new_len - 8 + (size_t) i] = (uint8_t) (bit_lo >> (56 - 8 * i));
	}

	for (size_t off = 0; off < new_len; off += 128) {
		sha512_compress(h, msg + off);
	}
	free(msg);

	for (size_t i = 0; i < out_bytes; i++) {
		size_t	 wi = i / 8;
		unsigned sh = (unsigned) (56 - (i % 8) * 8);
		out[i]		= (uint8_t) (h[wi] >> sh);
	}
}

void crypto_sha512(const uint8_t* data,
				   size_t		  len,
				   uint8_t		  out[CRYPTO_SHA512_DIGEST_LEN]) {
	static const uint64_t iv[8] = {
		0x6a09e667f3bcc908ull, 0xbb67ae8584caa73bull, 0x3c6ef372fe94f82bull,
		0xa54ff53a5f1d36f1ull, 0x510e527fade682d1ull, 0x9b05688c2b3e6c1full,
		0x1f83d9abfb41bd6bull, 0x5be0cd19137e2179ull
	};
	sha512_hash(data, len, iv, out, CRYPTO_SHA512_DIGEST_LEN);
}

void crypto_sha384(const uint8_t* data,
				   size_t		  len,
				   uint8_t		  out[CRYPTO_SHA384_DIGEST_LEN]) {
	static const uint64_t iv[8] = {
		0xcbbb9d5dc1059ed8ull, 0x629a292a367cd507ull, 0x9159015a3070dd17ull,
		0x152fecd8f70e5939ull, 0x67332667ffc00b31ull, 0x8eb44a8768581511ull,
		0xdb0c2e0d64f98fa7ull, 0x47b5481dbefa4fa4ull
	};
	sha512_hash(data, len, iv, out, CRYPTO_SHA384_DIGEST_LEN);
}

/* --- SHA-224 (32-bit SHA-256 path, different IV, 28-byte output) --- */

static inline uint32_t u32x(uint64_t x) {
	return (uint32_t) (x & 0xffffffffu);
}

static inline uint32_t ror32x(uint32_t x, int n) {
	return u32x(((uint64_t) x << (32 - n)) | ((uint64_t) x >> n));
}

static inline uint32_t shr32x(uint32_t x, int n) {
	return x >> n;
}

static inline uint32_t ch32(uint32_t x, uint32_t y, uint32_t z) {
	return u32x((x & y) ^ (~x & z));
}

static inline uint32_t maj32(uint32_t x, uint32_t y, uint32_t z) {
	return u32x((x & y) ^ (x & z) ^ (y & z));
}

static inline uint32_t bsig0_32(uint32_t x) {
	return u32x(ror32x(x, 2) ^ ror32x(x, 13) ^ ror32x(x, 22));
}

static inline uint32_t bsig1_32(uint32_t x) {
	return u32x(ror32x(x, 6) ^ ror32x(x, 11) ^ ror32x(x, 25));
}

static inline uint32_t ssig0_32(uint32_t x) {
	return u32x(ror32x(x, 7) ^ ror32x(x, 18) ^ shr32x(x, 3));
}

static inline uint32_t ssig1_32(uint32_t x) {
	return u32x(ror32x(x, 17) ^ ror32x(x, 19) ^ shr32x(x, 10));
}

void crypto_sha224(const uint8_t* data,
				   size_t		  len,
				   uint8_t		  out[CRYPTO_SHA224_DIGEST_LEN]) {
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

	uint32_t h0 = 0xc1059ed8u;
	uint32_t h1 = 0x367cd507u;
	uint32_t h2 = 0x3070dd17u;
	uint32_t h3 = 0xf70e5939u;
	uint32_t h4 = 0xffc00b31u;
	uint32_t h5 = 0x68581511u;
	uint32_t h6 = 0x64f98fa7u;
	uint32_t h7 = 0xbefa4fa4u;

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
			w[i] = u32x(ssig1_32(w[i - 2]) + w[i - 7] + ssig0_32(w[i - 15])
						+ w[i - 16]);
		}
		uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6,
				 hh = h7;
		for (int i = 0; i < 64; i++) {
			uint32_t t1 = u32x(hh + bsig1_32(e) + ch32(e, f, g) + K[i] + w[i]);
			uint32_t t2 = u32x(bsig0_32(a) + maj32(a, b, c));
			hh			= g;
			g			= f;
			f			= e;
			e			= u32x(d + t1);
			d			= c;
			c			= b;
			b			= a;
			a			= u32x(t1 + t2);
		}
		h0 = u32x(h0 + a);
		h1 = u32x(h1 + b);
		h2 = u32x(h2 + c);
		h3 = u32x(h3 + d);
		h4 = u32x(h4 + e);
		h5 = u32x(h5 + f);
		h6 = u32x(h6 + g);
		h7 = u32x(h7 + hh);
	}
	free(msg);

	uint32_t H[7] = { h0, h1, h2, h3, h4, h5, h6 };
	for (int i = 0; i < 7; i++) {
		out[i * 4 + 0] = (uint8_t) ((H[i] >> 24) & 0xffu);
		out[i * 4 + 1] = (uint8_t) ((H[i] >> 16) & 0xffu);
		out[i * 4 + 2] = (uint8_t) ((H[i] >> 8) & 0xffu);
		out[i * 4 + 3] = (uint8_t) (H[i] & 0xffu);
	}
}

static void hmac_sha2_64_inner(const uint8_t* key,
							   size_t		  key_len,
							   const uint8_t* msg,
							   size_t		  msg_len,
							   void (*hash)(const uint8_t*, size_t, uint8_t*),
							   size_t	digest_len,
							   uint8_t* out) {
	enum {
		block = 128
	};

	uint8_t* kbuf = Allocate(block);
	memset(kbuf, 0, block);
	if (key_len > block) {
		hash(key, key_len, kbuf);
		key_len = digest_len;
	} else {
		memcpy(kbuf, key, key_len);
	}
	uint8_t ipad[block], opad[block];
	for (size_t i = 0; i < block; i++) {
		ipad[i] = (uint8_t) (kbuf[i] ^ 0x36u);
		opad[i] = (uint8_t) (kbuf[i] ^ 0x5cu);
	}
	free(kbuf);

	size_t	 inner_len = block + msg_len;
	uint8_t* inner	   = Allocate(inner_len);
	memcpy(inner, ipad, block);
	memcpy(inner + block, msg, msg_len);
	uint8_t ih[64];
	hash(inner, inner_len, ih);
	free(inner);

	size_t	 outer_len = block + digest_len;
	uint8_t* outer	   = Allocate(outer_len);
	memcpy(outer, opad, block);
	memcpy(outer + block, ih, digest_len);
	hash(outer, outer_len, out);
	free(outer);
}

static void _h384(const uint8_t* d, size_t l, uint8_t* o) {
	crypto_sha384(d, l, o);
}

static void _h512(const uint8_t* d, size_t l, uint8_t* o) {
	crypto_sha512(d, l, o);
}

void crypto_hmac_sha384(const uint8_t* key,
						size_t		   key_len,
						const uint8_t* msg,
						size_t		   msg_len,
						uint8_t		   out[CRYPTO_SHA384_DIGEST_LEN]) {
	hmac_sha2_64_inner(key,
					   key_len,
					   msg,
					   msg_len,
					   _h384,
					   CRYPTO_SHA384_DIGEST_LEN,
					   out);
}

void crypto_hmac_sha512(const uint8_t* key,
						size_t		   key_len,
						const uint8_t* msg,
						size_t		   msg_len,
						uint8_t		   out[CRYPTO_SHA512_DIGEST_LEN]) {
	hmac_sha2_64_inner(key,
					   key_len,
					   msg,
					   msg_len,
					   _h512,
					   CRYPTO_SHA512_DIGEST_LEN,
					   out);
}
