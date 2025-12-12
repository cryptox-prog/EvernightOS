# Interrupts in Protected Mode

An **interrupt** is a signal which tells the processor to stop whatever it is doing to handle something else. This is done instead of polling (i.e., continuously checking the state of a device to determine if an action needs to be taken). Interrupts are more performance efficient than polling.

In real mode, BIOS interrupts are active and can perform low-level functions. However, once we switch to protected mode, these interrupts are no longer active. Instead, an **Interrupt Descriptor Table (IDT)** is used, which stores pointers to programmer-defined interrupts to handle them.

The BIOS has its own interrupt vector table mapping to BIOS routines (functions) which do their defined tasks. Once we switched we are using paging (maybe), have a GDT (new code segment), are using 32 bit addressing mode. The BIOS functions had none of those, so we have to write our own interrupt service routines (`ISR`) and define our own and write a `IDT` which maps to these funcitons.

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

## PIC

The i386 systems generally use 8259 PIC to control its interrupts. A single chip can handle 8 interrupts. To have more interrupts two such chips were cascaded to get **15** interrupts (npt 16 as one of the singlas had to be used to signal to the second chip). Now the reason we are talking about PIC at all is because we have remap the PIC.
The CPU has some internal exceptions (divide by zero, invalid opcode, etc..), these also end up looking at the IDT for what to do in the protected mode. These go from `0x00` to `0x1f` (i.e., 0 to 31), however the PIC can also signals in this range form `0x08` to `0x0f` (the master) (the slave sendes `0x70` to `0x77`), so if we start the PIC signals from `0x20` (32) so that this problem is gone.

---

**TODO:** add references.
