# 16 Bit Registers
In assembly, we utilize the registers present in the CPU directly to perform arithmetic operations, data manipulation, etc.

## General Purpose Registers
The registers used for general operations are:
- **AX** - Accumulator Register, primary accumulator for arithmetic and logic operations.
- **BX** - Base Register, used to store the base address (the base address of an array would be the pointer to its first element).
- **CX** - Count Register, used as a counter for loops.
- **DX** - Data Register, generally used to temporarily store data. It can also be used to store the result if the result is larger than 16 bits, with AX storing the lower 16 bits and DX the upper ones. Its usage is more relaxed than other registers, and miscellaneous data can be stored here.

All the above registers are general-purpose 16-bit registers in which the upper and lower parts can be utilized individually using AH and AL. They are supposed to be used for certain things only because of BIOS convention; otherwise, there is no difference in them innately. All the above registers can be accessed directly.

**NOTE:** Just because it is convention doesn't mean it can be completely ignored. The BIOS interrupt 0x10 specifically runs the instruction in AH.

Other general-purpose registers are:
- **SI** - Source Index, used to store the pointer to a character in a string or some other source data.
- **DI** - Destination Index, used to store a pointer to the location in memory where the data should be stored.
- **BP** - Base Pointer, almost always used for addressing with stacks as their pointer to their start.

These registers store pointers. Since we are in 16-bit real mode, we have up to 2^16 possible memory addresses. Thus, to be able to access them in their entirety, these registers are not divided into two.

## Segment Registers
- **CS** - Code Segment, holds the base address to where instructions are stored. To modify this register, 'far jumps' or 'calls' are used; it can't be altered directly.
- **DS** - Data Segment, points to the base address of where the data is stored.
- **ES** - Generally used as a second Data Segment.
- **SS** - Holds the base address for the stack.

While only in the CS segment is the MOV operation not allowed, even in the remaining Segment registers, an immediate value cannot be loaded, i.e., `MOV ES, 0` is incorrect.

This is correct:
```
MOV AX, 0
MOV ES, AX
```
## Refrences
- [8086 16 Bit Registers](https://www.geeksforgeeks.org/types-of-registers-in-8086-microprocessor/)
- [General Purpose Registers](https://www.geeksforgeeks.org/general-purpose-registers-8086-microprocessor/)
