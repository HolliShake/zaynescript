/**
 * @file blob.h
 * @brief Definition of the Blob structure and related functions for
 * handling binary data in the interpreter.
 *
 * This file defines the Blob structure, which represents binary
 * data along with its MIME type, and declares functions for creating
 * Blob values within the interpreter.
 */

#include "./global.h"

#ifndef BLOB_H
#	define BLOB_H

/**
 * @brief Creates a new Blob from raw binary data.
 * @param data Pointer to the binary data.
 * @param size The size of the data in bytes.
 * @param mimeType The MIME type of the data.
 * @return A new Blob instance (caller must free).
 */
Blob* CreateBlob(uint8_t* data, size_t size, String mimeType);

#endif