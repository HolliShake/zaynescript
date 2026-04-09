#include "./global.h"

#ifndef BLOB_H
#	define BLOB_H

/**
 * @file blob.h
 * @brief Definition of the Blob structure and related functions for
 * handling binary data in the interpreter.
 *
 * This file defines the Blob structure, which represents binary
 * data along with its MIME type, and declares functions for creating
 * Blob values within the interpreter.
 */
Blob* CreateBlob(uint8_t* data, size_t size, String mimeType);

#endif