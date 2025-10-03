#include "types.h"
#include "idt.h"
#include "io.h"

void set_cursor_position(uint16_t row, uint16_t col);
void clear_screen(void);

void kernel_main(void) {
    init_idt();

    clear_screen();

    //outb(0x3D4, 0x01);
    //uint8_t width = inb(0x3D5) + 1;

    set_cursor_position(10, 30);

    // Write a character 'Y' to VGA video memory for your own test
    //volatile char *vidptr = (char*)0xb8000;
    //*vidptr = 'Y';

    // Trigger the ISR by generating a software interrupt (vector 0x20)
    //__asm__ volatile ("int $0x20");

    // The interrupt print overwrites the kernel print

    while (1);
}

void set_cursor_position(uint16_t row, uint16_t col) {
    uint16_t pos = row * 80 + col;

    // Send the low byte
    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);

    // Send the high byte
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
}

void clear_screen(void) {
    volatile char *vidptr = (char*)0xb8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vidptr[i] = ' ';       // ASCII space
        vidptr[i + 1] = 0x07;  // Light gray on black
    }
}
