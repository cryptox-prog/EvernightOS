#include "executable/program_services.h"

uint32_t program_main(const program_services_t *services,
                      const program_startup_t *startup) {
    (void)startup;
    if (!services || services->abi_version != PROGRAM_SERVICES_ABI_VERSION ||
        !services->shutdown) return 1;
    services->shutdown();
    return 0;
}
