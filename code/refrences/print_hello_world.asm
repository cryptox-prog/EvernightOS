[BITS 16]
[ORG 0x7c00]

start:
    MOV SI, msg
    
    JMP print_loop

print_loop:
    LODSB
    CMP AL, 0
    JZ hang

    MOV AH, 0x0e
    INT 0x10

    JMP print_loop

hang:
    JMP hang

msg DB 'Hello There', 0

TIMES 510 - ($ - $$) DB 0
DW 0xaa55
