#ifndef IDT_H
#define IDT_H

#include "types.h"

#define IDT_SIZE 256

/// @brief Defines the structure of a single Interrupt Vector for 32 bit mode, 64 bits in total with 32 bit offset
/// @cite https://wiki.osdev.org/Interrupt_Descriptor_Table
struct InterruptDescriptor32;
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

// Structure representing the IDT Register (IDTR)
struct IDT_Descriptor;
typedef struct IDT_Descriptor {
    uint16_t limit; // Size of the IDT in bytes minus 1
    uint32_t base;  // Base address of the IDT array
} __attribute__((packed)) IDT_Descriptor;
// The packed attribute disables padding which would have been provided to the smaller datatype to account for alignment

void set_idt_entry(uint8_t vector_number, uint32_t isr_address, uint16_t selector, uint8_t type_attr);
void init_idt(void);
void write_idt(void);

#endif // IDT_H
