#include "executable/program_services.h"

#define CAT_BUFFER_SIZE 256

uint32_t program_main(const program_services_t *services,
                      const program_startup_t *startup) {
    if (!services || !startup ||
        services->abi_version != PROGRAM_SERVICES_ABI_VERSION ||
        startup->arguments_length == 0) return 1;

    program_file_t file;
    if (services->file_open(startup->arguments, startup->arguments_length, &file)) {
        return 2;
    }

    char buffer[CAT_BUFFER_SIZE];
    while (1) {
        uint32_t bytes_read;
        if (services->file_read(&file, buffer, sizeof(buffer), &bytes_read)) {
            services->file_close(&file);
            return 3;
        }
        if (bytes_read == 0) break;

        uint32_t bytes_written;
        if (services->output_write((program_output_t *)&startup->standard_output,
                                   buffer, bytes_read, &bytes_written) ||
            bytes_written != bytes_read) {
            services->file_close(&file);
            return 4;
        }
    }
    services->file_close(&file);
    return 0;
}
