#include "descriptor_tables/idt.h"
#include "utils/ports.h"
#include <stdint.h>

InterruptDescriptor32 idt[IDT_SIZE];

extern void default_isr(void);
extern void isr_keyboard(void);

/// @brief Essentially a constructor for the InterruptDescriptor32
/// @param vector_number The entry number in the table
/// @param isr_address The 32-bit number containing both the offset_1 and offset_2
/// @param selector The pointer to the code segment as described in the gdt
/// @param type_attr Set privileges using types and attributes
void set_idt_entry(uint8_t vector_number, uint32_t isr_address, uint16_t code_segment_selector, uint8_t type_attr) {
    idt[vector_number].offset_1 = get_lower_16(isr_address);
    idt[vector_number].selector = code_segment_selector;
    idt[vector_number].zero = 0; // This field is always 0 by convention
    idt[vector_number].type_attributes = type_attr;
    idt[vector_number].offset_2 = get_upper_16(isr_address);
}

/// @brief Initialize all the IDT entries
/// @note Does not enable interrupts
void init_idt(void) {
    for (uint16_t i = 0; i < IDT_SIZE; i++) {
        // For each interrupt vector, set a default entry.
        // Selector 0x08: typical kernel code segment selector.
        // Type attribute 0x8E: present, DPL=0, 32-bit interrupt gate.
        set_idt_entry(i, (uint32_t)default_isr, 0x08, 0x8E);
    }
    set_idt_entry(0x21, (uint32_t)isr_keyboard, 0x08, 0x8E);

    pic_remap();
    write_idt();
    
    irq_clear_mask(IRQ_LINE_KEYBOARD);
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

/// @brief Remap the master and slave pic for given offsets (offset + IRQ<x>)
/// @note by vector it simply means that they are a form of index
void pic_remap(void) {
    // Start Initialization
    uint8_t ICW1 = ICW1_INIT | ICW1_CASCADE_MODE | ICW1_USE_TRIGGER_EDGE | ICW1_EXPECT_ICW4;
	write_to_port(PIC_MASTER_COMMAND_PORT, ICW1);
	io_wait();
	write_to_port(PIC_SLAVE_COMMAND_PORT, ICW1);
	io_wait();

    // Set Offset
	write_to_port(PIC_MASTER_DATA_PORT, ICW2_OFFSET_MASTER);
	io_wait();
	write_to_port(PIC_SLAVE_DATA_PORT, ICW2_OFFSET_SLAVE);
	io_wait();

    // Define the cascade order
	write_to_port(PIC_MASTER_DATA_PORT, ICW3_SLAVE_AT_IRQ_MASK);
	io_wait();
	write_to_port(PIC_SLAVE_DATA_PORT, ICW3_MASTER_IRQ_FOR_SLAVE_VALUE);
	io_wait();
	
    // Change to 8086 mode
    uint8_t ICW4 = ICW4_USE_8086_MODE | ICW4_ISR_END_MANUAL | ICW4_BUFFER_NO | ICW4_MULTI_NESTING_NO;
	write_to_port(PIC_MASTER_DATA_PORT, ICW4);
	io_wait();
	write_to_port(PIC_SLAVE_DATA_PORT, ICW4);
	io_wait();

	// Mask both PIC
	write_to_port(PIC_MASTER_DATA_PORT, 0xff);
	write_to_port(PIC_SLAVE_DATA_PORT, 0xff);
}

/// @brief Disable a IRQ line
/// @param irq_line The IRQ line to disable
void irq_set_mask(uint8_t irq_line) {
    uint16_t port = pick_pic_data_port(irq_line);
    uint8_t value = read_from_port(port) | IRQ_ONE_HOT_ENCODE(irq_line);
    write_to_port(port, value);
}

/// @brief Enable a IRQ line
/// @param irq_line The IRQ line to enable
void irq_clear_mask(uint8_t irq_line) {
    uint16_t port = pick_pic_data_port(irq_line);
    uint8_t value = read_from_port(port) & ~IRQ_ONE_HOT_ENCODE(irq_line);

    write_to_port(port, value);        
}


