#include "types.h"

extern void isr_0x20(void);

#define IDT_SIZE 256

struct InterruptDescriptor32;
struct IDT_Descriptor;

void set_idt_entry(uint8_t vector_number, uint32_t isr_address,uint16_t selector, uint8_t type_attr) ;
void initialize_idt(void);

/// @brief Defines the structure of a single Interrupt Vector for 32 bit mode, 64 bits in total with 32 bit offset
/// @cite https://wiki.osdev.org/Interrupt_Descriptor_Table
typedef struct InterruptDescriptor32 {
    uint16_t offset_1;        // offset bits 0..15
    uint16_t selector;        // a code segment selector in GDT or LDT
    uint8_t  zero;            // unused, set to 0
    /*
    *  7    6     5    4    3      2      1      0
    * [P | DPL | DPL | 0 | TYPE | TYPE | TYPE | TYPE]
    * P (Present)	Must be 1 for the descriptor to be valid
    * DPL (Descriptor Privilege Level) Ring level
    * S (Storage Segment) Always 0 for interrupt/trap gates
    * Gate Type	Describes the kind of gate (interrupt/trap/task)
    */
    uint8_t  type_attributes; // gate type, dpl, and p fields
    uint16_t offset_2;        // offset bits 16..31
} InterruptDescriptor32;

InterruptDescriptor32 idt[IDT_SIZE];

// Structure representing the IDT Register (IDTR)
typedef struct IDT_Descriptor {
    uint16_t limit; // Size of the IDT in bytes minus 1
    uint32_t base;  // Base address of the IDT array
} __attribute__((packed)) IDT_Descriptor;
// The packed attribute disables padding which would have been provided to the smaller datatype to account for alignment

/// @brief Essentialy a constructor for the InterruptDescriptor32
/// @param vector_number The entry number in the table
/// @param isr_address The 32-bit number containing both the offset_1 and offset_2
/// @param selector The pointer to the code segment as described in the gdt
/// @param type_attr Set privelages
void set_idt_entry(uint8_t vector_number, uint32_t isr_address, uint16_t selector, uint8_t type_attr) {
    /*
    * Extracting two 16 bit numbers from a 32 bit number:
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
/// @param None 
void initialize_idt(void) {
    for (uint16_t i = 0; i < IDT_SIZE; i++) {
        // For each interrupt vector, set a default entry.
        // Selector 0x08: typical kernel code segment selector.
        // Type attribute 0x8E: present, DPL=0, 32-bit interrupt gate.
        set_idt_entry(i, (uint32_t)isr_0x20, 0x08, 0x8E);
    }
}

/// @brief Write the IDT into the LIDT register
/// @param None
void write_idt(void) {
    IDT_Descriptor idtr;
    idtr.limit = (uint16_t)((sizeof(InterruptDescriptor32) * IDT_SIZE) - 1); // Count starts from 0 so '-1'
    idtr.base = (uint32_t)&idt;
    
    // Inline assembly to load the IDT using the lidt instruction
    // volatile to prevent any compiler optimizations
    // __asm__ could have been asm doesn't matter for newer versions of C
    // m is a directive to make for essentialy lidt [idtr]
    __asm__ volatile ("lidt %0" : : "m" (idtr));
}
