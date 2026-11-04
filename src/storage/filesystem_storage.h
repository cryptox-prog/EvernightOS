#ifndef FILESYSTEM_STORAGE_H
#define FILESYSTEM_STORAGE_H

#include "storage/block_device.h"

#define FILESYSTEM_BLOCK_SIZE 4096
#ifndef FILESYSTEM_FIRST_SECTOR
#error "FILESYSTEM_FIRST_SECTOR must be supplied by the build configuration"
#endif

typedef enum {
    FILESYSTEM_STORAGE_SUCCESS = 0,
    FILESYSTEM_STORAGE_ERROR_INVALID_ARGUMENT,
    FILESYSTEM_STORAGE_ERROR_NOT_READY,
    FILESYSTEM_STORAGE_ERROR_INCOMPATIBLE_DEVICE,
    FILESYSTEM_STORAGE_ERROR_OUT_OF_RANGE,
    FILESYSTEM_STORAGE_ERROR_IO
} filesystem_storage_result_t;

typedef struct {
    block_device_t *device;
    uint32_t first_sector;
    uint32_t device_blocks_per_filesystem_block;
    uint32_t total_blocks;
    uint8_t initialized;
} filesystem_storage_t;

filesystem_storage_result_t filesystem_storage_initialize(
    filesystem_storage_t *storage,
    block_device_t *device
);

filesystem_storage_result_t filesystem_storage_read_blocks(
    filesystem_storage_t *storage,
    uint32_t start_block,
    uint32_t block_count,
    void *destination
);

filesystem_storage_result_t filesystem_storage_write_blocks(
    filesystem_storage_t *storage,
    uint32_t start_block,
    uint32_t block_count,
    const void *source
);

#endif // FILESYSTEM_STORAGE_H
