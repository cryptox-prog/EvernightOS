#ifndef PROGRAM_SERVICES_H
#define PROGRAM_SERVICES_H

#include "utils/types.h"

#define PROGRAM_SERVICES_ABI_VERSION 1

#define PROGRAM_COLOR_BLACK 0
#define PROGRAM_COLOR_LIGHT_GREY 7
#define PROGRAM_COLOR_LIGHT_RED 12

#define PROGRAM_OUTPUT_TERMINAL 0
#define PROGRAM_OUTPUT_FILE 1

typedef struct {
    uint32_t private_data[3];
} program_file_t;

typedef struct {
    uint32_t type;
    program_file_t file;
} program_output_t;

typedef struct {
    const char *arguments;
    uint32_t arguments_length;
    program_output_t standard_output;
} program_startup_t;

typedef struct {
    uint32_t abi_version;
    void (*terminal_clear)(void);
    void (*terminal_write_character)(char character);
    char (*keyboard_read_character)(void);
    uint32_t (*file_create)(
        const char *name,
        uint32_t name_length,
        program_file_t *file
    );
    uint32_t (*file_open)(
        const char *name,
        uint32_t name_length,
        program_file_t *file
    );
    uint32_t (*file_read)(
        program_file_t *file,
        void *destination,
        uint32_t byte_count,
        uint32_t *bytes_read
    );
    uint32_t (*file_write)(
        program_file_t *file,
        const void *source,
        uint32_t byte_count,
        uint32_t *bytes_written
    );
    uint32_t (*file_close)(program_file_t *file);
    uint32_t (*output_write)(
        program_output_t *output,
        const void *source,
        uint32_t byte_count,
        uint32_t *bytes_written
    );
    uint32_t (*execute)(
        const char *name,
        uint32_t name_length,
        const program_startup_t *startup,
        uint32_t *exit_code
    );
    void (*shutdown)(void);
    void (*terminal_recolor_previous)(
        uint32_t character_count,
        uint8_t foreground,
        uint8_t background
    );
} program_services_t;

#endif // PROGRAM_SERVICES_H
