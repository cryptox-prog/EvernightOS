#ifndef ATA_H
#define ATA_H

#include "utils/types.h"
#include "storage/block_device.h"

#define ATA_SECTOR_SIZE 512
#define ATA_WORDS_PER_SECTOR (ATA_SECTOR_SIZE / 2)
#define ATA_LBA28_MAX 0x0fffffff

typedef enum {
    ATA_SUCCESS = 0,
    ATA_ERROR_INVALID_ARGUMENT,
    ATA_ERROR_NOT_INITIALIZED,
    ATA_ERROR_NO_DEVICE,
    ATA_ERROR_UNSUPPORTED_DEVICE,
    ATA_ERROR_OUT_OF_RANGE,
    ATA_ERROR_DEVICE_FAULT,
    ATA_ERROR_COMMAND,
    ATA_ERROR_TIMEOUT
} ata_result_t;

/// @brief Detect and initialize the primary master ATA drive.
ata_result_t ata_initialize(void);

/// @brief Return the number of addressable 512-byte sectors.
uint32_t ata_get_sector_count(void);

/// @brief Return the generic block-device interface for the detected disk.
block_device_t *ata_get_block_device(void);

/// @brief Read complete sectors from the primary master ATA drive.
ata_result_t ata_read_sectors(
    uint32_t start_lba,
    uint32_t sector_count,
    void *destination
);

/// @brief Write complete sectors to the primary master ATA drive.
ata_result_t ata_write_sectors(
    uint32_t start_lba,
    uint32_t sector_count,
    const void *source
);

#endif // ATA_H
