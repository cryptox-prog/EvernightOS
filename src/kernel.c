#include "descriptor_tables/idt.h"
#include "display/vga.h"
#include "display/welcome_screen.h"
#include "drivers/keyboard.h"
#include "drivers/ata.h"
#include "storage/filesystem_storage.h"
#include "filesystem/filesystem.h"
#include "executable/executable_format.h"
#include "executable/executable_loader.h"
#include "executable/program_services.h"
#include "utils/memory.h"
#include "utils/ports.h"

#define QEMU_ACPI_SHUTDOWN_PORT 0x0604
#define QEMU_ACPI_SHUTDOWN_VALUE 0x2000

static uint32_t cursor;

ata_result_t kernel_ata_status;
filesystem_storage_result_t kernel_filesystem_storage_status;
filesystem_storage_t kernel_filesystem_storage;
filesystem_result_t kernel_filesystem_status;
filesystem_t kernel_filesystem;
executable_result_t kernel_shell_status;
uint32_t kernel_shell_exit_code;

_Static_assert(
    sizeof(program_file_t) == sizeof(filesystem_file_t),
    "program file handle must fit a filesystem file handle"
);

static void put_cell(uint32_t position, char c) {
    VGA_MEMORY[position * 2] = c;
    VGA_MEMORY[position * 2 + 1] = vga_color_encode(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

static void scroll_if_needed(void) {
    if (cursor < VGA_WIDTH * VGA_HEIGHT) return;
    for (uint32_t i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1) * 2; i++) {
        VGA_MEMORY[i] = VGA_MEMORY[i + VGA_WIDTH * 2];
    }
    for (uint32_t i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
        put_cell(i, ' ');
    }
    cursor -= VGA_WIDTH;
}

static void terminal_clear(void) {
    clear_screen();
    cursor = 0;
    set_cursor_position(0, 0);
}

static void terminal_write_character(char character) {
    if (character == '\b') {
        if (cursor) put_cell(--cursor, ' ');
    } else if (character == '\n') {
        cursor = (cursor / VGA_WIDTH + 1) * VGA_WIDTH;
    } else if (character >= ' ' && character <= '~') {
        put_cell(cursor++, character);
    }
    scroll_if_needed();
    set_cursor_position(cursor / VGA_WIDTH, cursor % VGA_WIDTH);
}

static void terminal_recolor_previous(
    uint32_t character_count,
    uint8_t foreground,
    uint8_t background
) {
    if (character_count > cursor) character_count = cursor;
    uint8_t color = vga_color_encode(
        (vga_color_t)foreground,
        (vga_color_t)background
    );
    for (uint32_t position = cursor - character_count;
         position < cursor;
         position++) {
        VGA_MEMORY[position * 2 + 1] = color;
    }
}

static uint32_t service_file_create(
    const char *name,
    uint32_t name_length,
    program_file_t *file
) {
    filesystem_file_t native_file;
    filesystem_result_t result = filesystem_create_file(
        &kernel_filesystem,
        name,
        name_length,
        &native_file
    );
    if (result == FILESYSTEM_SUCCESS) {
        memory_copy(file, &native_file, sizeof(native_file));
    }
    return result;
}

static uint32_t service_file_open(
    const char *name,
    uint32_t name_length,
    program_file_t *file
) {
    filesystem_file_t native_file;
    filesystem_result_t result = filesystem_open_file(
        &kernel_filesystem,
        name,
        name_length,
        &native_file
    );
    if (result == FILESYSTEM_SUCCESS) {
        memory_copy(file, &native_file, sizeof(native_file));
    }
    return result;
}

static uint32_t service_file_read(
    program_file_t *file,
    void *destination,
    uint32_t byte_count,
    uint32_t *bytes_read
) {
    filesystem_file_t native_file;
    memory_copy(&native_file, file, sizeof(native_file));
    filesystem_result_t result = filesystem_read_file(
        &native_file,
        destination,
        byte_count,
        bytes_read
    );
    memory_copy(file, &native_file, sizeof(native_file));
    return result;
}

static uint32_t service_file_write(
    program_file_t *file,
    const void *source,
    uint32_t byte_count,
    uint32_t *bytes_written
) {
    filesystem_file_t native_file;
    memory_copy(&native_file, file, sizeof(native_file));
    filesystem_result_t result = filesystem_write_file(
        &native_file,
        source,
        byte_count,
        bytes_written
    );
    memory_copy(file, &native_file, sizeof(native_file));
    return result;
}

static uint32_t service_file_close(program_file_t *file) {
    filesystem_file_t native_file;
    memory_copy(&native_file, file, sizeof(native_file));
    filesystem_result_t result = filesystem_close_file(&native_file);
    memory_copy(file, &native_file, sizeof(native_file));
    return result;
}

static uint32_t service_output_write(
    program_output_t *output,
    const void *source,
    uint32_t byte_count,
    uint32_t *bytes_written
) {
    if (!output || !source || !bytes_written) return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    if (output->type == PROGRAM_OUTPUT_FILE) {
        return service_file_write(&output->file, source, byte_count, bytes_written);
    }
    if (output->type != PROGRAM_OUTPUT_TERMINAL) {
        return FILESYSTEM_ERROR_INVALID_ARGUMENT;
    }

    const char *characters = source;
    for (*bytes_written = 0; *bytes_written < byte_count; (*bytes_written)++) {
        terminal_write_character(characters[*bytes_written]);
    }
    return FILESYSTEM_SUCCESS;
}

static const program_services_t program_services;

static uint32_t service_execute(
    const char *name,
    uint32_t name_length,
    const program_startup_t *startup,
    uint32_t *exit_code
) {
    return executable_run(
        &kernel_filesystem,
        name,
        name_length,
        CHILD_PROGRAM_LOAD_ADDRESS,
        &program_services,
        startup,
        exit_code
    );
}

static void service_shutdown(void) {
    __asm__ volatile ("cli");
    write_word_to_port(QEMU_ACPI_SHUTDOWN_PORT, QEMU_ACPI_SHUTDOWN_VALUE);
    for (;;) __asm__ volatile ("hlt");
}

static const program_services_t program_services = {
    .abi_version = PROGRAM_SERVICES_ABI_VERSION,
    .terminal_clear = terminal_clear,
    .terminal_write_character = terminal_write_character,
    .keyboard_read_character = keyboard_get_char,
    .file_create = service_file_create,
    .file_open = service_file_open,
    .file_read = service_file_read,
    .file_write = service_file_write,
    .file_close = service_file_close,
    .output_write = service_output_write,
    .execute = service_execute,
    .shutdown = service_shutdown,
    .terminal_recolor_previous = terminal_recolor_previous
};

void kernel_main(void) {
    __asm__ volatile ("cli");
    init_idt();
    init_keyboard();
    kernel_ata_status = ata_initialize();
    if (kernel_ata_status == ATA_SUCCESS) {
        kernel_filesystem_storage_status = filesystem_storage_initialize(
            &kernel_filesystem_storage,
            ata_get_block_device()
        );
    } else {
        kernel_filesystem_storage_status = FILESYSTEM_STORAGE_ERROR_NOT_READY;
    }
    if (kernel_filesystem_storage_status == FILESYSTEM_STORAGE_SUCCESS) {
        kernel_filesystem_status = filesystem_mount(
            &kernel_filesystem,
            &kernel_filesystem_storage
        );
        if (kernel_filesystem_status == FILESYSTEM_ERROR_BAD_MAGIC) {
            kernel_filesystem_status = filesystem_format(&kernel_filesystem_storage);
            if (kernel_filesystem_status == FILESYSTEM_SUCCESS) {
                kernel_filesystem_status = filesystem_mount(
                    &kernel_filesystem,
                    &kernel_filesystem_storage
                );
            }
        }
    } else {
        kernel_filesystem_status = FILESYSTEM_ERROR_STORAGE;
    }
    __asm__ volatile ("sti");
    display_welcome_screen();
    while (keyboard_get_char() != '\n') {}
    terminal_clear();

    if (kernel_filesystem_status == FILESYSTEM_SUCCESS) {
        const program_startup_t shell_startup = {
            .arguments = 0,
            .arguments_length = 0,
            .standard_output = { .type = PROGRAM_OUTPUT_TERMINAL }
        };
        kernel_shell_status = executable_run(
            &kernel_filesystem,
            "shell",
            5,
            SHELL_LOAD_ADDRESS,
            &program_services,
            &shell_startup,
            &kernel_shell_exit_code
        );
    } else {
        kernel_shell_status = EXECUTABLE_ERROR_FILE;
    }
    for (;;) __asm__ volatile ("hlt");
}
