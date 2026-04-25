/**
 * RC4 stream cipher. Original C.
 * @see https://github.com/brix/crypto-js/tree/develop/src — @c rc4.js
 */
#include "./crypto.h"
#include "./crypto_algo.h"

void crypto_rc4(const uint8_t* key,
				size_t		   key_len,
				unsigned	   drop_n,
				const uint8_t* data,
				size_t		   len,
				uint8_t*	   out) {
	uint8_t S[256];
	for (int i = 0; i < 256; i++) {
		S[i] = (uint8_t) i;
	}
	size_t j = 0;
	for (int i = 0; i < 256; i++) {
		j		  = (j + S[i] + key[i % key_len]) % 256u;
		uint8_t t = S[i];
		S[i]	  = S[j];
		S[j]	  = t;
	}
	size_t i = 0;
	j		 = 0;
	for (unsigned d = 0; d < drop_n; d++) {
		i		  = (i + 1) % 256u;
		j		  = (j + S[i]) % 256u;
		uint8_t t = S[i];
		S[i]	  = S[j];
		S[j]	  = t;
	}
	for (size_t k = 0; k < len; k++) {
		i		  = (i + 1) % 256u;
		j		  = (j + S[i]) % 256u;
		uint8_t t = S[i];
		S[i]	  = S[j];
		S[j]	  = t;
		out[k]	  = data[k] ^ S[(S[i] + S[j]) % 256u];
	}
}
