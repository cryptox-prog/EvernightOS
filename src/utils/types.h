#ifndef TYPES_H
#define TYPES_H

#define MAX_STRING_SIZE 1024

/// @brief Designed with respect to the gcc with -m32 flag compiler to be 8-bits 16-bits and 32-bits
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

typedef struct string {
    char str[MAX_STRING_SIZE];
    uint32_t length;
} string_t;

static inline string_t create_string(const char* c_str) {
    string_t new_string;
    new_string.length = 0;

    if (c_str) { // handle NULL input safely
        while (c_str[new_string.length] != '\0' && new_string.length < MAX_STRING_SIZE - 1) {
            new_string.str[new_string.length] = c_str[new_string.length];
            new_string.length++;
        }
    }

    new_string.str[new_string.length] = '\0'; // Null-terminate
    return new_string;
}

#endif // TYPES_H
