
#include "./crypto.h"
#include "./crypto_algo.h"

int crypto_evpkdf_md5(const uint8_t* password,
					  size_t		 pass_len,
					  const uint8_t* salt,
					  size_t		 salt_len,
					  uint32_t		 iterations,
					  size_t		 key_len,
					  size_t		 iv_len,
					  uint8_t*		 key_out,
					  uint8_t*		 iv_out) {
	size_t	 need = key_len + iv_len;
	uint8_t* out  = Allocate(need);
	size_t	 pos  = 0;
	uint8_t	 digest[CRYPTO_MD5_DIGEST_LEN];
	uint8_t	 data[4096];
	size_t	 dlen;

	while (pos < need) {
		if (pos == 0) {
			dlen = 0;
			if (dlen + pass_len + salt_len > sizeof(data)) {
				free(out);
				return -1;
			}
			memcpy(data + dlen, password, pass_len);
			dlen += pass_len;
			memcpy(data + dlen, salt, salt_len);
			dlen += salt_len;
		} else {
			dlen = 0;
			if (dlen + sizeof(digest) + pass_len + salt_len > sizeof(data)) {
				free(out);
				return -1;
			}
			memcpy(data + dlen, digest, sizeof(digest));
			dlen += sizeof(digest);
			memcpy(data + dlen, password, pass_len);
			dlen += pass_len;
			memcpy(data + dlen, salt, salt_len);
			dlen += salt_len;
		}
		crypto_md5(data, dlen, digest);
		for (uint32_t it = 1; it < iterations; it++) {
			crypto_md5(digest, sizeof(digest), digest);
		}
		size_t copy = need - pos;
		if (copy > sizeof(digest)) {
			copy = sizeof(digest);
		}
		memcpy(out + pos, digest, copy);
		pos += copy;
	}
	memcpy(key_out, out, key_len);
	memcpy(iv_out, out + key_len, iv_len);
	free(out);
	return 0;
}
