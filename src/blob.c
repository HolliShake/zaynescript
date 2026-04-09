#include "./blob.h"

Blob* CreateBlob(uint8_t* data, size_t size, String mime_type) {
	Blob* blob		= Allocate(sizeof(Blob));
	blob->mime_type = AllocateString(mime_type);
	blob->data		= Allocate(size);
	memcpy(blob->data, data, size);
	blob->size = size;
	return blob;
}