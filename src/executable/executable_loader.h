#ifndef EXECUTABLE_LOADER_H
#define EXECUTABLE_LOADER_H

#include "filesystem/filesystem.h"
#include "executable/program_services.h"
#include "utils/types.h"

typedef enum {
    EXECUTABLE_SUCCESS = 0,
    EXECUTABLE_ERROR_INVALID_ARGUMENT,
    EXECUTABLE_ERROR_FILE,
    EXECUTABLE_ERROR_BAD_MAGIC,
    EXECUTABLE_ERROR_UNSUPPORTED_VERSION,
    EXECUTABLE_ERROR_INVALID_HEADER,
    EXECUTABLE_ERROR_INVALID_MEMORY_LAYOUT
} executable_result_t;

executable_result_t executable_run(
    filesystem_t *filesystem,
    const char *name,
    uint32_t name_length,
    uint32_t expected_load_address,
    const program_services_t *services,
    const program_startup_t *startup,
    uint32_t *exit_code
);

#endif // EXECUTABLE_LOADER_H
