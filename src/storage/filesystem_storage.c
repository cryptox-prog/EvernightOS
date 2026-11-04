#include "storage/filesystem_storage.h"

static filesystem_storage_result_t filesystem_storage_convert_result(
    block_device_result_t result
) {
    if (result == BLOCK_DEVICE_SUCCESS) return FILESYSTEM_STORAGE_SUCCESS;
    if (result == BLOCK_DEVICE_ERROR_INVALID_ARGUMENT) {
        return FILESYSTEM_STORAGE_ERROR_INVALID_ARGUMENT;
    }
    if (result == BLOCK_DEVICE_ERROR_NOT_READY) {
        return FILESYSTEM_STORAGE_ERROR_NOT_READY;
    }
    if (result == BLOCK_DEVICE_ERROR_OUT_OF_RANGE) {
        return FILESYSTEM_STORAGE_ERROR_OUT_OF_RANGE;
    }
    return FILESYSTEM_STORAGE_ERROR_IO;
}

filesystem_storage_result_t filesystem_storage_initialize(
    filesystem_storage_t *storage,
    block_device_t *device
) {
    if (!storage || !device) return FILESYSTEM_STORAGE_ERROR_INVALID_ARGUMENT;

    storage->initialized = 0;
    storage->device = 0;
    storage->first_sector = 0;
    storage->device_blocks_per_filesystem_block = 0;
    storage->total_blocks = 0;

    if (device->block_size == 0 || FILESYSTEM_BLOCK_SIZE % device->block_size != 0) {
        return FILESYSTEM_STORAGE_ERROR_INCOMPATIBLE_DEVICE;
    }
    if (FILESYSTEM_FIRST_SECTOR >= device->total_blocks) {
        return FILESYSTEM_STORAGE_ERROR_OUT_OF_RANGE;
    }

    storage->device = device;
    storage->first_sector = FILESYSTEM_FIRST_SECTOR;
    storage->device_blocks_per_filesystem_block =
        FILESYSTEM_BLOCK_SIZE / device->block_size;
    storage->total_blocks =
        (device->total_blocks - storage->first_sector) /
        storage->device_blocks_per_filesystem_block;

    if (storage->total_blocks == 0) {
        return FILESYSTEM_STORAGE_ERROR_OUT_OF_RANGE;
    }

    storage->initialized = 1;
    return FILESYSTEM_STORAGE_SUCCESS;
}

static filesystem_storage_result_t filesystem_storage_validate_request(
    filesystem_storage_t *storage,
    uint32_t start_block,
    uint32_t block_count,
    const void *buffer
) {
    if (!storage || !buffer || block_count == 0) {
        return FILESYSTEM_STORAGE_ERROR_INVALID_ARGUMENT;
    }
    if (!storage->initialized || !storage->device) {
        return FILESYSTEM_STORAGE_ERROR_NOT_READY;
    }
    if (start_block >= storage->total_blocks) {
        return FILESYSTEM_STORAGE_ERROR_OUT_OF_RANGE;
    }
    if (block_count > storage->total_blocks - start_block) {
        return FILESYSTEM_STORAGE_ERROR_OUT_OF_RANGE;
    }
    return FILESYSTEM_STORAGE_SUCCESS;
}

filesystem_storage_result_t filesystem_storage_read_blocks(
    filesystem_storage_t *storage,
    uint32_t start_block,
    uint32_t block_count,
    void *destination
) {
    filesystem_storage_result_t result = filesystem_storage_validate_request(
        storage,
        start_block,
        block_count,
        destination
    );
    if (result != FILESYSTEM_STORAGE_SUCCESS) return result;

    uint32_t first_sector =
        storage->first_sector +
        start_block * storage->device_blocks_per_filesystem_block;
    uint32_t sector_count =
        block_count * storage->device_blocks_per_filesystem_block;
    return filesystem_storage_convert_result(
        block_device_read(storage->device, first_sector, sector_count, destination)
    );
}

filesystem_storage_result_t filesystem_storage_write_blocks(
    filesystem_storage_t *storage,
    uint32_t start_block,
    uint32_t block_count,
    const void *source
) {
    filesystem_storage_result_t result = filesystem_storage_validate_request(
        storage,
        start_block,
        block_count,
        source
    );
    if (result != FILESYSTEM_STORAGE_SUCCESS) return result;

    uint32_t first_sector =
        storage->first_sector +
        start_block * storage->device_blocks_per_filesystem_block;
    uint32_t sector_count =
        block_count * storage->device_blocks_per_filesystem_block;
    return filesystem_storage_convert_result(
        block_device_write(storage->device, first_sector, sector_count, source)
    );
}
