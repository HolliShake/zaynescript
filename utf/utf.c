#include "utf.h"

#ifndef _UTFC_CC_H
#define _UTFC_CC_H

int utf_to_codepoint(unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4) {
    int ord = 0;
    switch (utf_size_of_utf(b1)) {
        case 1:
            return b1;

        case 2:
            ord = ((b1 & _2BYTE_FOLLOW) << 6);
            ord |= ((b2 & _MAX_TRAILING));
            break;
        case 3:
            ord = ((b1 & _3BYTE_FOLLOW) << 12);
            ord |= ((b2 & _MAX_TRAILING) << 6);
            ord |= ((b3 & _MAX_TRAILING));
            break;
        case 4:
            ord = ((b1 & _4BYTE_FOLLOW) << 18);
            ord |= ((b2 & _MAX_TRAILING) << 12);
            ord |= ((b3 & _MAX_TRAILING) << 6);
            ord |= ((b4 & _MAX_TRAILING));
            break;
        default:
            printf("DEFAULT: %c(size: %d)\n", b1, utf_size_of_utf(b1));
            invalidUtf();
            break;
    }

    if (ord == 0) {
        printf("ORD: ZERO(size: %d) [%lu, %lu, %lu, %lu]!!\n", utf_size_of_utf(b1), b1, b2, b3, b4);
        invalidUtf();
    }

    return ord;
}

int utf_encode_char(int codepoint, unsigned char buffer[5]) {
    int size = utf_size_of_codepoint(codepoint);

    switch (size) {
        case 1:
            buffer[0] = codepoint;
            buffer[1] = '\0';
            break;
        case 2:
            buffer[0] = (codepoint >> 6) | _BYTE2;
            buffer[1] = (codepoint & _MAX_TRAILING) | _VALID_TRAILING;
            buffer[2] = '\0';
            break;
        case 3:
            buffer[0] = (codepoint >> 12) | _BYTE3;
            buffer[1] = ((codepoint >> 6) & _MAX_TRAILING) | _VALID_TRAILING;
            buffer[2] = (codepoint & _MAX_TRAILING) | _VALID_TRAILING;
            buffer[3] = '\0';
            break;
        case 4:
            buffer[0] = (codepoint >> 18) | _BYTE4;
            buffer[1] = ((codepoint >> 12) & _MAX_TRAILING) | _VALID_TRAILING;
            buffer[2] = ((codepoint >> 6) & _MAX_TRAILING) | _VALID_TRAILING;
            buffer[3] = (codepoint & _MAX_TRAILING) | _VALID_TRAILING;
            buffer[4] = '\0';
            break;
        default:
            return 0;
    }

    return size;
}

int utf_size_of_utf(unsigned char firstByte) {
    if ((firstByte & _BYTE4) == _BYTE4)
        return 4;
    else if ((firstByte & _BYTE3) == _BYTE3)
        return 3;
    else if ((firstByte & _BYTE2) == _BYTE2)
        return 2;
    else if ((firstByte & _BYTE1) == 0)
        return 1;

    invalidUtf();
    return 0;
}

int utf_size_of_codepoint(int codepoint) {
    if (codepoint < 0x80)
        return 1;
    else if (codepoint < 0x000800)
        return 2;
    else if (codepoint < 0x010000)
        return 3;
    else if (codepoint < 0x110000)
        return 4;
    invalidUtf();
    return 0;
}

int utf_is_letter(int codepoint) {
    if (codepoint < 0x80) {
        if (codepoint == '_') {
            return true;
        }
        return (((unsigned int) codepoint | 0x20) - 0x61) < 26;
    }
    switch (utf8proc_category(codepoint)) {
        case UTF8PROC_CATEGORY_LU:
        case UTF8PROC_CATEGORY_LL:
        case UTF8PROC_CATEGORY_LT:
        case UTF8PROC_CATEGORY_LM:
        case UTF8PROC_CATEGORY_LO:
            return true;
    }

    return false;
}

int utf_is_digit(int codepoint) {
    if (codepoint < 0x80) {
        return ((unsigned int) codepoint - '0') < 10;
    }
    return utf8proc_category(codepoint) == UTF8PROC_CATEGORY_ND;
}

int utf_is_letter_or_digit(int codepoint) {
    if (codepoint < 0x80) {
        if (codepoint == '_') {
            return true;
        }
        if ((((unsigned int) codepoint | 0x20) - 0x61) < 26) {
            return true;
        }
        return ((unsigned int) codepoint - '0') < 10;
    }
    switch (utf8proc_category(codepoint)) {
        case UTF8PROC_CATEGORY_LU:
        case UTF8PROC_CATEGORY_LL:
        case UTF8PROC_CATEGORY_LT:
        case UTF8PROC_CATEGORY_LM:
        case UTF8PROC_CATEGORY_LO:
            return true;
        case UTF8PROC_CATEGORY_ND:
            return true;
    }
    return false;
}

int utf_is_white_space(int codepoint) {
    switch (codepoint) {
        case ' ':
        case '\b':
        case '\t':
        case '\n':
        case '\r':
            return true;
    }
    return false;
}

char* utf_rune_to_string(int codepoint) {
    size_t size = utf_size_of_codepoint(codepoint);

    unsigned char buffer[5] = {0, 0, 0, 0, 0};

    int res      = utf8proc_encode_char(codepoint, buffer);
    buffer[size] = '\0';
    if (res != size) {
        fprintf(stderr, "Failed to create string from rune.\n");
        exit(EXIT_FAILURE);
    }

    // assert
    int code;
    if ((code = utf_to_codepoint(buffer[0], buffer[1], buffer[2], buffer[3])) != codepoint) {
        fprintf(stderr, "Mismatch codepoint.\n");
        exit(EXIT_FAILURE);
    }

    char* final = malloc(sizeof(unsigned char) * (strlen(buffer) + 1));
    final[size] = '\0';

    if (final == NULL) {
        fprintf(stderr, "Failed to create string from rune.\n");
        exit(EXIT_FAILURE);
    }

    strcpy(final, buffer);
    return final;
}

char* utf_nfkc(char* _str) {
    char* str = utf8proc_NFKC((unsigned char*) str);
    if (str == NULL) {
        fprintf(stderr, "Failed to normalize string.\n");
        exit(EXIT_FAILURE);
    }
    return str;
}

size_t utf_char_length(char* str) {
    unsigned char c = (unsigned char) *str;
    return utf_size_of_utf(c);
}

int utf_char_code_at(char* _str, size_t index) {
    char* str = _str;

    for (size_t i = 0; i < index && *str; i++) {
        str += utf_char_length(str);
    }

    if (!*str) {
        return 0;
    }

    int len = utf_char_length(str);

    switch (len) {
        case 1:
            return utf_to_codepoint((unsigned char) str[0], 0, 0, 0);

        case 2:
            return utf_to_codepoint((unsigned char) str[0], (unsigned char) str[1], 0, 0);

        case 3:
            return utf_to_codepoint(
                (unsigned char) str[0],
                (unsigned char) str[1],
                (unsigned char) str[2],
                0);

        case 4:
            return utf_to_codepoint(
                (unsigned char) str[0],
                (unsigned char) str[1],
                (unsigned char) str[2],
                (unsigned char) str[3]);
        default:
            return 0;
    }
}

char* utf_char_at(char* str, size_t index) {
    return utf_rune_to_string(utf_char_code_at(str, index));
}

size_t utf_length(char* _str) {
    size_t length = 0;
    char*  str    = _str;
    while (*str) {
        str += utf_char_length(str);
        length++;
    }
    return length;
}

#endif