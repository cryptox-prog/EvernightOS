# Interrupts in Protected Mode

An **interrupt** is a routine which tells the processor to stop whatever it is doing to handle something else. This is done instead of polling (i.e., continuously checking the state of a device to determine if an action needs to be taken). Interrupts are more performance efficient than polling.

In real mode, BIOS interrupts are active and can perform low-level functions. However, once we switch to protected mode, these interrupts are no longer active. Instead, an **Interrupt Descriptor Table (IDT)** is used, which stores pointers to programmer-defined interrupts to handle them.

## IDT Entry Format

The IDT stores values in the following format:

```c
uint16_t offset_1;        // offset bits 0..15
uint16_t selector;        // a code segment selector in GDT or LDT
uint8_t  zero;            // unused, set to 0
/*
*  7    6     5    4    3      2      1      0
* [P | DPL | DPL | 0 | TYPE | TYPE | TYPE | TYPE]
* P (Present)    Must be 1 for the descriptor to be valid
* DPL (Descriptor Privilege Level) Ring level
* S (Storage Segment) Always 0 for interrupt/trap gates
* Gate Type    Describes the kind of gate (interrupt/trap/task)
*/
uint8_t  type_attributes; // gate type, dpl, and p fields
uint16_t offset_2;        // offset bits 16..31
```

The base address and size of this table are stored in the `lidt` register.

---

**TODO:** Add PIC to this. Also add references.
