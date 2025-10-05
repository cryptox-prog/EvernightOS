#include "descriptor_tables/idt.h"
#include "display/welcome_screen.h"

void kernel_main(void) {
    init_idt();
    
    display_welcome_screen();

    // Trigger the ISR by generating a software interrupt (vector 0x20)
    //__asm__ volatile ("int $0x20");

    while (1);
}
