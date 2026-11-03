#include "drivers/ata.h"
#include "utils/ports.h"

// Primary ATA channel registers.
#define ATA_DATA_PORT 0x1f0
#define ATA_ERROR_PORT 0x1f1
#define ATA_SECTOR_COUNT_PORT 0x1f2
#define ATA_LBA_LOW_PORT 0x1f3
#define ATA_LBA_MID_PORT 0x1f4
#define ATA_LBA_HIGH_PORT 0x1f5
#define ATA_DRIVE_SELECT_PORT 0x1f6
#define ATA_STATUS_COMMAND_PORT 0x1f7
#define ATA_ALTERNATE_STATUS_PORT 0x3f6

// Status-register bits.
#define ATA_STATUS_ERROR 0x01
#define ATA_STATUS_DATA_REQUEST 0x08
#define ATA_STATUS_DEVICE_FAULT 0x20
#define ATA_STATUS_BUSY 0x80

// Commands and drive-selection values.
#define ATA_COMMAND_READ_SECTORS 0x20
#define ATA_COMMAND_WRITE_SECTORS 0x30
#define ATA_COMMAND_CACHE_FLUSH 0xe7
#define ATA_COMMAND_IDENTIFY 0xec
#define ATA_SELECT_PRIMARY_MASTER 0xa0
#define ATA_SELECT_PRIMARY_MASTER_LBA 0xe0

#define ATA_NO_DEVICE_STATUS 0x00
#define ATA_SINGLE_SECTOR 1
#define ATA_BYTES_PER_WORD 2
#define ATA_REGISTER_BYTE_MASK 0xff
#define ATA_DRIVE_LBA_NIBBLE_MASK 0x0f
#define ATA_LBA_MID_BYTE_SHIFT 8
#define ATA_LBA_HIGH_BYTE_SHIFT 16
#define ATA_LBA_DRIVE_NIBBLE_SHIFT 24
#define ATA_POLL_LIMIT 1000000
#define ATA_IDENTIFY_LBA_SECTOR_COUNT_LOW_WORD 60
#define ATA_IDENTIFY_LBA_SECTOR_COUNT_HIGH_WORD 61

static uint32_t primary_master_sector_count;
static uint8_t primary_master_initialized;

static block_device_result_t ata_block_device_read(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    void *destination
);

static block_device_result_t ata_block_device_write(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    const void *source
);

static block_device_t primary_master_block_device = {
    .block_size = ATA_SECTOR_SIZE,
    .total_blocks = 0,
    .read_blocks = ata_block_device_read,
    .write_blocks = ata_block_device_write,
    .driver_data = 0
};

static block_device_result_t ata_convert_result(ata_result_t result) {
    if (result == ATA_SUCCESS) return BLOCK_DEVICE_SUCCESS;
    if (result == ATA_ERROR_INVALID_ARGUMENT) {
        return BLOCK_DEVICE_ERROR_INVALID_ARGUMENT;
    }
    if (result == ATA_ERROR_NOT_INITIALIZED || result == ATA_ERROR_NO_DEVICE) {
        return BLOCK_DEVICE_ERROR_NOT_READY;
    }
    if (result == ATA_ERROR_OUT_OF_RANGE) return BLOCK_DEVICE_ERROR_OUT_OF_RANGE;
    return BLOCK_DEVICE_ERROR_IO;
}

static void ata_delay_400_nanoseconds(void) {
    read_from_port(ATA_ALTERNATE_STATUS_PORT);
    read_from_port(ATA_ALTERNATE_STATUS_PORT);
    read_from_port(ATA_ALTERNATE_STATUS_PORT);
    read_from_port(ATA_ALTERNATE_STATUS_PORT);
}

static ata_result_t ata_wait_until_not_busy(void) {
    for (uint32_t poll = 0; poll < ATA_POLL_LIMIT; poll++) {
        uint8_t status = read_from_port(ATA_STATUS_COMMAND_PORT);
        if (!(status & ATA_STATUS_BUSY)) return ATA_SUCCESS;
    }
    return ATA_ERROR_TIMEOUT;
}

static ata_result_t ata_wait_for_completion(void) {
    for (uint32_t poll = 0; poll < ATA_POLL_LIMIT; poll++) {
        uint8_t status = read_from_port(ATA_STATUS_COMMAND_PORT);
        if (status & ATA_STATUS_BUSY) continue;
        if (status & ATA_STATUS_ERROR) return ATA_ERROR_COMMAND;
        if (status & ATA_STATUS_DEVICE_FAULT) return ATA_ERROR_DEVICE_FAULT;
        return ATA_SUCCESS;
    }
    return ATA_ERROR_TIMEOUT;
}

static ata_result_t ata_wait_for_data(void) {
    for (uint32_t poll = 0; poll < ATA_POLL_LIMIT; poll++) {
        uint8_t status = read_from_port(ATA_STATUS_COMMAND_PORT);
        if (status & ATA_STATUS_ERROR) return ATA_ERROR_COMMAND;
        if (status & ATA_STATUS_DEVICE_FAULT) return ATA_ERROR_DEVICE_FAULT;
        if (!(status & ATA_STATUS_BUSY) && (status & ATA_STATUS_DATA_REQUEST)) {
            return ATA_SUCCESS;
        }
    }
    return ATA_ERROR_TIMEOUT;
}

static void ata_select_lba(uint32_t lba) {
    write_to_port(
        ATA_DRIVE_SELECT_PORT,
        ATA_SELECT_PRIMARY_MASTER_LBA |
            ((lba >> ATA_LBA_DRIVE_NIBBLE_SHIFT) & ATA_DRIVE_LBA_NIBBLE_MASK)
    );
    ata_delay_400_nanoseconds();
    write_to_port(ATA_SECTOR_COUNT_PORT, ATA_SINGLE_SECTOR);
    write_to_port(ATA_LBA_LOW_PORT, lba & ATA_REGISTER_BYTE_MASK);
    write_to_port(
        ATA_LBA_MID_PORT,
        (lba >> ATA_LBA_MID_BYTE_SHIFT) & ATA_REGISTER_BYTE_MASK
    );
    write_to_port(
        ATA_LBA_HIGH_PORT,
        (lba >> ATA_LBA_HIGH_BYTE_SHIFT) & ATA_REGISTER_BYTE_MASK
    );
}

static ata_result_t ata_validate_request(
    uint32_t start_lba,
    uint32_t sector_count,
    const void *buffer
) {
    if (!primary_master_initialized) return ATA_ERROR_NOT_INITIALIZED;
    if (!buffer || sector_count == 0) return ATA_ERROR_INVALID_ARGUMENT;
    if (start_lba > ATA_LBA28_MAX) return ATA_ERROR_OUT_OF_RANGE;
    if (start_lba >= primary_master_sector_count) return ATA_ERROR_OUT_OF_RANGE;
    if (sector_count > primary_master_sector_count - start_lba) {
        return ATA_ERROR_OUT_OF_RANGE;
    }
    return ATA_SUCCESS;
}

ata_result_t ata_initialize(void) {
    uint16_t identify_data[ATA_WORDS_PER_SECTOR];

    primary_master_initialized = 0;
    primary_master_sector_count = 0;
    primary_master_block_device.total_blocks = 0;

    write_to_port(ATA_DRIVE_SELECT_PORT, ATA_SELECT_PRIMARY_MASTER);
    ata_delay_400_nanoseconds();
    write_to_port(ATA_SECTOR_COUNT_PORT, 0);
    write_to_port(ATA_LBA_LOW_PORT, 0);
    write_to_port(ATA_LBA_MID_PORT, 0);
    write_to_port(ATA_LBA_HIGH_PORT, 0);
    write_to_port(ATA_STATUS_COMMAND_PORT, ATA_COMMAND_IDENTIFY);

    uint8_t status = read_from_port(ATA_STATUS_COMMAND_PORT);
    if (status == ATA_NO_DEVICE_STATUS) return ATA_ERROR_NO_DEVICE;

    ata_result_t result = ata_wait_until_not_busy();
    if (result != ATA_SUCCESS) return result;

    // Nonzero values indicate an ATAPI device rather than an ATA disk.
    if (read_from_port(ATA_LBA_MID_PORT) != 0 ||
        read_from_port(ATA_LBA_HIGH_PORT) != 0) {
        return ATA_ERROR_UNSUPPORTED_DEVICE;
    }

    result = ata_wait_for_data();
    if (result != ATA_SUCCESS) return result;

    for (uint32_t word = 0; word < ATA_WORDS_PER_SECTOR; word++) {
        identify_data[word] = read_word_from_port(ATA_DATA_PORT);
    }

    primary_master_sector_count =
        (uint32_t)identify_data[ATA_IDENTIFY_LBA_SECTOR_COUNT_LOW_WORD] |
        ((uint32_t)identify_data[ATA_IDENTIFY_LBA_SECTOR_COUNT_HIGH_WORD]
            << ATA_LBA_HIGH_BYTE_SHIFT);

    if (primary_master_sector_count == 0) return ATA_ERROR_UNSUPPORTED_DEVICE;
    primary_master_initialized = 1;
    primary_master_block_device.total_blocks = primary_master_sector_count;
    return ATA_SUCCESS;
}

uint32_t ata_get_sector_count(void) {
    return primary_master_sector_count;
}

block_device_t *ata_get_block_device(void) {
    if (!primary_master_initialized) return 0;
    return &primary_master_block_device;
}

ata_result_t ata_read_sectors(
    uint32_t start_lba,
    uint32_t sector_count,
    void *destination
) {
    ata_result_t result = ata_validate_request(start_lba, sector_count, destination);
    if (result != ATA_SUCCESS) return result;

    uint8_t *output = destination;
    for (uint32_t sector = 0; sector < sector_count; sector++) {
        result = ata_wait_until_not_busy();
        if (result != ATA_SUCCESS) return result;

        ata_select_lba(start_lba + sector);
        write_to_port(ATA_STATUS_COMMAND_PORT, ATA_COMMAND_READ_SECTORS);

        result = ata_wait_for_data();
        if (result != ATA_SUCCESS) return result;

        for (uint32_t word = 0; word < ATA_WORDS_PER_SECTOR; word++) {
            uint16_t value = read_word_from_port(ATA_DATA_PORT);
            output[word * ATA_BYTES_PER_WORD] = value & ATA_REGISTER_BYTE_MASK;
            output[word * ATA_BYTES_PER_WORD + 1] = value >> ATA_LBA_MID_BYTE_SHIFT;
        }
        result = ata_wait_for_completion();
        if (result != ATA_SUCCESS) return result;
        output += ATA_SECTOR_SIZE;
    }
    return ATA_SUCCESS;
}

ata_result_t ata_write_sectors(
    uint32_t start_lba,
    uint32_t sector_count,
    const void *source
) {
    ata_result_t result = ata_validate_request(start_lba, sector_count, source);
    if (result != ATA_SUCCESS) return result;

    const uint8_t *input = source;
    for (uint32_t sector = 0; sector < sector_count; sector++) {
        result = ata_wait_until_not_busy();
        if (result != ATA_SUCCESS) return result;

        ata_select_lba(start_lba + sector);
        write_to_port(ATA_STATUS_COMMAND_PORT, ATA_COMMAND_WRITE_SECTORS);

        result = ata_wait_for_data();
        if (result != ATA_SUCCESS) return result;

        for (uint32_t word = 0; word < ATA_WORDS_PER_SECTOR; word++) {
            uint16_t value =
                input[word * ATA_BYTES_PER_WORD] |
                ((uint16_t)input[word * ATA_BYTES_PER_WORD + 1]
                    << ATA_LBA_MID_BYTE_SHIFT);
            write_word_to_port(ATA_DATA_PORT, value);
        }
        result = ata_wait_for_completion();
        if (result != ATA_SUCCESS) return result;
        input += ATA_SECTOR_SIZE;
    }

    write_to_port(ATA_STATUS_COMMAND_PORT, ATA_COMMAND_CACHE_FLUSH);
    return ata_wait_for_completion();
}

static block_device_result_t ata_block_device_read(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    void *destination
) {
    (void)device;
    return ata_convert_result(
        ata_read_sectors(start_block, block_count, destination)
    );
}

static block_device_result_t ata_block_device_write(
    block_device_t *device,
    uint32_t start_block,
    uint32_t block_count,
    const void *source
) {
    (void)device;
    return ata_convert_result(
        ata_write_sectors(start_block, block_count, source)
    );
}
