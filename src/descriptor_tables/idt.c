#include "descriptor_tables/idt.h"
#include "utils/ports.h"

InterruptDescriptor32 idt[IDT_SIZE];

extern void default_isr(void);
extern void isr_keyboard(void);

/// @brief Essentially a constructor for the InterruptDescriptor32
/// @param vector_number The entry number in the table
/// @param isr_address The 32-bit number containing both the offset_1 and offset_2
/// @param selector The pointer to the code segment as described in the gdt
/// @param type_attr Set privileges
void set_idt_entry(uint8_t vector_number, uint32_t isr_address, uint16_t selector, uint8_t type_attr) {
    /*
    * Extracting two 16-bit numbers from a 32-bit number:
    *
    * 0x1234ABCD = 00010010001101001010101111001101
    *           &  00000000000000001111111111111111
    *              --------------------------------
    *              00000000000000001010101111001101 = 0xABCD
    * 
    * upper16 = value >> 16
    *         = 0xCAFEBABE >> 16
    *         = 0x0000CAFE
    */
    idt[vector_number].offset_1 = isr_address & 0xFFFF;            // Lower 16 bits of the address
    idt[vector_number].selector = selector;                        // Code segment selector
    idt[vector_number].zero = 0;                                   // This field is always 0
    idt[vector_number].type_attributes = type_attr;                // Set the type and attributes
    idt[vector_number].offset_2 = (isr_address >> 16);             // Upper 16 bits of the address
}

/// @brief Initialize all the IDT entries
void init_idt(void) {
    for (uint16_t i = 0; i < IDT_SIZE; i++) {
        // For each interrupt vector, set a default entry.
        // Selector 0x08: typical kernel code segment selector.
        // Type attribute 0x8E: present, DPL=0, 32-bit interrupt gate.
        set_idt_entry(i, (uint32_t)default_isr, 0x08, 0x8E);
    }
    set_idt_entry(0x21, (uint32_t)isr_keyboard, 0x08, 0x8E);

    PIC_remap(0x20, 0x28);

    // 4) Write IDT now that PIC vectors are established
    write_idt();

    // 5) Unmask IRQ1 (keyboard) on master PIC
    IRQ_clear_mask(1); // unmask keyboard IRQ (IRQ1)
    IRQ_set_mask(0); // mask timer IRQ (IRQ0)

    __asm__ volatile ("sti"); // Enable interrupts
}

static IDT_Descriptor idtr;

/// @brief Write the IDT into the LIDT register
void write_idt(void) {
    idtr.limit = (uint16_t)((sizeof(InterruptDescriptor32) * IDT_SIZE) - 1); // Count starts from 0 so '-1'
    idtr.base = (uint32_t)&idt;
    
    // Inline assembly to load the IDT using the lidt instruction
    // volatile to prevent any compiler optimizations
    // __asm__ could have been asm doesn't matter for newer versions of C
    // m is a directive to make for essentially lidt [idtr]
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}


/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/
void PIC_remap(int offset1, int offset2)
{
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 1 << CASCADE_IRQ);        // ICW3: tell Master PIC that there is a slave PIC at IRQ2
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
	
	outb(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	// Unmask both PICs.
	outb(PIC1_DATA, 0x00);
	outb(PIC2_DATA, 0x00);
}

void IRQ_set_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(port, value);        
}

void IRQ_clear_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);        
}


