// kernel.c
#include "types.h"

// Forward-declare the functions defined in idt.c
void initialize_idt(void);
void write_idt(void);

// Prototypes
static inline void outb(uint16_t port, uint8_t value);
void move_cursor(uint16_t position);
void clear_screen(void);

void kernel_main(void) {
    // Set up and load the IDT so that interrupts use your ISR wrapper
    initialize_idt();
    write_idt();

    clear_screen();

    // Write a character 'Y' to VGA video memory for your own test
    //volatile char *vidptr = (char*)0xb8000;
    //*vidptr = 'Y';

    // Trigger the ISR by generating a software interrupt (vector 0x20)
    //asm volatile ("int $0x20");

    while (1);
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}


void move_cursor(uint16_t position) {
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position >> 8) & 0xFF));
}

void clear_screen(void) {
    volatile char *vidptr = (char*)0xb8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vidptr[i] = ' ';       // ASCII space
        vidptr[i + 1] = 0x07;  // Light gray on black
    }
}
