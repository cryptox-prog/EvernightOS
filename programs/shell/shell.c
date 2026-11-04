#include "executable/program_services.h"

#define COMMAND_BUFFER_SIZE 128

static void write_text(const program_services_t *services, const char *text) {
    for (uint32_t index = 0; text[index]; index++) {
        services->terminal_write_character(text[index]);
    }
}

static uint8_t text_matches(const char *text, uint32_t text_length,
                            const char *expected, uint32_t expected_length) {
    if (text_length != expected_length) return 0;
    for (uint32_t index = 0; index < text_length; index++) {
        if (text[index] != expected[index]) return 0;
    }
    return 1;
}

static uint8_t is_recognized_command(const char *command, uint32_t length) {
    return text_matches(command, length, "echo", 4) ||
           text_matches(command, length, "cat", 3) ||
           text_matches(command, length, "exit", 4);
}

static void update_command_color(
    const program_services_t *services,
    const char *command,
    uint32_t command_length
) {
    for (uint32_t index = 0; index < command_length; index++) {
        if (command[index] == ' ') return;
    }
    services->terminal_recolor_previous(
        command_length,
        is_recognized_command(command, command_length)
            ? PROGRAM_COLOR_LIGHT_RED
            : PROGRAM_COLOR_LIGHT_GREY,
        PROGRAM_COLOR_BLACK
    );
}

static void trim_spaces(const char *text, uint32_t *start, uint32_t *end) {
    while (*start < *end && text[*start] == ' ') (*start)++;
    while (*end > *start && text[*end - 1] == ' ') (*end)--;
}

static void run_command(const program_services_t *services, char *command,
                        uint32_t command_length) {
    uint32_t command_start = 0;
    uint32_t command_end = command_length;
    trim_spaces(command, &command_start, &command_end);
    if (command_start == command_end) return;

    uint32_t name_end = command_start;
    while (name_end < command_end && command[name_end] != ' ') name_end++;
    uint32_t argument_start = name_end;
    while (argument_start < command_end && command[argument_start] == ' ') {
        argument_start++;
    }

    program_startup_t child_startup = {
        .arguments = command + argument_start,
        .arguments_length = command_end - argument_start,
        .standard_output = { .type = PROGRAM_OUTPUT_TERMINAL }
    };
    program_file_t redirected_file;
    uint8_t output_is_file = 0;

    if (text_matches(command + command_start, name_end - command_start, "echo", 4)) {
        uint32_t redirect = argument_start;
        while (redirect < command_end && command[redirect] != '>') redirect++;
        if (redirect < command_end) {
            uint32_t text_end = redirect;
            while (text_end > argument_start && command[text_end - 1] == ' ') text_end--;
            uint32_t file_start = redirect + 1;
            uint32_t file_end = command_end;
            trim_spaces(command, &file_start, &file_end);
            if (file_start == file_end ||
                services->file_create(command + file_start,
                                      file_end - file_start,
                                      &redirected_file)) {
                write_text(services, "redirect: could not create file\n");
                return;
            }
            child_startup.arguments_length = text_end - argument_start;
            child_startup.standard_output.type = PROGRAM_OUTPUT_FILE;
            child_startup.standard_output.file = redirected_file;
            output_is_file = 1;
        }
    } else if (!text_matches(command + command_start, name_end - command_start,
                             "cat", 3) &&
               !text_matches(command + command_start, name_end - command_start,
                             "exit", 4)) {
        return;
    }

    uint32_t exit_code;
    uint32_t result = services->execute(command + command_start,
                                        name_end - command_start,
                                        &child_startup,
                                        &exit_code);
    if (output_is_file) services->file_close(&redirected_file);
    if (result || exit_code) write_text(services, "program failed\n");
}

uint32_t program_main(const program_services_t *services,
                      const program_startup_t *startup) {
    (void)startup;
    if (!services || services->abi_version != PROGRAM_SERVICES_ABI_VERSION) return 1;

    services->terminal_clear();
    services->terminal_write_character('$');
    char command[COMMAND_BUFFER_SIZE];
    uint32_t command_length = 0;

    for (;;) {
        char character = services->keyboard_read_character();
        if (character == '\b') {
            if (command_length) {
                services->terminal_write_character(character);
                command_length--;
                update_command_color(services, command, command_length);
            }
        } else if (character == '\n') {
            services->terminal_write_character(character);
            run_command(services, command, command_length);
            services->terminal_write_character('$');
            command_length = 0;
        } else if (character >= ' ' && character <= '~' &&
                   command_length < COMMAND_BUFFER_SIZE) {
            services->terminal_write_character(character);
            command[command_length++] = character;
            update_command_color(services, command, command_length);
        }
    }
}
