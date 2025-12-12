#ifndef IDT_H
#define IDT_H

#include "utils/types.h"

#define IDT_SIZE 256


#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)


#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

#define CASCADE_IRQ 2

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
void PIC_remap(int offset1, int offset2);
void IRQ_set_mask(uint8_t IRQline);
void IRQ_clear_mask(uint8_t IRQline);

#endif // IDT_H
