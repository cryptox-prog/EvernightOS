#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "storage/filesystem_storage.h"
#include "utils/types.h"

#define FILESYSTEM_MAGIC 0x45564e46
#define FILESYSTEM_VERSION 1
#define FILESYSTEM_TOTAL_INODES 128
#define FILESYSTEM_ROOT_INODE 0
#define FILESYSTEM_INLINE_EXTENT_COUNT 6
#define FILESYSTEM_INODE_SIZE 128
#define FILESYSTEM_MAX_FILE_SIZE 0xffffffffU
#define FILESYSTEM_EXTENTS_PER_OVERFLOW_BLOCK 340
#define FILESYSTEM_MAX_FILE_NAME_LENGTH 56
#define FILESYSTEM_BITS_PER_BITMAP_BLOCK (FILESYSTEM_BLOCK_SIZE * 8)

#define FILESYSTEM_SUPERBLOCK_BLOCK 0
#define FILESYSTEM_BLOCK_BITMAP_BLOCK 1
#define FILESYSTEM_INODE_BITMAP_BLOCK 2
#define FILESYSTEM_INODE_TABLE_START_BLOCK 3
#define FILESYSTEM_INODES_PER_BLOCK (FILESYSTEM_BLOCK_SIZE / FILESYSTEM_INODE_SIZE)
#define FILESYSTEM_INODE_TABLE_BLOCK_COUNT \
    (FILESYSTEM_TOTAL_INODES / FILESYSTEM_INODES_PER_BLOCK)
#define FILESYSTEM_DATA_START_BLOCK \
    (FILESYSTEM_INODE_TABLE_START_BLOCK + FILESYSTEM_INODE_TABLE_BLOCK_COUNT)

typedef uint32_t inode_number_t;

typedef enum {
    FILESYSTEM_INODE_UNUSED = 0,
    FILESYSTEM_INODE_REGULAR_FILE,
    FILESYSTEM_INODE_DIRECTORY
} filesystem_inode_type_t;

typedef struct {
    uint32_t logical_start_block;
    uint32_t disk_start_block;
    uint32_t block_count;
} filesystem_extent_t;

typedef struct {
    uint32_t type;
    uint32_t size_bytes;
    uint32_t extent_count;
    filesystem_extent_t inline_extents[FILESYSTEM_INLINE_EXTENT_COUNT];
    uint32_t first_overflow_extent_block;
    uint32_t reserved[10];
} filesystem_inode_t;

typedef struct {
    uint32_t extent_count;
    uint32_t next_extent_block;
    filesystem_extent_t extents[FILESYSTEM_EXTENTS_PER_OVERFLOW_BLOCK];
    uint32_t reserved[2];
} filesystem_extent_block_t;

typedef struct {
    inode_number_t inode_number;
    uint32_t name_length;
    char name[FILESYSTEM_MAX_FILE_NAME_LENGTH];
} filesystem_directory_entry_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t block_bitmap_block;
    uint32_t inode_bitmap_block;
    uint32_t inode_table_start_block;
    uint32_t inode_table_block_count;
    uint32_t data_start_block;
    uint32_t total_inodes;
    uint32_t root_inode;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t reserved[3];
} filesystem_superblock_t;

typedef enum {
    FILESYSTEM_SUCCESS = 0,
    FILESYSTEM_ERROR_INVALID_ARGUMENT,
    FILESYSTEM_ERROR_NOT_MOUNTED,
    FILESYSTEM_ERROR_BAD_MAGIC,
    FILESYSTEM_ERROR_UNSUPPORTED_VERSION,
    FILESYSTEM_ERROR_CORRUPT,
    FILESYSTEM_ERROR_OUT_OF_RANGE,
    FILESYSTEM_ERROR_NO_SPACE,
    FILESYSTEM_ERROR_NOT_FOUND,
    FILESYSTEM_ERROR_ALREADY_EXISTS,
    FILESYSTEM_ERROR_STORAGE
} filesystem_result_t;

typedef struct {
    filesystem_storage_t *storage;
    filesystem_superblock_t superblock;
    uint8_t block_buffer[FILESYSTEM_BLOCK_SIZE];
    uint8_t mounted;
} filesystem_t;

typedef struct {
    filesystem_t *filesystem;
    inode_number_t inode_number;
    uint32_t position;
} filesystem_file_t;

filesystem_result_t filesystem_format(filesystem_storage_t *storage);

filesystem_result_t filesystem_mount(
    filesystem_t *filesystem,
    filesystem_storage_t *storage
);

filesystem_result_t filesystem_read_inode(
    filesystem_t *filesystem,
    inode_number_t inode_number,
    filesystem_inode_t *inode
);

filesystem_result_t filesystem_write_inode(
    filesystem_t *filesystem,
    inode_number_t inode_number,
    const filesystem_inode_t *inode
);

filesystem_result_t filesystem_allocate_block(
    filesystem_t *filesystem,
    uint32_t *allocated_block
);

filesystem_result_t filesystem_allocate_inode(
    filesystem_t *filesystem,
    filesystem_inode_type_t type,
    inode_number_t *allocated_inode
);

filesystem_result_t filesystem_read_inode_data(
    filesystem_t *filesystem,
    inode_number_t inode_number,
    uint32_t byte_offset,
    uint32_t byte_count,
    void *destination,
    uint32_t *bytes_read
);

filesystem_result_t filesystem_write_inode_data(
    filesystem_t *filesystem,
    inode_number_t inode_number,
    uint32_t byte_offset,
    uint32_t byte_count,
    const void *source,
    uint32_t *bytes_written
);

filesystem_result_t filesystem_create_file(
    filesystem_t *filesystem,
    const char *name,
    uint32_t name_length,
    filesystem_file_t *file
);

filesystem_result_t filesystem_open_file(
    filesystem_t *filesystem,
    const char *name,
    uint32_t name_length,
    filesystem_file_t *file
);

filesystem_result_t filesystem_read_file(
    filesystem_file_t *file,
    void *destination,
    uint32_t byte_count,
    uint32_t *bytes_read
);

filesystem_result_t filesystem_write_file(
    filesystem_file_t *file,
    const void *source,
    uint32_t byte_count,
    uint32_t *bytes_written
);

filesystem_result_t filesystem_seek_file(
    filesystem_file_t *file,
    uint32_t position
);

filesystem_result_t filesystem_close_file(filesystem_file_t *file);

#endif // FILESYSTEM_H
