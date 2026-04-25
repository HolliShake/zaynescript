
#include "./crypto.h"
#include "./crypto_algo.h"

static int
pkcs7_pad(const uint8_t* pt, size_t pt_len, uint8_t** padded, size_t* out_len) {
	size_t pad = CRYPTO_AES_BLOCK_SIZE - (pt_len % CRYPTO_AES_BLOCK_SIZE);
	if (pad == 0) {
		pad = CRYPTO_AES_BLOCK_SIZE;
	}
	size_t	 n	 = pt_len + pad;
	uint8_t* buf = Allocate(n);
	memcpy(buf, pt, pt_len);
	for (size_t i = 0; i < pad; i++) {
		buf[pt_len + i] = (uint8_t) pad;
	}
	*padded	 = buf;
	*out_len = n;
	return 0;
}

static int
pkcs7_unpad(const uint8_t* ct, size_t ct_len, uint8_t** pt, size_t* pt_len) {
	if (ct_len == 0 || (ct_len % CRYPTO_AES_BLOCK_SIZE) != 0) {
		return -1;
	}
	uint8_t pad = ct[ct_len - 1];
	if (pad == 0 || pad > CRYPTO_AES_BLOCK_SIZE) {
		return -1;
	}
	for (size_t i = 0; i < pad; i++) {
		if (ct[ct_len - 1 - i] != pad) {
			return -1;
		}
	}
	*pt_len = ct_len - pad;
	*pt		= Allocate(*pt_len ? *pt_len : 1);
	memcpy(*pt, ct, *pt_len);
	return 0;
}

int crypto_aes_ecb_encrypt(const uint8_t* key,
						   size_t		  key_len,
						   const uint8_t* plaintext,
						   size_t		  pt_len,
						   uint8_t**	  ct_out,
						   size_t*		  ct_len) {
	if (key_len != 16) {
		return -1;
	}
	uint8_t* padded = NULL;
	size_t	 pl		= 0;
	if (pkcs7_pad(plaintext, pt_len, &padded, &pl) != 0) {
		return -1;
	}
	struct AES_ctx ctx;
	AES_init_ctx(&ctx, key);
	for (size_t off = 0; off < pl; off += CRYPTO_AES_BLOCK_SIZE) {
		AES_ECB_encrypt(&ctx, padded + off);
	}
	*ct_out = padded;
	*ct_len = pl;
	return 0;
}

int crypto_aes_ecb_decrypt(const uint8_t* key,
						   size_t		  key_len,
						   const uint8_t* ciphertext,
						   size_t		  ct_len,
						   uint8_t**	  pt_out,
						   size_t*		  pt_len) {
	if (key_len != 16 || (ct_len % CRYPTO_AES_BLOCK_SIZE) != 0) {
		return -1;
	}
	uint8_t* buf = Allocate(ct_len);
	memcpy(buf, ciphertext, ct_len);
	struct AES_ctx ctx;
	AES_init_ctx(&ctx, key);
	for (size_t off = 0; off < ct_len; off += CRYPTO_AES_BLOCK_SIZE) {
		AES_ECB_decrypt(&ctx, buf + off);
	}
	if (pkcs7_unpad(buf, ct_len, pt_out, pt_len) != 0) {
		free(buf);
		return -1;
	}
	free(buf);
	return 0;
}

int crypto_aes_cbc_encrypt(const uint8_t* key,
						   size_t		  key_len,
						   const uint8_t* iv,
						   const uint8_t* plaintext,
						   size_t		  pt_len,
						   uint8_t**	  ct_out,
						   size_t*		  ct_len) {
	if (key_len != 16) {
		return -1;
	}
	uint8_t* padded = NULL;
	size_t	 pl		= 0;
	if (pkcs7_pad(plaintext, pt_len, &padded, &pl) != 0) {
		return -1;
	}
	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key, iv);
	AES_CBC_encrypt_buffer(&ctx, padded, pl);
	*ct_out = padded;
	*ct_len = pl;
	return 0;
}

int crypto_aes_cbc_decrypt(const uint8_t* key,
						   size_t		  key_len,
						   const uint8_t* iv,
						   const uint8_t* ciphertext,
						   size_t		  ct_len,
						   uint8_t**	  pt_out,
						   size_t*		  pt_len) {
	if (key_len != 16 || (ct_len % CRYPTO_AES_BLOCK_SIZE) != 0) {
		return -1;
	}
	uint8_t* buf = Allocate(ct_len);
	memcpy(buf, ciphertext, ct_len);
	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key, iv);
	AES_CBC_decrypt_buffer(&ctx, buf, ct_len);
	if (pkcs7_unpad(buf, ct_len, pt_out, pt_len) != 0) {
		free(buf);
		return -1;
	}
	free(buf);
	return 0;
}
