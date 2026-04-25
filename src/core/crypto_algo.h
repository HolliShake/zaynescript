/**
 * @file crypto_algo.h
 * @brief Extended digests, KDFs, and ciphers aligned with CryptoJS @c
 * develop/src modules (original C; @see
 * https://github.com/brix/crypto-js/tree/develop/src).
 * @see thirdparty/crypto/pqcleanfips202/ (PQClean FIPS 202 Keccak,
 * TweetFips202/SUPERCOP lineage)
 */
#ifndef CORE_CRYPTO_ALGO_H
#define CORE_CRYPTO_ALGO_H

#include "../../thirdparty/crypto/pqcleanfips202/pqclean_fips202.h"
#include "../../thirdparty/crypto/RHash/librhash/ripemd-160.h"
#include "../../thirdparty/crypto/tiny-AES-c/aes.h"
#include "../global.h"


#define CRYPTO_SHA224_DIGEST_LEN	28
#define CRYPTO_SHA384_DIGEST_LEN	48
#define CRYPTO_SHA512_DIGEST_LEN	64
#define CRYPTO_SHA3_224_DIGEST_LEN	28
#define CRYPTO_SHA3_256_DIGEST_LEN	32
#define CRYPTO_SHA3_384_DIGEST_LEN	48
#define CRYPTO_SHA3_512_DIGEST_LEN	64
#define CRYPTO_RIPEMD160_DIGEST_LEN 20
#define CRYPTO_DES_BLOCK_SIZE		8
#define CRYPTO_AES_BLOCK_SIZE		16

void crypto_sha224(const uint8_t* data,
				   size_t		  len,
				   uint8_t		  out[CRYPTO_SHA224_DIGEST_LEN]);
void crypto_sha384(const uint8_t* data,
				   size_t		  len,
				   uint8_t		  out[CRYPTO_SHA384_DIGEST_LEN]);
void crypto_sha512(const uint8_t* data,
				   size_t		  len,
				   uint8_t		  out[CRYPTO_SHA512_DIGEST_LEN]);

void crypto_hmac_sha384(const uint8_t* key,
						size_t		   key_len,
						const uint8_t* msg,
						size_t		   msg_len,
						uint8_t		   out[CRYPTO_SHA384_DIGEST_LEN]);
void crypto_hmac_sha512(const uint8_t* key,
						size_t		   key_len,
						const uint8_t* msg,
						size_t		   msg_len,
						uint8_t		   out[CRYPTO_SHA512_DIGEST_LEN]);

void crypto_sha3_224(const uint8_t* data,
					 size_t			len,
					 uint8_t		out[CRYPTO_SHA3_224_DIGEST_LEN]);
void crypto_sha3_256(const uint8_t* data,
					 size_t			len,
					 uint8_t		out[CRYPTO_SHA3_256_DIGEST_LEN]);
void crypto_sha3_384(const uint8_t* data,
					 size_t			len,
					 uint8_t		out[CRYPTO_SHA3_384_DIGEST_LEN]);
void crypto_sha3_512(const uint8_t* data,
					 size_t			len,
					 uint8_t		out[CRYPTO_SHA3_512_DIGEST_LEN]);

void crypto_ripemd160(const uint8_t* data,
					  size_t		 len,
					  uint8_t		 out[CRYPTO_RIPEMD160_DIGEST_LEN]);

void crypto_hmac_ripemd160(const uint8_t* key,
						   size_t		  key_len,
						   const uint8_t* msg,
						   size_t		  msg_len,
						   uint8_t		  out[CRYPTO_RIPEMD160_DIGEST_LEN]);

/** PBKDF2 (RFC 2898). @a dkLen output length; @a prf 0=SHA256 1=SHA1 2=MD5 */
int crypto_pbkdf2(const uint8_t* password,
				  size_t		 pass_len,
				  const uint8_t* salt,
				  size_t		 salt_len,
				  uint32_t		 iterations,
				  int			 prf,
				  uint8_t*		 out,
				  size_t		 dk_len);

/**
 * @brief OpenSSL EVP_BytesToKey MD5 style (crypto-js @c evpkdf.js).
 * @return 0 on success.
 */
int crypto_evpkdf_md5(const uint8_t* password,
					  size_t		 pass_len,
					  const uint8_t* salt,
					  size_t		 salt_len,
					  uint32_t		 iterations,
					  size_t		 key_len,
					  size_t		 iv_len,
					  uint8_t*		 key_out,
					  uint8_t*		 iv_out);

/** RC4 drop-n = 0 for standard. */
void crypto_rc4(const uint8_t* key,
				size_t		   key_len,
				unsigned	   drop_n,
				const uint8_t* data,
				size_t		   len,
				uint8_t*	   out);

/** AES-128 only (16-byte key). ECB/CBC, PKCS#7. @a ct_out allocated with @ref
 * Allocate. */
int crypto_aes_ecb_encrypt(const uint8_t* key,
						   size_t		  key_len,
						   const uint8_t* plaintext,
						   size_t		  pt_len,
						   uint8_t**	  ct_out,
						   size_t*		  ct_len);

int crypto_aes_ecb_decrypt(const uint8_t* key,
						   size_t		  key_len,
						   const uint8_t* ciphertext,
						   size_t		  ct_len,
						   uint8_t**	  pt_out,
						   size_t*		  pt_len);

/** CBC: @a iv 16 bytes. */
int crypto_aes_cbc_encrypt(const uint8_t* key,
						   size_t		  key_len,
						   const uint8_t* iv,
						   const uint8_t* plaintext,
						   size_t		  pt_len,
						   uint8_t**	  ct_out,
						   size_t*		  ct_len);

int crypto_aes_cbc_decrypt(const uint8_t* key,
						   size_t		  key_len,
						   const uint8_t* iv,
						   const uint8_t* ciphertext,
						   size_t		  ct_len,
						   uint8_t**	  pt_out,
						   size_t*		  pt_len);

/** RFC 4648 base64url (no padding in output). @a out cap >= 4*((len+2)/3)+4 */
size_t	 crypto_base64url_encode(const uint8_t* data, size_t len, char* out);
uint8_t* crypto_base64url_decode(const char* s, size_t* out_len);

#endif
