# Interrupts in Protected Mode

An **interrupt*is a signal which tells the processor to stop whatever it is doing to handle something else. This is done instead of polling (i.e., continuously checking the state of a device to determine if an action needs to be taken). Interrupts are more performance efficient than polling.

In real mode, BIOS interrupts are active and can perform low-level functions. However, once we switch to protected mode, these interrupts are no longer active. Instead, an **Interrupt Descriptor Table (IDT)*is used, which stores pointers to programmer-defined interrupts to handle them.

The BIOS has its own interrupt vector table mapping to BIOS routines (functions) which do their defined tasks. Once we switched we are using paging (maybe), have a GDT (new code segment), are using 32 bit addressing mode. The BIOS functions had none of those, so we have to write our own interrupt service routines (`ISR`) and define our own and write a `IDT` which maps to these funcitons.

## IDT Entry Format

The IDT stores values in the following format:

```c
uint16_t offset_1; // offset bits 0..15
uint16_t selector; // a code segment selector in GDT or LDT
uint8_t  zero; // unused, set to 0
/*
 7    6     5    4    3      2      1      0
[P | DPL | DPL | 0 | TYPE | TYPE | TYPE | TYPE]
P (Present)    Must be 1 for the descriptor to be valid
DPL (Descriptor Privilege Level) Ring level
S (Storage Segment) Always 0 for interrupt/trap gates
Gate Type    Describes the kind of gate (interrupt/trap/task)
*/
uint8_t  type_attributes; // gate type, dpl, and p fields
uint16_t offset_2; // offset bits 16..31
```

The base address and size of this table are stored in the `lidt` register.

## PIC

The i386 systems generally use 8259 PIC to control its interrupts. A single chip can handle 8 interrupts. To have more interrupts two such chips were cascaded to get **15*interrupts (npt 16 as one of the singlas had to be used to signal to the second chip). Now the reason we are talking about PIC at all is because we have remap the PIC.
The CPU has some internal exceptions (divide by zero, invalid opcode, etc..), these also end up looking at the IDT for what to do in the protected mode. These go from `0x00` to `0x1f` (i.e., 0 to 31), however the PIC can also signals in this range form `0x08` to `0x0f` (the master) (the slave sendes `0x70` to `0x77`), so if we start the PIC signals from `0x20` (32) so that this problem is gone.

## Remaping PIC
To remap the 8259 PIC control words need to be sent to the chip (Initialization Control Words), in order from 1 to 4 (inclusive).
ICW1 is for setting initializiation conditions. The format is as follows:
```
 7    6    5    4      3       2      1       0
[0 |  0  | 0 | INIT | LTIM |  ADI |  SNGL |  IC4 ]
INIT (Initialize) 1 for Start initilization
LTIM (Level Triggered Interrupt Mode) 1 for trigger IRQ at high level, or trigger at low to high transition
ADI (Address Interval) Not needed for 8086, In 8080/8085 PIC gave [Call Instruction|Fixed Interval Address],the increment of that address is decided by this
SNGL (Single) - 1 if no cascading
IC4 (Initialise Control 4) 1 if ICW4 will follow (This has to be specified because systems like 8080 dont need ICW4)
```

ICW2 are just vector (index) offsets for the interrupts (IRQ<x> + offset), they can be differnt for master and slave (0x20, 0x28 respectively for us).

For ICW3 the master gets a 8-bit one hot encoded value while slave gets a plain number. This tells master which IRQ the slave is at, and the slave which irq of master it is connected to. The hardware has non programmable lines which tell it wether it is a slave or not, so it knows how to interpret ICW3 and other stuff.

ICW4 is for some final settings. The format is as follows:
```
 7    6    5     4      3      2      1       0
[0 |  0  | 0 | SFNM |  BUF |  M/S |  AEOI |  uPM ]
SFNM (Special fully nested node) 1 if slave is to have slave 
BUF (Buffer Mode) If the the system bus doesnt drive the PIC then extra hardware is needed for a buffered mode (0 for non buffered)
M/S (Master Slave Buffered Mode) Only matters if BUF - 1, 0 for slave buffer
AEOI (Automatic end of interrupt) 1 for automatic, 0 then system has to sen end of interrupt signal
uPM - (Microprocessor Mode) 1 for 8086/8088, 0 for 8080/8085
```
ICW1 is sent to the command port while the rest are sent to the Data Port.

## Refrences
- [PIC](https://wiki.osdev.org/8259_PIC)
- [ICW](https://www.geeksforgeeks.org/computer-organization-architecture/command-words-of-8259-pic/)

