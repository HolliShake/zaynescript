/**
 * PBKDF2 (RFC 2898). Original C.
 * @see https://github.com/brix/crypto-js/tree/develop/src — @c pbkdf2.js
 */
#include "./crypto.h"
#include "./crypto_algo.h"

typedef void (*crypto_hmac_fn)(const uint8_t* key,
							   size_t		  key_len,
							   const uint8_t* msg,
							   size_t		  msg_len,
							   uint8_t*		  out);

static void
_hm_md5(const uint8_t* k, size_t kl, const uint8_t* m, size_t ml, uint8_t* o) {
	crypto_hmac_md5(k, kl, m, ml, o);
}

static void
_hm_sha1(const uint8_t* k, size_t kl, const uint8_t* m, size_t ml, uint8_t* o) {
	crypto_hmac_sha1(k, kl, m, ml, o);
}

static void _hm_sha256(const uint8_t* k,
					   size_t		  kl,
					   const uint8_t* m,
					   size_t		  ml,
					   uint8_t*		  o) {
	crypto_hmac_sha256(k, kl, m, ml, o);
}

int crypto_pbkdf2(const uint8_t* password,
				  size_t		 pass_len,
				  const uint8_t* salt,
				  size_t		 salt_len,
				  uint32_t		 iterations,
				  int			 prf,
				  uint8_t*		 out,
				  size_t		 dk_len) {
	uint8_t		   U[CRYPTO_SHA256_DIGEST_LEN];
	uint8_t		   T[CRYPTO_SHA256_DIGEST_LEN];
	size_t		   hlen;
	crypto_hmac_fn hmac_fn;

	if (prf == 0) {
		hlen	= CRYPTO_SHA256_DIGEST_LEN;
		hmac_fn = _hm_sha256;
	} else if (prf == 1) {
		hlen	= CRYPTO_SHA1_DIGEST_LEN;
		hmac_fn = _hm_sha1;
	} else if (prf == 2) {
		hlen	= CRYPTO_MD5_DIGEST_LEN;
		hmac_fn = _hm_md5;
	} else {
		return -1;
	}

	uint8_t* saltblk = Allocate(salt_len + 4);
	memcpy(saltblk, salt, salt_len);

	for (uint32_t block = 1; dk_len > 0; block++) {
		saltblk[salt_len + 0] = (uint8_t) ((block >> 24) & 0xffu);
		saltblk[salt_len + 1] = (uint8_t) ((block >> 16) & 0xffu);
		saltblk[salt_len + 2] = (uint8_t) ((block >> 8) & 0xffu);
		saltblk[salt_len + 3] = (uint8_t) (block & 0xffu);

		hmac_fn(password, pass_len, saltblk, salt_len + 4, U);
		memcpy(T, U, hlen);
		for (uint32_t u = 1; u < iterations; u++) {
			hmac_fn(password, pass_len, U, hlen, U);
			for (size_t x = 0; x < hlen; x++) {
				T[x] ^= U[x];
			}
		}
		size_t cpy = dk_len < hlen ? dk_len : hlen;
		memcpy(out, T, cpy);
		out	   += cpy;
		dk_len -= cpy;
	}
	free(saltblk);
	return 0;
}
