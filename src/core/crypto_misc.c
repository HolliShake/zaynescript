/**
 * Base64url (RFC 4648 §5). Original C.
 * @see https://github.com/brix/crypto-js/tree/develop/src — @c enc-base64url.js
 */
#include "./crypto.h"
#include "./crypto_algo.h"

size_t crypto_base64url_encode(const uint8_t* data, size_t len, char* out) {
	static const char* b64 =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
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
		if (has1) {
			out[o++] = b64[(n >> 6) & 63];
		}
		if (has2) {
			out[o++] = b64[n & 63];
		}
	}
	out[o] = '\0';
	return o;
}

static int b64url_index(int c) {
	if (c >= 'A' && c <= 'Z') {
		return c - 'A';
	}
	if (c >= 'a' && c <= 'z') {
		return c - 'a' + 26;
	}
	if (c >= '0' && c <= '9') {
		return c - '0' + 52;
	}
	if (c == '-') {
		return 62;
	}
	if (c == '_') {
		return 63;
	}
	return -1;
}

uint8_t* crypto_base64url_decode(const char* s, size_t* out_len) {
	if (s == NULL) {
		return NULL;
	}
	size_t cap	 = strlen(s) + 8;
	char*  clean = Allocate(cap);
	size_t c	 = 0;
	for (const char* p = s; *p; p++) {
		if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') {
			continue;
		}
		clean[c++] = *p;
	}
	clean[c] = '\0';

	/* pad to multiple of 4 */
	while (c % 4u != 0u) {
		clean[c++] = '=';
	}
	clean[c] = '\0';

	size_t	 maxout = (c / 4u) * 3u;
	uint8_t* out	= Allocate(maxout ? maxout : 1);
	size_t	 o		= 0;
	for (size_t j = 0; j + 4 <= c; j += 4) {
		int n0 = b64url_index((unsigned char) clean[j]);
		int n1 = b64url_index((unsigned char) clean[j + 1]);
		int n2 = (clean[j + 2] == '=')
					 ? 0
					 : b64url_index((unsigned char) clean[j + 2]);
		int n3 = (clean[j + 3] == '=')
					 ? 0
					 : b64url_index((unsigned char) clean[j + 3]);
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
