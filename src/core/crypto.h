#include "../error.h"
#include "../function.h"
#include "../global.h"
#include "../hashmap.h"
#include "../value.h"

#ifndef CORE_CRYPTO_H
#	define CORE_CRYPTO_H

#	define CRYPTO_MD5_DIGEST_LEN	 16
#	define CRYPTO_SHA1_DIGEST_LEN	 20
#	define CRYPTO_SHA256_DIGEST_LEN 32

/**
 * @brief RFC 1321 MD5 over @a data; digest written to @a out (16 bytes).
 */
void crypto_md5(const uint8_t* data,
				size_t		   len,
				uint8_t		   out[CRYPTO_MD5_DIGEST_LEN]);

/**
 * @brief FIPS 180-1 SHA-1 over @a data; digest written to @a out (20 bytes).
 */
void crypto_sha1(const uint8_t* data,
				 size_t			len,
				 uint8_t		out[CRYPTO_SHA1_DIGEST_LEN]);

/**
 * @brief FIPS 180-4 SHA-256 over @a data; digest written to @a out (32 bytes).
 */
void crypto_sha256(const uint8_t* data,
				   size_t		  len,
				   uint8_t		  out[CRYPTO_SHA256_DIGEST_LEN]);

void crypto_hmac_md5(const uint8_t* key,
					 size_t			key_len,
					 const uint8_t* msg,
					 size_t			msg_len,
					 uint8_t		out[CRYPTO_MD5_DIGEST_LEN]);

void crypto_hmac_sha1(const uint8_t* key,
					  size_t		 key_len,
					  const uint8_t* msg,
					  size_t		 msg_len,
					  uint8_t		 out[CRYPTO_SHA1_DIGEST_LEN]);

void crypto_hmac_sha256(const uint8_t* key,
						size_t		   key_len,
						const uint8_t* msg,
						size_t		   msg_len,
						uint8_t		   out[CRYPTO_SHA256_DIGEST_LEN]);

/** Lowercase hex; @a out must hold at least @c 2 * len + 1 bytes (NUL). */
void crypto_hex_stringify(const uint8_t* data, size_t len, char* out);

/**
 * @brief Parses hex string into bytes allocated with @ref Allocate.
 * @return Allocated buffer or NULL on invalid input; @a *out_len set on
 * success.
 */
uint8_t* crypto_hex_parse(const char* hex, size_t* out_len);

/** RFC 4648 base64; @a out must hold at least @c 4 * ((len + 2) / 3) + 1 bytes.
 */
size_t crypto_base64_encode(const uint8_t* data, size_t len, char* out);

/** Allocates decoded bytes with @ref Allocate; caller @c free. */
uint8_t* crypto_base64_decode(const char* b64, size_t* out_len);

/**
 * @brief Loads @c core:crypto — @c Crypto aggregate (hashes, HMAC, encoders).
 * @see https://github.com/brix/crypto-js — API surface mirrored by this module.
 */
Value* LoadCoreCrypto(Interpreter* interpreter);

#endif
