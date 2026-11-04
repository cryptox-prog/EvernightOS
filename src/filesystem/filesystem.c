#include "filesystem/filesystem.h"
#include "utils/memory.h"

_Static_assert(
    sizeof(filesystem_inode_t) == FILESYSTEM_INODE_SIZE,
    "filesystem inode size does not match its on-disk format"
);

_Static_assert(
    sizeof(filesystem_extent_block_t) == FILESYSTEM_BLOCK_SIZE,
    "overflow extent block must occupy exactly one filesystem block"
);

_Static_assert(
    sizeof(filesystem_superblock_t) == 64,
    "filesystem superblock header must remain 64 bytes"
);

_Static_assert(
    sizeof(filesystem_directory_entry_t) == 64,
    "filesystem directory entry must remain 64 bytes"
);

static filesystem_result_t filesystem_convert_storage_result(
    filesystem_storage_result_t result
) {
    if (result == FILESYSTEM_STORAGE_SUCCESS) return FILESYSTEM_SUCCESS;
    if (result == FILESYSTEM_STORAGE_ERROR_INVALID_ARGUMENT) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (result == FILESYSTEM_STORAGE_ERROR_OUT_OF_RANGE) {
        return FILESYSTEM_ERROR_OUT_OF_RANGE;
    }
    return FILESYSTEM_ERROR_STORAGE;
}

static uint8_t bitmap_is_set(const uint8_t *bitmap, uint32_t bit) {
    return bitmap[bit / 8] & (1U << (bit % 8));
}

static void bitmap_set(uint8_t *bitmap, uint32_t bit) {
    bitmap[bit / 8] |= 1U << (bit % 8);
}

static filesystem_result_t filesystem_write_superblock(
    filesystem_t *filesystem
) {
    memory_set(filesystem->block_buffer, 0, FILESYSTEM_BLOCK_SIZE);
    memory_copy(
        filesystem->block_buffer,
        &filesystem->superblock,
        sizeof(filesystem_superblock_t)
    );
    return filesystem_convert_storage_result(filesystem_storage_write_blocks(
        filesystem->storage,
        FILESYSTEM_SUPERBLOCK_BLOCK,
        1,
        filesystem->block_buffer
    ));
}

static filesystem_result_t filesystem_validate_superblock(
    filesystem_t *filesystem
) {
    filesystem_superblock_t *superblock = &filesystem->superblock;
    if (superblock->magic != FILESYSTEM_MAGIC) return FILESYSTEM_ERROR_BAD_MAGIC;
    if (superblock->version != FILESYSTEM_VERSION) {
        return FILESYSTEM_ERROR_UNSUPPORTED_VERSION;
    }
    if (superblock->block_size != FILESYSTEM_BLOCK_SIZE ||
        superblock->total_blocks != filesystem->storage->total_blocks ||
        superblock->block_bitmap_block != FILESYSTEM_BLOCK_BITMAP_BLOCK ||
        superblock->inode_bitmap_block != FILESYSTEM_INODE_BITMAP_BLOCK ||
        superblock->inode_table_start_block != FILESYSTEM_INODE_TABLE_START_BLOCK ||
        superblock->inode_table_block_count != FILESYSTEM_INODE_TABLE_BLOCK_COUNT ||
        superblock->data_start_block != FILESYSTEM_DATA_START_BLOCK ||
        superblock->total_inodes != FILESYSTEM_TOTAL_INODES ||
        superblock->root_inode != FILESYSTEM_ROOT_INODE ||
        superblock->data_start_block >= superblock->total_blocks ||
        superblock->free_blocks > superblock->total_blocks - superblock->data_start_block ||
        superblock->free_inodes >= superblock->total_inodes) {
        return FILESYSTEM_ERROR_CORRUPT;
    }
    return FILESYSTEM_SUCCESS;
}

static filesystem_result_t filesystem_validate_bitmaps(
    filesystem_t *filesystem
) {
    filesystem_result_t result = filesystem_convert_storage_result(
        filesystem_storage_read_blocks(
            filesystem->storage,
            filesystem->superblock.block_bitmap_block,
            1,
            filesystem->block_buffer
        )
    );
    if (result != FILESYSTEM_SUCCESS) return result;

    uint32_t free_blocks = 0;
    for (uint32_t block = 0; block < filesystem->superblock.total_blocks; block++) {
        uint8_t allocated = bitmap_is_set(filesystem->block_buffer, block);
        if (block < filesystem->superblock.data_start_block && !allocated) {
            return FILESYSTEM_ERROR_CORRUPT;
        }
        if (!allocated) free_blocks++;
    }
    if (free_blocks != filesystem->superblock.free_blocks) {
        return FILESYSTEM_ERROR_CORRUPT;
    }

    result = filesystem_convert_storage_result(filesystem_storage_read_blocks(
        filesystem->storage,
        filesystem->superblock.inode_bitmap_block,
        1,
        filesystem->block_buffer
    ));
    if (result != FILESYSTEM_SUCCESS) return result;
    if (!bitmap_is_set(filesystem->block_buffer, FILESYSTEM_ROOT_INODE)) {
        return FILESYSTEM_ERROR_CORRUPT;
    }

    uint32_t free_inodes = 0;
    for (inode_number_t inode = 0;
         inode < filesystem->superblock.total_inodes;
         inode++) {
        if (!bitmap_is_set(filesystem->block_buffer, inode)) free_inodes++;
    }
    if (free_inodes != filesystem->superblock.free_inodes) {
        return FILESYSTEM_ERROR_CORRUPT;
    }
    return FILESYSTEM_SUCCESS;
}

filesystem_result_t filesystem_format(filesystem_storage_t *storage) {
    if (!storage || !storage->initialized) return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    if (storage->total_blocks <= FILESYSTEM_DATA_START_BLOCK ||
        storage->total_blocks > FILESYSTEM_BITS_PER_BITMAP_BLOCK ||
        FILESYSTEM_TOTAL_INODES > FILESYSTEM_BITS_PER_BITMAP_BLOCK) {
        return FILESYSTEM_ERROR_OUT_OF_RANGE;
    }

    filesystem_t filesystem;
    memory_set(&filesystem, 0, sizeof(filesystem));
    filesystem.storage = storage;
    filesystem.mounted = 1;
    filesystem.superblock.magic = FILESYSTEM_MAGIC;
    filesystem.superblock.version = FILESYSTEM_VERSION;
    filesystem.superblock.block_size = FILESYSTEM_BLOCK_SIZE;
    filesystem.superblock.total_blocks = storage->total_blocks;
    filesystem.superblock.block_bitmap_block = FILESYSTEM_BLOCK_BITMAP_BLOCK;
    filesystem.superblock.inode_bitmap_block = FILESYSTEM_INODE_BITMAP_BLOCK;
    filesystem.superblock.inode_table_start_block = FILESYSTEM_INODE_TABLE_START_BLOCK;
    filesystem.superblock.inode_table_block_count = FILESYSTEM_INODE_TABLE_BLOCK_COUNT;
    filesystem.superblock.data_start_block = FILESYSTEM_DATA_START_BLOCK;
    filesystem.superblock.total_inodes = FILESYSTEM_TOTAL_INODES;
    filesystem.superblock.root_inode = FILESYSTEM_ROOT_INODE;
    filesystem.superblock.free_blocks = storage->total_blocks - FILESYSTEM_DATA_START_BLOCK;
    filesystem.superblock.free_inodes = FILESYSTEM_TOTAL_INODES - 1;

    filesystem_result_t result = filesystem_write_superblock(&filesystem);
    if (result != FILESYSTEM_SUCCESS) return result;

    // Reserve every metadata block in the block allocation bitmap.
    memory_set(filesystem.block_buffer, 0, FILESYSTEM_BLOCK_SIZE);
    for (uint32_t block = 0; block < FILESYSTEM_DATA_START_BLOCK; block++) {
        bitmap_set(filesystem.block_buffer, block);
    }
    result = filesystem_convert_storage_result(filesystem_storage_write_blocks(
        storage,
        FILESYSTEM_BLOCK_BITMAP_BLOCK,
        1,
        filesystem.block_buffer
    ));
    if (result != FILESYSTEM_SUCCESS) return result;

    // Reserve inode zero for the root directory.
    memory_set(filesystem.block_buffer, 0, FILESYSTEM_BLOCK_SIZE);
    bitmap_set(filesystem.block_buffer, FILESYSTEM_ROOT_INODE);
    result = filesystem_convert_storage_result(filesystem_storage_write_blocks(
        storage,
        FILESYSTEM_INODE_BITMAP_BLOCK,
        1,
        filesystem.block_buffer
    ));
    if (result != FILESYSTEM_SUCCESS) return result;

    // Clear the complete inode table before creating the root inode.
    memory_set(filesystem.block_buffer, 0, FILESYSTEM_BLOCK_SIZE);
    for (uint32_t block = 0; block < FILESYSTEM_INODE_TABLE_BLOCK_COUNT; block++) {
        result = filesystem_convert_storage_result(filesystem_storage_write_blocks(
            storage,
            FILESYSTEM_INODE_TABLE_START_BLOCK + block,
            1,
            filesystem.block_buffer
        ));
        if (result != FILESYSTEM_SUCCESS) return result;
    }

    filesystem_inode_t root_inode;
    memory_set(&root_inode, 0, sizeof(root_inode));
    root_inode.type = FILESYSTEM_INODE_DIRECTORY;
    return filesystem_write_inode(&filesystem, FILESYSTEM_ROOT_INODE, &root_inode);
}

filesystem_result_t filesystem_mount(
    filesystem_t *filesystem,
    filesystem_storage_t *storage
) {
    if (!filesystem || !storage || !storage->initialized) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }

    memory_set(filesystem, 0, sizeof(*filesystem));
    filesystem->storage = storage;
    filesystem_result_t result = filesystem_convert_storage_result(
        filesystem_storage_read_blocks(
            storage,
            FILESYSTEM_SUPERBLOCK_BLOCK,
            1,
            filesystem->block_buffer
        )
    );
    if (result != FILESYSTEM_SUCCESS) return result;

    memory_copy(
        &filesystem->superblock,
        filesystem->block_buffer,
        sizeof(filesystem_superblock_t)
    );
    result = filesystem_validate_superblock(filesystem);
    if (result != FILESYSTEM_SUCCESS) return result;

    result = filesystem_validate_bitmaps(filesystem);
    if (result != FILESYSTEM_SUCCESS) return result;

    filesystem->mounted = 1;
    filesystem_inode_t root_inode;
    result = filesystem_read_inode(filesystem, FILESYSTEM_ROOT_INODE, &root_inode);
    if (result != FILESYSTEM_SUCCESS) {
        filesystem->mounted = 0;
        return result;
    }
    if (root_inode.type != FILESYSTEM_INODE_DIRECTORY) {
        filesystem->mounted = 0;
        return FILESYSTEM_ERROR_CORRUPT;
    }
    return FILESYSTEM_SUCCESS;
}

filesystem_result_t filesystem_read_inode(
    filesystem_t *filesystem,
    inode_number_t inode_number,
    filesystem_inode_t *inode
) {
    if (!filesystem || !inode) return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    if (!filesystem->mounted) return FILESYSTEM_ERROR_NOT_MOUNTED;
    if (inode_number >= filesystem->superblock.total_inodes) {
        return FILESYSTEM_ERROR_OUT_OF_RANGE;
    }

    uint32_t table_block = inode_number / FILESYSTEM_INODES_PER_BLOCK;
    uint32_t block_offset =
        (inode_number % FILESYSTEM_INODES_PER_BLOCK) * FILESYSTEM_INODE_SIZE;
    filesystem_result_t result = filesystem_convert_storage_result(
        filesystem_storage_read_blocks(
            filesystem->storage,
            filesystem->superblock.inode_table_start_block + table_block,
            1,
            filesystem->block_buffer
        )
    );
    if (result != FILESYSTEM_SUCCESS) return result;
    memory_copy(inode, filesystem->block_buffer + block_offset, sizeof(*inode));
    return FILESYSTEM_SUCCESS;
}

filesystem_result_t filesystem_write_inode(
    filesystem_t *filesystem,
    inode_number_t inode_number,
    const filesystem_inode_t *inode
) {
    if (!filesystem || !inode) return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    if (!filesystem->mounted) return FILESYSTEM_ERROR_NOT_MOUNTED;
    if (inode_number >= filesystem->superblock.total_inodes) {
        return FILESYSTEM_ERROR_OUT_OF_RANGE;
    }
    if ((inode->extent_count > FILESYSTEM_INLINE_EXTENT_COUNT &&
         !inode->first_overflow_extent_block) ||
        (inode->extent_count <= FILESYSTEM_INLINE_EXTENT_COUNT &&
         inode->first_overflow_extent_block)) {
        return FILESYSTEM_ERROR_CORRUPT;
    }

    uint32_t table_block = inode_number / FILESYSTEM_INODES_PER_BLOCK;
    uint32_t block_offset =
        (inode_number % FILESYSTEM_INODES_PER_BLOCK) * FILESYSTEM_INODE_SIZE;
    filesystem_result_t result = filesystem_convert_storage_result(
        filesystem_storage_read_blocks(
            filesystem->storage,
            filesystem->superblock.inode_table_start_block + table_block,
            1,
            filesystem->block_buffer
        )
    );
    if (result != FILESYSTEM_SUCCESS) return result;
    memory_copy(filesystem->block_buffer + block_offset, inode, sizeof(*inode));
    return filesystem_convert_storage_result(filesystem_storage_write_blocks(
        filesystem->storage,
        filesystem->superblock.inode_table_start_block + table_block,
        1,
        filesystem->block_buffer
    ));
}

filesystem_result_t filesystem_allocate_block(
    filesystem_t *filesystem,
    uint32_t *allocated_block
) {
    if (!filesystem || !allocated_block) return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    if (!filesystem->mounted) return FILESYSTEM_ERROR_NOT_MOUNTED;
    if (filesystem->superblock.free_blocks == 0) return FILESYSTEM_ERROR_NO_SPACE;

    filesystem_result_t result = filesystem_convert_storage_result(
        filesystem_storage_read_blocks(
            filesystem->storage,
            filesystem->superblock.block_bitmap_block,
            1,
            filesystem->block_buffer
        )
    );
    if (result != FILESYSTEM_SUCCESS) return result;

    for (uint32_t block = filesystem->superblock.data_start_block;
         block < filesystem->superblock.total_blocks;
         block++) {
        if (bitmap_is_set(filesystem->block_buffer, block)) continue;
        bitmap_set(filesystem->block_buffer, block);
        result = filesystem_convert_storage_result(filesystem_storage_write_blocks(
            filesystem->storage,
            filesystem->superblock.block_bitmap_block,
            1,
            filesystem->block_buffer
        ));
        if (result != FILESYSTEM_SUCCESS) return result;

        memory_set(filesystem->block_buffer, 0, FILESYSTEM_BLOCK_SIZE);
        result = filesystem_convert_storage_result(filesystem_storage_write_blocks(
            filesystem->storage,
            block,
            1,
            filesystem->block_buffer
        ));
        if (result != FILESYSTEM_SUCCESS) return result;

        filesystem->superblock.free_blocks--;
        result = filesystem_write_superblock(filesystem);
        if (result != FILESYSTEM_SUCCESS) return result;
        *allocated_block = block;
        return FILESYSTEM_SUCCESS;
    }
    return FILESYSTEM_ERROR_CORRUPT;
}

filesystem_result_t filesystem_allocate_inode(
    filesystem_t *filesystem,
    filesystem_inode_type_t type,
    inode_number_t *allocated_inode
) {
    if (!filesystem || !allocated_inode ||
        (type != FILESYSTEM_INODE_REGULAR_FILE &&
         type != FILESYSTEM_INODE_DIRECTORY)) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (!filesystem->mounted) return FILESYSTEM_ERROR_NOT_MOUNTED;
    if (filesystem->superblock.free_inodes == 0) return FILESYSTEM_ERROR_NO_SPACE;

    filesystem_result_t result = filesystem_convert_storage_result(
        filesystem_storage_read_blocks(
            filesystem->storage,
            filesystem->superblock.inode_bitmap_block,
            1,
            filesystem->block_buffer
        )
    );
    if (result != FILESYSTEM_SUCCESS) return result;

    for (inode_number_t inode_number = 1;
         inode_number < filesystem->superblock.total_inodes;
         inode_number++) {
        if (bitmap_is_set(filesystem->block_buffer, inode_number)) continue;
        bitmap_set(filesystem->block_buffer, inode_number);
        result = filesystem_convert_storage_result(filesystem_storage_write_blocks(
            filesystem->storage,
            filesystem->superblock.inode_bitmap_block,
            1,
            filesystem->block_buffer
        ));
        if (result != FILESYSTEM_SUCCESS) return result;

        filesystem_inode_t inode;
        memory_set(&inode, 0, sizeof(inode));
        inode.type = type;
        result = filesystem_write_inode(filesystem, inode_number, &inode);
        if (result != FILESYSTEM_SUCCESS) return result;

        filesystem->superblock.free_inodes--;
        result = filesystem_write_superblock(filesystem);
        if (result != FILESYSTEM_SUCCESS) return result;
        *allocated_inode = inode_number;
        return FILESYSTEM_SUCCESS;
    }
    return FILESYSTEM_ERROR_CORRUPT;
}

static filesystem_result_t filesystem_read_extent_block(
    filesystem_t *filesystem,
    uint32_t block_number,
    filesystem_extent_block_t *extent_block
) {
    if (block_number < filesystem->superblock.data_start_block ||
        block_number >= filesystem->superblock.total_blocks) {
        return FILESYSTEM_ERROR_CORRUPT;
    }
    filesystem_result_t result = filesystem_convert_storage_result(
        filesystem_storage_read_blocks(
            filesystem->storage,
            block_number,
            1,
            filesystem->block_buffer
        )
    );
    if (result != FILESYSTEM_SUCCESS) return result;
    memory_copy(extent_block, filesystem->block_buffer, sizeof(*extent_block));
    if (extent_block->extent_count > FILESYSTEM_EXTENTS_PER_OVERFLOW_BLOCK) {
        return FILESYSTEM_ERROR_CORRUPT;
    }
    return FILESYSTEM_SUCCESS;
}

static filesystem_result_t filesystem_write_extent_block(
    filesystem_t *filesystem,
    uint32_t block_number,
    const filesystem_extent_block_t *extent_block
) {
    memory_copy(filesystem->block_buffer, extent_block, sizeof(*extent_block));
    return filesystem_convert_storage_result(filesystem_storage_write_blocks(
        filesystem->storage,
        block_number,
        1,
        filesystem->block_buffer
    ));
}

static filesystem_result_t filesystem_get_extent(
    filesystem_t *filesystem,
    const filesystem_inode_t *inode,
    uint32_t extent_index,
    filesystem_extent_t *extent
) {
    if (extent_index >= inode->extent_count) return FILESYSTEM_ERROR_OUT_OF_RANGE;
    if (extent_index < FILESYSTEM_INLINE_EXTENT_COUNT) {
        *extent = inode->inline_extents[extent_index];
        return FILESYSTEM_SUCCESS;
    }

    uint32_t remaining_index = extent_index - FILESYSTEM_INLINE_EXTENT_COUNT;
    uint32_t extent_block_number = inode->first_overflow_extent_block;
    uint32_t remaining_extents = inode->extent_count - FILESYSTEM_INLINE_EXTENT_COUNT;
    while (extent_block_number && remaining_extents) {
        filesystem_extent_block_t extent_block;
        filesystem_result_t result = filesystem_read_extent_block(
            filesystem,
            extent_block_number,
            &extent_block
        );
        if (result != FILESYSTEM_SUCCESS) return result;
        if (extent_block.extent_count == 0 ||
            extent_block.extent_count > remaining_extents) {
            return FILESYSTEM_ERROR_CORRUPT;
        }
        if (remaining_index < extent_block.extent_count) {
            *extent = extent_block.extents[remaining_index];
            return FILESYSTEM_SUCCESS;
        }
        remaining_index -= extent_block.extent_count;
        remaining_extents -= extent_block.extent_count;
        extent_block_number = extent_block.next_extent_block;
    }
    return FILESYSTEM_ERROR_CORRUPT;
}

static filesystem_result_t filesystem_update_extent(
    filesystem_t *filesystem,
    filesystem_inode_t *inode,
    uint32_t extent_index,
    const filesystem_extent_t *extent
) {
    if (extent_index >= inode->extent_count) return FILESYSTEM_ERROR_OUT_OF_RANGE;
    if (extent_index < FILESYSTEM_INLINE_EXTENT_COUNT) {
        inode->inline_extents[extent_index] = *extent;
        return FILESYSTEM_SUCCESS;
    }

    uint32_t remaining_index = extent_index - FILESYSTEM_INLINE_EXTENT_COUNT;
    uint32_t extent_block_number = inode->first_overflow_extent_block;
    while (extent_block_number) {
        filesystem_extent_block_t extent_block;
        filesystem_result_t result = filesystem_read_extent_block(
            filesystem,
            extent_block_number,
            &extent_block
        );
        if (result != FILESYSTEM_SUCCESS) return result;
        if (remaining_index < extent_block.extent_count) {
            extent_block.extents[remaining_index] = *extent;
            return filesystem_write_extent_block(
                filesystem,
                extent_block_number,
                &extent_block
            );
        }
        remaining_index -= extent_block.extent_count;
        extent_block_number = extent_block.next_extent_block;
    }
    return FILESYSTEM_ERROR_CORRUPT;
}

static filesystem_result_t filesystem_get_allocated_block_count(
    filesystem_t *filesystem,
    const filesystem_inode_t *inode,
    uint32_t *allocated_blocks
) {
    uint32_t expected_logical_block = 0;
    for (uint32_t extent_index = 0;
         extent_index < inode->extent_count;
         extent_index++) {
        filesystem_extent_t extent;
        filesystem_result_t result = filesystem_get_extent(
            filesystem,
            inode,
            extent_index,
            &extent
        );
        if (result != FILESYSTEM_SUCCESS) return result;
        if (extent.block_count == 0 ||
            extent.logical_start_block != expected_logical_block ||
            extent.disk_start_block < filesystem->superblock.data_start_block ||
            extent.disk_start_block >= filesystem->superblock.total_blocks ||
            extent.block_count >
                filesystem->superblock.total_blocks - extent.disk_start_block) {
            return FILESYSTEM_ERROR_CORRUPT;
        }
        expected_logical_block += extent.block_count;
    }
    *allocated_blocks = expected_logical_block;
    return FILESYSTEM_SUCCESS;
}

static filesystem_result_t filesystem_append_new_extent(
    filesystem_t *filesystem,
    filesystem_inode_t *inode,
    const filesystem_extent_t *new_extent
) {
    if (inode->extent_count < FILESYSTEM_INLINE_EXTENT_COUNT) {
        inode->inline_extents[inode->extent_count] = *new_extent;
        inode->extent_count++;
        return FILESYSTEM_SUCCESS;
    }

    if (!inode->first_overflow_extent_block) {
        filesystem_result_t result = filesystem_allocate_block(
            filesystem,
            &inode->first_overflow_extent_block
        );
        if (result != FILESYSTEM_SUCCESS) return result;
        filesystem_extent_block_t first_block;
        memory_set(&first_block, 0, sizeof(first_block));
        result = filesystem_write_extent_block(
            filesystem,
            inode->first_overflow_extent_block,
            &first_block
        );
        if (result != FILESYSTEM_SUCCESS) return result;
    }

    uint32_t extent_block_number = inode->first_overflow_extent_block;
    while (1) {
        filesystem_extent_block_t extent_block;
        filesystem_result_t result = filesystem_read_extent_block(
            filesystem,
            extent_block_number,
            &extent_block
        );
        if (result != FILESYSTEM_SUCCESS) return result;

        if (extent_block.extent_count < FILESYSTEM_EXTENTS_PER_OVERFLOW_BLOCK) {
            extent_block.extents[extent_block.extent_count] = *new_extent;
            extent_block.extent_count++;
            result = filesystem_write_extent_block(
                filesystem,
                extent_block_number,
                &extent_block
            );
            if (result != FILESYSTEM_SUCCESS) return result;
            inode->extent_count++;
            return FILESYSTEM_SUCCESS;
        }

        if (!extent_block.next_extent_block) {
            uint32_t next_extent_block;
            result = filesystem_allocate_block(filesystem, &next_extent_block);
            if (result != FILESYSTEM_SUCCESS) return result;
            extent_block.next_extent_block = next_extent_block;
            result = filesystem_write_extent_block(
                filesystem,
                extent_block_number,
                &extent_block
            );
            if (result != FILESYSTEM_SUCCESS) return result;

            filesystem_extent_block_t next_block;
            memory_set(&next_block, 0, sizeof(next_block));
            result = filesystem_write_extent_block(
                filesystem,
                next_extent_block,
                &next_block
            );
            if (result != FILESYSTEM_SUCCESS) return result;
            extent_block_number = next_extent_block;
        } else {
            extent_block_number = extent_block.next_extent_block;
        }
    }
}

static filesystem_result_t filesystem_attach_allocated_block(
    filesystem_t *filesystem,
    filesystem_inode_t *inode,
    uint32_t disk_block
) {
    uint32_t logical_block;
    filesystem_result_t result = filesystem_get_allocated_block_count(
        filesystem,
        inode,
        &logical_block
    );
    if (result != FILESYSTEM_SUCCESS) return result;

    if (inode->extent_count) {
        filesystem_extent_t last_extent;
        result = filesystem_get_extent(
            filesystem,
            inode,
            inode->extent_count - 1,
            &last_extent
        );
        if (result != FILESYSTEM_SUCCESS) return result;
        if (last_extent.disk_start_block + last_extent.block_count == disk_block) {
            last_extent.block_count++;
            return filesystem_update_extent(
                filesystem,
                inode,
                inode->extent_count - 1,
                &last_extent
            );
        }
    }

    filesystem_extent_t new_extent = {
        .logical_start_block = logical_block,
        .disk_start_block = disk_block,
        .block_count = 1
    };
    return filesystem_append_new_extent(filesystem, inode, &new_extent);
}

static filesystem_result_t filesystem_find_disk_block(
    filesystem_t *filesystem,
    const filesystem_inode_t *inode,
    uint32_t logical_block,
    uint32_t *disk_block
) {
    for (uint32_t extent_index = 0;
         extent_index < inode->extent_count;
         extent_index++) {
        filesystem_extent_t extent;
        filesystem_result_t result = filesystem_get_extent(
            filesystem,
            inode,
            extent_index,
            &extent
        );
        if (result != FILESYSTEM_SUCCESS) return result;
        if (logical_block >= extent.logical_start_block &&
            logical_block - extent.logical_start_block < extent.block_count) {
            *disk_block = extent.disk_start_block +
                (logical_block - extent.logical_start_block);
            return FILESYSTEM_SUCCESS;
        }
    }
    return FILESYSTEM_ERROR_CORRUPT;
}

filesystem_result_t filesystem_read_inode_data(
    filesystem_t *filesystem,
    inode_number_t inode_number,
    uint32_t byte_offset,
    uint32_t byte_count,
    void *destination,
    uint32_t *bytes_read
) {
    if (!filesystem || !destination || !bytes_read) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }
    *bytes_read = 0;
    filesystem_inode_t inode;
    filesystem_result_t result = filesystem_read_inode(
        filesystem,
        inode_number,
        &inode
    );
    if (result != FILESYSTEM_SUCCESS) return result;
    if (inode.type == FILESYSTEM_INODE_UNUSED || byte_offset > inode.size_bytes) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }

    uint32_t available_bytes = inode.size_bytes - byte_offset;
    if (byte_count > available_bytes) byte_count = available_bytes;
    uint8_t *output = destination;
    while (*bytes_read < byte_count) {
        uint32_t current_offset = byte_offset + *bytes_read;
        uint32_t logical_block = current_offset / FILESYSTEM_BLOCK_SIZE;
        uint32_t offset_in_block = current_offset % FILESYSTEM_BLOCK_SIZE;
        uint32_t chunk_size = FILESYSTEM_BLOCK_SIZE - offset_in_block;
        if (chunk_size > byte_count - *bytes_read) {
            chunk_size = byte_count - *bytes_read;
        }

        uint32_t disk_block;
        result = filesystem_find_disk_block(
            filesystem,
            &inode,
            logical_block,
            &disk_block
        );
        if (result != FILESYSTEM_SUCCESS) return result;

        if (offset_in_block == 0 && chunk_size == FILESYSTEM_BLOCK_SIZE) {
            result = filesystem_convert_storage_result(
                filesystem_storage_read_blocks(
                    filesystem->storage,
                    disk_block,
                    1,
                    output + *bytes_read
                )
            );
        } else {
            result = filesystem_convert_storage_result(
                filesystem_storage_read_blocks(
                    filesystem->storage,
                    disk_block,
                    1,
                    filesystem->block_buffer
                )
            );
            if (result == FILESYSTEM_SUCCESS) {
                memory_copy(
                    output + *bytes_read,
                    filesystem->block_buffer + offset_in_block,
                    chunk_size
                );
            }
        }
        if (result != FILESYSTEM_SUCCESS) return result;
        *bytes_read += chunk_size;
    }
    return FILESYSTEM_SUCCESS;
}

filesystem_result_t filesystem_write_inode_data(
    filesystem_t *filesystem,
    inode_number_t inode_number,
    uint32_t byte_offset,
    uint32_t byte_count,
    const void *source,
    uint32_t *bytes_written
) {
    if (!filesystem || !source || !bytes_written || byte_count == 0) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }
    *bytes_written = 0;
    filesystem_inode_t inode;
    filesystem_result_t result = filesystem_read_inode(
        filesystem,
        inode_number,
        &inode
    );
    if (result != FILESYSTEM_SUCCESS) return result;
    if (inode.type == FILESYSTEM_INODE_UNUSED || byte_offset > inode.size_bytes ||
        byte_count > FILESYSTEM_MAX_FILE_SIZE - byte_offset) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }

    uint32_t end_offset = byte_offset + byte_count;
    uint32_t required_blocks = end_offset / FILESYSTEM_BLOCK_SIZE;
    if (end_offset % FILESYSTEM_BLOCK_SIZE) required_blocks++;

    uint32_t allocated_blocks;
    result = filesystem_get_allocated_block_count(
        filesystem,
        &inode,
        &allocated_blocks
    );
    if (result != FILESYSTEM_SUCCESS) return result;

    while (allocated_blocks < required_blocks) {
        uint32_t disk_block;
        result = filesystem_allocate_block(filesystem, &disk_block);
        if (result != FILESYSTEM_SUCCESS) return result;
        result = filesystem_attach_allocated_block(filesystem, &inode, disk_block);
        if (result != FILESYSTEM_SUCCESS) return result;
        allocated_blocks++;
    }

    // Persist block ownership before writing file contents into those blocks.
    result = filesystem_write_inode(filesystem, inode_number, &inode);
    if (result != FILESYSTEM_SUCCESS) return result;

    const uint8_t *input = source;
    while (*bytes_written < byte_count) {
        uint32_t current_offset = byte_offset + *bytes_written;
        uint32_t logical_block = current_offset / FILESYSTEM_BLOCK_SIZE;
        uint32_t offset_in_block = current_offset % FILESYSTEM_BLOCK_SIZE;
        uint32_t chunk_size = FILESYSTEM_BLOCK_SIZE - offset_in_block;
        if (chunk_size > byte_count - *bytes_written) {
            chunk_size = byte_count - *bytes_written;
        }

        uint32_t disk_block;
        result = filesystem_find_disk_block(
            filesystem,
            &inode,
            logical_block,
            &disk_block
        );
        if (result != FILESYSTEM_SUCCESS) return result;

        if (offset_in_block == 0 && chunk_size == FILESYSTEM_BLOCK_SIZE) {
            result = filesystem_convert_storage_result(
                filesystem_storage_write_blocks(
                    filesystem->storage,
                    disk_block,
                    1,
                    input + *bytes_written
                )
            );
        } else {
            result = filesystem_convert_storage_result(
                filesystem_storage_read_blocks(
                    filesystem->storage,
                    disk_block,
                    1,
                    filesystem->block_buffer
                )
            );
            if (result == FILESYSTEM_SUCCESS) {
                memory_copy(
                    filesystem->block_buffer + offset_in_block,
                    input + *bytes_written,
                    chunk_size
                );
                result = filesystem_convert_storage_result(
                    filesystem_storage_write_blocks(
                        filesystem->storage,
                        disk_block,
                        1,
                        filesystem->block_buffer
                    )
                );
            }
        }
        if (result != FILESYSTEM_SUCCESS) return result;
        *bytes_written += chunk_size;
    }

    if (end_offset > inode.size_bytes) inode.size_bytes = end_offset;
    return filesystem_write_inode(filesystem, inode_number, &inode);
}

static uint8_t filesystem_names_equal(
    const filesystem_directory_entry_t *entry,
    const char *name,
    uint32_t name_length
) {
    if (entry->name_length != name_length) return 0;
    for (uint32_t index = 0; index < name_length; index++) {
        if (entry->name[index] != name[index]) return 0;
    }
    return 1;
}

static filesystem_result_t filesystem_find_root_entry(
    filesystem_t *filesystem,
    const char *name,
    uint32_t name_length,
    filesystem_directory_entry_t *found_entry
) {
    filesystem_inode_t root_inode;
    filesystem_result_t result = filesystem_read_inode(
        filesystem,
        FILESYSTEM_ROOT_INODE,
        &root_inode
    );
    if (result != FILESYSTEM_SUCCESS) return result;
    if (root_inode.type != FILESYSTEM_INODE_DIRECTORY ||
        root_inode.size_bytes % sizeof(filesystem_directory_entry_t)) {
        return FILESYSTEM_ERROR_CORRUPT;
    }

    for (uint32_t offset = 0;
         offset < root_inode.size_bytes;
         offset += sizeof(filesystem_directory_entry_t)) {
        filesystem_directory_entry_t entry;
        uint32_t bytes_read;
        result = filesystem_read_inode_data(
            filesystem,
            FILESYSTEM_ROOT_INODE,
            offset,
            sizeof(entry),
            &entry,
            &bytes_read
        );
        if (result != FILESYSTEM_SUCCESS) return result;
        if (bytes_read != sizeof(entry) ||
            entry.name_length == 0 ||
            entry.name_length > FILESYSTEM_MAX_FILE_NAME_LENGTH ||
            entry.inode_number >= filesystem->superblock.total_inodes) {
            return FILESYSTEM_ERROR_CORRUPT;
        }
        if (filesystem_names_equal(&entry, name, name_length)) {
            if (found_entry) *found_entry = entry;
            return FILESYSTEM_SUCCESS;
        }
    }
    return FILESYSTEM_ERROR_NOT_FOUND;
}

filesystem_result_t filesystem_create_file(
    filesystem_t *filesystem,
    const char *name,
    uint32_t name_length,
    filesystem_file_t *file
) {
    if (!filesystem || !name || !file || name_length == 0 ||
        name_length > FILESYSTEM_MAX_FILE_NAME_LENGTH) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }

    filesystem_result_t result = filesystem_find_root_entry(
        filesystem,
        name,
        name_length,
        0
    );
    if (result == FILESYSTEM_SUCCESS) return FILESYSTEM_ERROR_ALREADY_EXISTS;
    if (result != FILESYSTEM_ERROR_NOT_FOUND) return result;

    inode_number_t inode_number;
    result = filesystem_allocate_inode(
        filesystem,
        FILESYSTEM_INODE_REGULAR_FILE,
        &inode_number
    );
    if (result != FILESYSTEM_SUCCESS) return result;

    filesystem_inode_t root_inode;
    result = filesystem_read_inode(filesystem, FILESYSTEM_ROOT_INODE, &root_inode);
    if (result != FILESYSTEM_SUCCESS) return result;

    filesystem_directory_entry_t entry;
    memory_set(&entry, 0, sizeof(entry));
    entry.inode_number = inode_number;
    entry.name_length = name_length;
    memory_copy(entry.name, name, name_length);

    uint32_t bytes_written;
    result = filesystem_write_inode_data(
        filesystem,
        FILESYSTEM_ROOT_INODE,
        root_inode.size_bytes,
        sizeof(entry),
        &entry,
        &bytes_written
    );
    if (result != FILESYSTEM_SUCCESS) return result;
    if (bytes_written != sizeof(entry)) return FILESYSTEM_ERROR_STORAGE;

    file->filesystem = filesystem;
    file->inode_number = inode_number;
    file->position = 0;
    return FILESYSTEM_SUCCESS;
}

filesystem_result_t filesystem_open_file(
    filesystem_t *filesystem,
    const char *name,
    uint32_t name_length,
    filesystem_file_t *file
) {
    if (!filesystem || !name || !file || name_length == 0 ||
        name_length > FILESYSTEM_MAX_FILE_NAME_LENGTH) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }

    filesystem_directory_entry_t entry;
    filesystem_result_t result = filesystem_find_root_entry(
        filesystem,
        name,
        name_length,
        &entry
    );
    if (result != FILESYSTEM_SUCCESS) return result;

    filesystem_inode_t inode;
    result = filesystem_read_inode(filesystem, entry.inode_number, &inode);
    if (result != FILESYSTEM_SUCCESS) return result;
    if (inode.type != FILESYSTEM_INODE_REGULAR_FILE) {
        return FILESYSTEM_ERROR_CORRUPT;
    }

    file->filesystem = filesystem;
    file->inode_number = entry.inode_number;
    file->position = 0;
    return FILESYSTEM_SUCCESS;
}

filesystem_result_t filesystem_read_file(
    filesystem_file_t *file,
    void *destination,
    uint32_t byte_count,
    uint32_t *bytes_read
) {
    if (!file || !file->filesystem) return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    filesystem_result_t result = filesystem_read_inode_data(
        file->filesystem,
        file->inode_number,
        file->position,
        byte_count,
        destination,
        bytes_read
    );
    if (result == FILESYSTEM_SUCCESS) file->position += *bytes_read;
    return result;
}

filesystem_result_t filesystem_write_file(
    filesystem_file_t *file,
    const void *source,
    uint32_t byte_count,
    uint32_t *bytes_written
) {
    if (!file || !file->filesystem) return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    filesystem_result_t result = filesystem_write_inode_data(
        file->filesystem,
        file->inode_number,
        file->position,
        byte_count,
        source,
        bytes_written
    );
    if (result == FILESYSTEM_SUCCESS) file->position += *bytes_written;
    return result;
}

filesystem_result_t filesystem_seek_file(
    filesystem_file_t *file,
    uint32_t position
) {
    if (!file || !file->filesystem) return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    filesystem_inode_t inode;
    filesystem_result_t result = filesystem_read_inode(
        file->filesystem,
        file->inode_number,
        &inode
    );
    if (result != FILESYSTEM_SUCCESS) return result;
    if (inode.type != FILESYSTEM_INODE_REGULAR_FILE || position > inode.size_bytes) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }
    file->position = position;
    return FILESYSTEM_SUCCESS;
}

filesystem_result_t filesystem_close_file(filesystem_file_t *file) {
    if (!file || !file->filesystem) return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    file->filesystem = 0;
    file->inode_number = 0;
    file->position = 0;
    return FILESYSTEM_SUCCESS;
}
