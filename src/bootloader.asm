[BITS 16]
[ORG 0x7c00]

start:
    CLI                                     ; Clear Interrupts, to prevent interference during important ececution of important instructions

    ; Enable A20 line via port 0x92. A20 line is activate because we need to access more than 1MB of memory
    ; TODO: Look at better implementations
    IN  AL, 0x92                            ; Load the value of port 0x92 into AL
    OR  AL, 0x02                            ; Make sure the 1st bit (start counting from 0) is 1
    OUT 0x92, AL                            ; Put the output into the 0x92 port thus turning on the A20 line

    ; Save boot drive (BIOS passes the boot drive number into DL by default, it is 1byte long)
    MOV [boot_drive], DL

    MOV AX, 0
    MOV ES, AX                              ; ES = 0
    MOV BX, second_stage_addr               ; BX = 0x1000 (destination address)

    MOV AH, 0x02                            ; BIOS function: Read sectors
    MOV AL, 1                               ; Number of sectors to read (Check the size of the second_stage binary file (NOT ELF)
    MOV CH, 0                               ; Cylinder 0
    MOV CL, 2                               ; Start at sector 2 (sector 1 is our bootloader)
    MOV DH, 0                               ; Head 0
    MOV DL, [boot_drive]                    ; Load Boot drive number again to ensure no loss occured
    INT 0x13                                ; BIOS disk read interrupt, when it is called it checks the above registers for the location of the data on the drive
    JC disk_error                           ; Jump if Carry Flag, set by int 0x13 when it fails to read

    ; Making segments 0
    XOR AX, AX
    MOV DS, AX
    MOV CS, AX

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


%include "gdt.asm"

disk_error:
    JMP disk_error

; Data declarations
boot_drive: DB 0                            ; The one byte needs to be defined to accomodate for when the boot drive number is loaded onto it
second_stage_addr EQU 0x1000                ; Address where the second stage is loaded

TIMES 510 - ($ - $$) DB 0
DW 0xaa55
