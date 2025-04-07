// kernel.c
#include "types.h"

// Forward-declare the functions defined in idt.c
void initialize_idt(void);
void write_idt(void);

// Prototypes
void clear_screen(void);

void kernel_main(void) {
    initialize_idt();
    write_idt();

    clear_screen();

    // Write a character 'Y' to VGA video memory for your own test
    volatile char *vidptr = (char*)0xb8000;
    *vidptr = 'Y';

    // Trigger the ISR by generating a software interrupt (vector 0x20)
    __asm__ volatile ("int $0x20");

    // The interrupt print overwrites the kernel print

    while (1);
}

void clear_screen(void) {
    volatile char *vidptr = (char*)0xb8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vidptr[i] = ' ';       // ASCII space
        vidptr[i + 1] = 0x07;  // Light gray on black
    }
}
