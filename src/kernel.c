#include "descriptor_tables/idt.h"
#include "display/welcome_screen.h"
#include "drivers/keyboard.h"

static uint32_t cursor;
static uint32_t input_start;

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
    input_start = input_start >= VGA_WIDTH ? input_start - VGA_WIDTH : 0;
}

static void prompt(void) {
    put_cell(cursor++, '$');
    input_start = cursor;
}

void kernel_main(void) {
    __asm__ volatile ("cli");
    init_idt();
    init_keyboard();
    display_welcome_screen();
    __asm__ volatile ("sti");

    while (keyboard_get_char() != '\n') {}
    clear_screen();
    cursor = 0;
    prompt();
    set_cursor_position(0, cursor);

    for (;;) {
        char c = keyboard_get_char();
        if (c == '\b') {
            if (cursor > input_start) put_cell(--cursor, ' ');
        } else if (c == '\n') {
            cursor = (cursor / VGA_WIDTH + 1) * VGA_WIDTH;
            scroll_if_needed();
            prompt();
        } else if (c >= ' ' && c <= '~') {
            put_cell(cursor++, c);
            scroll_if_needed();
        }
        set_cursor_position(cursor / VGA_WIDTH, cursor % VGA_WIDTH);
    }
}
