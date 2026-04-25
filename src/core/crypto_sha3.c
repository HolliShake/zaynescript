/**
 * SHA3 wrappers around PQClean FIPS 202 Keccak implementation.
 * @see thirdparty/crypto/pqcleanfips202/pqclean_fips202.c
 * @see https://github.com/PQClean/PQClean/tree/master/common
 * @see https://bench.cr.yp.to/supercop.html (TweetFips202/SUPERCOP origin, public domain)
 * @see https://github.com/brix/crypto-js/tree/develop/src — @c sha3.js
 */
#include "./crypto.h"
#include "./crypto_algo.h"

void crypto_sha3_224(const uint8_t* data,
					 size_t			len,
					 uint8_t		out[CRYPTO_SHA3_224_DIGEST_LEN]) {
	sha3_224((uint8_t*) out, data, len);
}

void crypto_sha3_256(const uint8_t* data,
					 size_t			len,
					 uint8_t		out[CRYPTO_SHA3_256_DIGEST_LEN]) {
	sha3_256((uint8_t*) out, data, len);
}

void crypto_sha3_384(const uint8_t* data,
					 size_t			len,
					 uint8_t		out[CRYPTO_SHA3_384_DIGEST_LEN]) {
	sha3_384((uint8_t*) out, data, len);
}

void crypto_sha3_512(const uint8_t* data,
					 size_t			len,
					 uint8_t		out[CRYPTO_SHA3_512_DIGEST_LEN]) {
	sha3_512((uint8_t*) out, data, len);
}
