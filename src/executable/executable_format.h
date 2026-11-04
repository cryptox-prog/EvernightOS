#ifndef EXECUTABLE_FORMAT_H
#define EXECUTABLE_FORMAT_H

#include "utils/types.h"

// These four bytes appear in the file as the ASCII characters "EVEX".
#define EXECUTABLE_MAGIC 0x58455645
#define EXECUTABLE_FORMAT_VERSION 1

#define SHELL_LOAD_ADDRESS 0x00200000
#define CHILD_PROGRAM_LOAD_ADDRESS 0x00300000
#define SHELL_REGION_END CHILD_PROGRAM_LOAD_ADDRESS
#define CHILD_PROGRAM_REGION_END 0x00400000
#define DEFAULT_PROGRAM_STACK_SIZE 0x00010000

typedef struct {
    uint32_t magic;
    uint32_t format_version;
    uint32_t header_size;
    uint32_t load_address;
    uint32_t entry_offset;
    uint32_t image_size;
    uint32_t memory_size;
    uint32_t stack_size;
} executable_header_t;

_Static_assert(
    sizeof(executable_header_t) == 32,
    "Evernight executable header must remain 32 bytes"
);

#endif // EXECUTABLE_FORMAT_H
