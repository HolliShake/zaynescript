#include "utf8proc/utf8proc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://github.com/andydevs69420/atom-compiler-c/blob/main/identifier/utf8.c

#ifndef _UTF8C_H
#define _UTF8C_H

#define invalidUtf()                                                                               \
    {                                                                                              \
        fprintf(stderr, "UTF::%s:Error: Invalid utf!!\n", __func__);                               \
        exit(1);                                                                                   \
    }

enum UTF
{
    _BYTE1 = 0b00000000,
    _BYTE2 = 0b11000000,
    _BYTE3 = 0b11100000,
    _BYTE4 = 0b11110000,

    _2BYTE_FOLLOW = 0b00011111,
    _3BYTE_FOLLOW = 0b00001111,
    _4BYTE_FOLLOW = 0b00000111,

    _VALID_TRAILING = 0b10000000,
    _MAX_TRAILING   = 0b00111111
};

/**
 * @brief Converts the given utf bytes to a codepoint.
 * @param b1 The first byte.
 * @param b2 The second byte.
 * @param b3 The third byte.
 * @param b4 The fourth byte.
 * @return The codepoint.
 */
int utf_to_codepoint(unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4);

/**
 * @brief Encodes the given codepoint to utf bytes.
 * @param codepoint The codepoint to encode.
 * @param buffer The buffer to store the utf bytes.
 * @return The size of the utf bytes.
 */
int utf_encode_char(int codepoint, unsigned char buffer[5]);

/**
 * @brief Returns the size of the utf bytes.
 * @param firstByte The first byte.
 * @return The size of the utf bytes.
 */
int utf_size_of_utf(unsigned char);

/**
 * @brief Returns the size of the codepoint.
 * @param codepoint The codepoint.
 * @return The size of the codepoint.
 */
int utf_size_of_codepoint(int codepoint);

/**
 * @brief Returns whether the given codepoint is a letter.
 * @param codepoint The codepoint.
 * @return Whether the codepoint is a letter.
 */
int utf_is_letter(int codepoint);

/**
 * @brief Returns whether the given codepoint is a digit.
 * @param codepoint The codepoint.
 * @return Whether the codepoint is a digit.
 */
int utf_is_digit(int codepoint);

/**
 * @brief Returns whether the given codepoint is a letter or digit.
 * @param codepoint The codepoint.
 * @return Whether the codepoint is a letter or digit.
 */
int utf_is_letter_or_digit(int codepoint);

/**
 * @brief Returns whether the given codepoint is a white space.
 * @param codepoint The codepoint.
 * @return Whether the codepoint is a white space.
 */
int utf_is_white_space(int codepoint);

/**
 * @brief Converts the given codepoint to a string.
 * @param codepoint The codepoint.
 * @return The string.
 */
char* utf_rune_to_string(int codepoint);

/**
 * @brief Returns the length of the utf string.
 * @param str The utf string.
 * @return The length of the utf string.
 */
char* utf_nfkc(char* str);

/**
 * @brief Returns the length of the utf string.
 * @param str The utf string.
 * @return The length of the utf string.
 */
size_t utf_char_length(char* str);

/**
 * @brief Returns the length of the utf string.
 * @param str The utf string.
 * @return The length of the utf string.
 */
int utf_char_code_at(char* str, size_t index);

/**
 * @brief Returns the length of the utf string.
 * @param str The utf string.
 * @return The length of the utf string.
 */
char* utf_char_at(char* str, size_t index);

/**
 * @brief Returns the length of the utf string.
 * @param str The utf string.
 * @return The length of the utf string.
 */
size_t utf_length(char* str);

#endif