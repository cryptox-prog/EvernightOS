#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include "utils/types.h"

typedef enum {
    BLOCK_DEVICE_SUCCESS = 0,
    BLOCK_DEVICE_ERROR_INVALID_ARGUMENT,
    BLOCK_DEVICE_ERROR_NOT_READY,
    BLOCK_DEVICE_ERROR_OUT_OF_RANGE,
    BLOCK_DEVICE_ERROR_IO
} block_device_result_t;

typedef struct block_device block_device_t;

typedef block_device_result_t (*block_device_read_function_t)(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    void *destination
);

typedef block_device_result_t (*block_device_write_function_t)(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    const void *source
);

struct block_device {
    uint32_t block_size;
    uint32_t total_blocks;
    block_device_read_function_t read_blocks;
    block_device_write_function_t write_blocks;
    void *driver_data;
};

block_device_result_t block_device_read(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    void *destination
);

block_device_result_t block_device_write(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    const void *source
);

#endif // BLOCK_DEVICE_H
