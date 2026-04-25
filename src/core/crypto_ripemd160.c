/**
 * RIPEMD-160 via RHash (permissive license in @c thirdparty/crypto/RHash).
 * @see https://github.com/brix/crypto-js/tree/develop/src — @c ripemd160.js
 */
#include "./crypto.h"
#include "./crypto_algo.h"

void crypto_ripemd160(const uint8_t* data,
					  size_t		 len,
					  uint8_t		 out[CRYPTO_RIPEMD160_DIGEST_LEN]) {
	ripemd160_ctx ctx;
	rhash_ripemd160_init(&ctx);
	rhash_ripemd160_update(&ctx, data, len);
	rhash_ripemd160_final(&ctx, out);
}

static void hmac_ripemd160_inner(const uint8_t* key,
								 size_t			key_len,
								 const uint8_t* msg,
								 size_t			msg_len,
								 uint8_t*		out) {
	enum {
		block = 64
	};

	uint8_t* kbuf = Allocate(block);
	memset(kbuf, 0, block);
	if (key_len > block) {
		crypto_ripemd160(key, key_len, kbuf);
		key_len = CRYPTO_RIPEMD160_DIGEST_LEN;
	} else {
		memcpy(kbuf, key, key_len);
	}
	uint8_t *ipad = Allocate(block), *opad = Allocate(block);
	for (size_t i = 0; i < block; i++) {
		ipad[i] = (uint8_t) (kbuf[i] ^ 0x36u);
		opad[i] = (uint8_t) (kbuf[i] ^ 0x5cu);
	}
	free(kbuf);
	size_t	 ilen  = block + msg_len;
	uint8_t* inner = Allocate(ilen);
	memcpy(inner, ipad, block);
	memcpy(inner + block, msg, msg_len);
	free(ipad);
	uint8_t ih[CRYPTO_RIPEMD160_DIGEST_LEN];
	crypto_ripemd160(inner, ilen, ih);
	free(inner);
	size_t	 olen  = block + CRYPTO_RIPEMD160_DIGEST_LEN;
	uint8_t* outer = Allocate(olen);
	memcpy(outer, opad, block);
	memcpy(outer + block, ih, CRYPTO_RIPEMD160_DIGEST_LEN);
	free(opad);
	crypto_ripemd160(outer, olen, out);
	free(outer);
}

void crypto_hmac_ripemd160(const uint8_t* key,
						   size_t		  key_len,
						   const uint8_t* msg,
						   size_t		  msg_len,
						   uint8_t		  out[CRYPTO_RIPEMD160_DIGEST_LEN]) {
	hmac_ripemd160_inner(key, key_len, msg, msg_len, out);
}
