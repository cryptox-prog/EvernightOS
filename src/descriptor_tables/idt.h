#ifndef IDT_H
#define IDT_H

#include "utils/types.h"

#define IDT_SIZE 256

#define PIC_MASTER_COMMAND_PORT 0x0020
#define PIC_MASTER_DATA_PORT (PIC_MASTER_COMMAND_PORT + 1)
#define PIC_SLAVE_COMMAND_PORT 0x00a0
#define PIC_SLAVE_DATA_PORT (PIC_SLAVE_COMMAND_PORT + 1)

#define IRQ_ONE_HOT_ENCODE(irq_line) (1U << (irq_line))

// Initializaton Control Words
// See docs/interrupts.md for detailed breakdown of each Control Word
#define ICW1_EXPECT_ICW4 0x01
#define ICW1_NO_EXPECT_ICW4 0x00

#define ICW1_CASCADE_MODE 0x00
#define ICW1_CASCADE_MODE_NO 0x02

#define ICW1_USE_TRIGGER_LEVEL 0x08
#define ICW1_USE_TRIGGER_EDGE 0x00

#define ICW1_INIT 0x10
#define ICW1_INIT_NO 0x00

#define ICW2_OFFSET_MASTER 0x20
#define ICW2_OFFSET_SLAVE 0x28

#define CASCADE_IRQ 2
#define ICW3_SLAVE_AT_IRQ_MASK IRQ_ONE_HOT_ENCODE(CASCADE_IRQ)
#define ICW3_MASTER_IRQ_FOR_SLAVE_VALUE CASCADE_IRQ

#define ICW4_USE_8086_MODE 0x01
#define ICW4_USE_8080_MODE 0X00

#define ICW4_ISR_END_AUTO 0x02
#define ICW4_ISR_END_MANUAL 0X00

#define ICW4_BUFFER_NO 0X00
#define ICW4_BUFFER_SLAVE 0x08
#define ICW4_BUFFER_MASTER 0x0c

#define ICW4_MULTI_NESTING 0x10
#define ICW4_MULTI_NESTING_NO 0x00

// IRQ Lines
#define IRQ_LINE_TIMER 0
#define IRQ_LINE_KEYBOARD 1

/// @brief Defines the structure of a single Interrupt Vector for 32 bit mode, 64 bits in total with 32 bit offset
/// @cite https://wiki.osdev.org/Interrupt_Descriptor_Table
/// @cite docs/interrupts.md
struct InterruptDescriptor32;
typedef struct InterruptDescriptor32 {
    uint16_t offset_1;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attributes;
    uint16_t offset_2;
} InterruptDescriptor32;

// Structure representing the IDT Register (IDTR)
struct IDT_Descriptor;
typedef struct IDT_Descriptor {
    uint16_t limit; // Size of the IDT in bytes minus 1
    uint32_t base;  // Base address of the IDT array
} __attribute__((packed)) IDT_Descriptor;
// The packed attribute disables padding which would have been provided to the smaller datatype to account for alignment

static inline uint16_t get_lower_16(uint32_t input_num) {
    /*
    * 0x1234ABCD = 00010010001101001010101111001101
    *           &  00000000000000001111111111111111
    *              --------------------------------
    *              00000000000000001010101111001101 = 0xABCD
    */
    return input_num & 0xFFFF;
}

static inline uint16_t get_upper_16(uint32_t input_num) {
    /* 
    * upper16 = value >> 16
    *         = 0xCAFEBABE >> 16
    *         = 0x0000CAFE
    */
    return input_num >> 16;
}

static inline uint16_t pick_pic_data_port(uint8_t irq_line) {
    return (irq_line < 8) ? PIC_MASTER_DATA_PORT : PIC_SLAVE_DATA_PORT;
}

void set_idt_entry(uint8_t vector_number, uint32_t isr_address, uint16_t code_segment_selector, uint8_t type_attr);
void init_idt(void);
void write_idt(void);
void pic_remap();
void irq_set_mask(uint8_t irq_line);
void irq_clear_mask(uint8_t irq_line);

#endif // IDT_H
