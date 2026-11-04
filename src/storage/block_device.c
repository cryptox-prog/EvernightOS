#include "storage/block_device.h"

static block_device_result_t block_device_validate_request(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    const void *buffer
) {
    if (!device || !buffer || block_count == 0) {
        return BLOCK_DEVICE_ERROR_INVALID_ARGUMENT;
    }
    if (device->block_size == 0) {
        return BLOCK_DEVICE_ERROR_NOT_READY;
    }
    if (start_block >= device->total_blocks) {
        return BLOCK_DEVICE_ERROR_OUT_OF_RANGE;
    }
    if (block_count > device->total_blocks - start_block) {
        return BLOCK_DEVICE_ERROR_OUT_OF_RANGE;
    }
    return BLOCK_DEVICE_SUCCESS;
}

block_device_result_t block_device_read(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    void *destination
) {
    block_device_result_t result = block_device_validate_request(
        device,
        start_block,
        block_count,
        destination
    );
    if (result != BLOCK_DEVICE_SUCCESS) return result;
    if (!device->read_blocks) return BLOCK_DEVICE_ERROR_NOT_READY;
    return device->read_blocks(device, start_block, block_count, destination);
}

block_device_result_t block_device_write(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    const void *source
) {
    block_device_result_t result = block_device_validate_request(
        device,
        start_block,
        block_count,
        source
    );
    if (result != BLOCK_DEVICE_SUCCESS) return result;
    if (!device->write_blocks) return BLOCK_DEVICE_ERROR_NOT_READY;
    return device->write_blocks(device, start_block, block_count, source);
}
