#include "utils/types.h"
#include "display/vga.h"
#include "utils/io.h"
#include "descriptor_tables/idt.h"
#include "display/welcome_screen.h"

void kernel_main(void) {
    init_idt();

    // outb(0x3D4, 0x01);
    // uint8_t width = inb(0x3D5) + 1;
    
    display_welcome_screen();

    // Trigger the ISR by generating a software interrupt (vector 0x20)
    //__asm__ volatile ("int $0x20");

    // The interrupt print overwrites the kernel print

    while (1);
}
