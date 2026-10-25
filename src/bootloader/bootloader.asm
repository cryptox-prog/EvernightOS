[BITS 16]
[ORG 0x7c00]

start:
    CLI                                     ; Clear Interrupts, to prevent interference during important ececution of important instructions
    JMP 0x0000:real_mode_start

real_mode_start:
    XOR AX, AX
    MOV DS, AX
    MOV ES, AX
    MOV SS, AX
    MOV SP, 0x7c00
    CLD

    ; Enable A20 line via port 0x92. A20 line is activate because we need to access more than 1MB of memory
    ; TODO: Look at better implementations
    IN  AL, system_control_port_a           ; Load the value of port 0x92 into AL
    OR  AL, 0x02                            ; Make sure the 1st bit (start counting from 0) is 1 so OR with 000000010
    OUT system_control_port_a, AL           ; Put the output into the 0x92 port thus turning on the A20 line

    ; Save boot drive (BIOS passes the boot drive number into DL by default, it is 1byte long)
    MOV [boot_drive], DL

    MOV AX, 0
    MOV ES, AX                              ; ES = 0
    MOV BX, second_stage_addr               ; BX = 0x1000 (destination address)

    MOV AH, bios_read_sector_function
    MOV AL, [second_stage_sectors]          ; Number of sectors to read (Check the size of the second_stage binary file (NOT ELF)
    MOV CH, 0                               ; Cylinder 0
    MOV CL, 2                               ; Start at sector 2 (sector 1 is our bootloader)
    MOV DH, 0                               ; Head 0
    MOV DL, [boot_drive]                    ; Load Boot drive number again to ensure no loss occured
    INT read_from_hardrive_interrupt
    JC disk_error                           ; Jump if Carry Flag, set by int 0x13 when it fails to read

    ; Making segments 0
    XOR AX, AX
    MOV DS, AX
    ; CS is loaded by the far jump below; MOV to CS is an invalid opcode.

    lgdt [gdt_descriptor]

    ; The CR0 register contains the PE (Protection Enable) bit setting it to 1 results in Activation of Protected Mode
    MOV EAX, CR0                            ; EAX is the 32 bit extended register of AX, it is completely fine to utilize it even in 16 bit mode. AX constitutes 0 - 15 of EAX
    OR  EAX, 1
    MOV CR0, EAX

    ;---------------
    ; This is a far jump or a long jump
    ; First this sets the code segment to the value before the ':'
    ; Then it updates the Instruction Pointer (EIP, IP in real mode), to point to protected_mode_start (essentialy what happens in a normal jump)
    ; It also clears the prefetch queue, i.e., a small internal buffer in the cache memory storing a few instructions ahed of thier execution, this lets us discar the instructions stored from the previous code segment
    ;---------------
    JMP code_segment:second_stage_addr


%include "descriptor_tables/gdt.asm"

disk_error:
    JMP disk_error

; Data declarations
boot_drive: DB 0                            ; The one byte needs to be defined to accomodate for when the boot drive number is loaded onto it

second_stage_addr EQU 0x1000                ; Address where the second stage is loaded, defined in linker.ld
bios_read_sector_function EQU 0x02
system_control_port_a EQU 0x92
bootloader_magic_number EQU 0xaa55
read_from_hardrive_interrupt EQU 0x13

TIMES 509 - ($ - $$) DB 0
second_stage_sectors: DB 0                  ; Number of sectors the second stage occupies (will be overwritten in the Makefile)
DW bootloader_magic_number
