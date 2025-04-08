# Real and Protected Mode

## What?
- Initially x86 was designed to work on `16-bit` systems, however even after the development of 32-bit systems to maintain back compatibility, the compueters still start in 16-bit real mode. Real mode has no access levels and utilizes BIOS interrupts.
- `32-bit` system has a 32-bit operand size and can access 4-GB of memory. With the utilization of GDT it give access levels and ither permitions to portions of the memory and with IDT it defines custom interrupts.

## Why?
- Real mode is benefeicial as it allows BIOS to handle interrupts.
- Real mode only allows access to `~1MByte` of RAM, only `64KByte` is available without segmentation, has no security, and the default operand mode is 16-bit (can be change via opperand size override prefix allowing access to 32-bit registers)

NOTE: The BIOS interrupts do not wrok in protected mode because they utilize the real modes conventions of addressing memory

Due to the above issues modern operating systems work in protected mode. It solves all the issues listed above although almost all BIOS interrupts are now innaccesable.

## Switching
Since, computers still start in real mode, a switch needs to performed to protected mode.
The basic steps to do this are:
- Disabling interrupts
- Enabling the `A20` line, is needed to be able to access more than 1MB of memory
- Loading the `GDT` into the `lgdt` register
- Setting the `LSB` of the `cr0` register to 1
- Then finally performing a far jump to the code segment selector
- Setting up the segemnt registers

After all this is done, the protected mode has started however the `IDT` still needs to be loaded into the `lidt` register, and the interrupts need to be re-enabled.

## Refrences
- https://wiki.osdev.org/Real_Mode
- https://wiki.osdev.org/Protected_Mode
- https://wiki.osdev.org/A20_Line
- https://stackoverflow.com/questions/38442880/what-role-does-a20-line-plays-in-protected-mode
- https://www.youtube.com/watch?v=Wh5nPn2U_1w
