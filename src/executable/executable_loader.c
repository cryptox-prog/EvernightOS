#include "executable/executable_format.h"
#include "executable/executable_loader.h"
#include "utils/memory.h"

extern uint32_t executable_call_entry(
    uint32_t entry_address,
    uint32_t stack_top,
    const program_services_t *services,
    const program_startup_t *startup
);

static uint32_t align_up(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

static executable_result_t executable_validate_header(
    const executable_header_t *header,
    uint32_t expected_load_address,
    uint32_t *stack_top
) {
    if (header->magic != EXECUTABLE_MAGIC) return EXECUTABLE_ERROR_BAD_MAGIC;
    if (header->format_version != EXECUTABLE_FORMAT_VERSION) {
        return EXECUTABLE_ERROR_UNSUPPORTED_VERSION;
    }
    if (header->header_size != sizeof(executable_header_t) ||
        header->image_size == 0 ||
        header->memory_size < header->image_size ||
        header->entry_offset >= header->image_size ||
        header->stack_size == 0 || header->stack_size % 16) {
        return EXECUTABLE_ERROR_INVALID_HEADER;
    }
    if (header->load_address != expected_load_address ||
        header->memory_size > 0xffffffffU - header->load_address) {
        return EXECUTABLE_ERROR_INVALID_MEMORY_LAYOUT;
    }

    uint32_t region_end;
    if (expected_load_address == SHELL_LOAD_ADDRESS) {
        region_end = SHELL_REGION_END;
    } else if (expected_load_address == CHILD_PROGRAM_LOAD_ADDRESS) {
        region_end = CHILD_PROGRAM_REGION_END;
    } else {
        return EXECUTABLE_ERROR_INVALID_MEMORY_LAYOUT;
    }

    uint32_t memory_end = header->load_address + header->memory_size;
    if (memory_end > 0xffffffffU - 15) {
        return EXECUTABLE_ERROR_INVALID_MEMORY_LAYOUT;
    }
    uint32_t stack_bottom = align_up(memory_end, 16);
    if (stack_bottom >= region_end) return EXECUTABLE_ERROR_INVALID_MEMORY_LAYOUT;
    if (header->stack_size > region_end - stack_bottom) {
        return EXECUTABLE_ERROR_INVALID_MEMORY_LAYOUT;
    }
    *stack_top = stack_bottom + header->stack_size;
    return EXECUTABLE_SUCCESS;
}

executable_result_t executable_run(
    filesystem_t *filesystem,
    const char *name,
    uint32_t name_length,
    uint32_t expected_load_address,
    const program_services_t *services,
    const program_startup_t *startup,
    uint32_t *exit_code
) {
    if (!filesystem || !name || name_length == 0 || !services || !startup ||
        !exit_code) {
        return EXECUTABLE_ERROR_INVALID_ARGUMENT;
    }

    filesystem_file_t file;
    if (filesystem_open_file(filesystem, name, name_length, &file) !=
        FILESYSTEM_SUCCESS) {
        return EXECUTABLE_ERROR_FILE;
    }

    executable_header_t header;
    uint32_t bytes_read;
    if (filesystem_read_file(
            &file,
            &header,
            sizeof(header),
            &bytes_read
        ) != FILESYSTEM_SUCCESS || bytes_read != sizeof(header)) {
        filesystem_close_file(&file);
        return EXECUTABLE_ERROR_FILE;
    }

    uint32_t stack_top;
    executable_result_t result = executable_validate_header(
        &header,
        expected_load_address,
        &stack_top
    );
    if (result != EXECUTABLE_SUCCESS) {
        filesystem_close_file(&file);
        return result;
    }

    void *program_memory = (void *)header.load_address;
    if (filesystem_read_file(
            &file,
            program_memory,
            header.image_size,
            &bytes_read
        ) != FILESYSTEM_SUCCESS || bytes_read != header.image_size) {
        filesystem_close_file(&file);
        return EXECUTABLE_ERROR_FILE;
    }

    uint8_t extra_byte;
    if (filesystem_read_file(&file, &extra_byte, 1, &bytes_read) !=
        FILESYSTEM_SUCCESS || bytes_read != 0) {
        filesystem_close_file(&file);
        return EXECUTABLE_ERROR_INVALID_HEADER;
    }
    filesystem_close_file(&file);

    memory_set(
        (uint8_t *)program_memory + header.image_size,
        0,
        header.memory_size - header.image_size
    );
    uint32_t entry_address = header.load_address + header.entry_offset;
    *exit_code = executable_call_entry(entry_address, stack_top, services, startup);
    return EXECUTABLE_SUCCESS;
}
