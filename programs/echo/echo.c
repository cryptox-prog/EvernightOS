#include "executable/program_services.h"

uint32_t program_main(const program_services_t *services,
                      const program_startup_t *startup) {
    if (!services || !startup ||
        services->abi_version != PROGRAM_SERVICES_ABI_VERSION) return 1;

    uint32_t bytes_written;
    if (startup->arguments_length &&
        services->output_write((program_output_t *)&startup->standard_output,
                               startup->arguments,
                               startup->arguments_length,
                               &bytes_written)) return 2;

    char newline = '\n';
    if (services->output_write((program_output_t *)&startup->standard_output,
                               &newline, 1, &bytes_written)) return 2;
    return 0;
}
