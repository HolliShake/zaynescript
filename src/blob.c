#include "./blob.h"

Blob* CreateBlob(uint8_t* data, size_t size, String mimeType) {
	Blob* blob	   = Allocate(sizeof(Blob));
	blob->MimeType = AllocateString(mimeType);
	blob->Data	   = Allocate(size);
	memcpy(blob->Data, data, size);
	blob->Size = size;
	return blob;
}