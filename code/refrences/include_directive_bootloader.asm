[BITS 16]
[ORG 0x7c00]

start:
    CALL print
    JMP hang

%include "include_directive_printer.asm"

hang:
    JMP hang

msg DB "Hey There", 0

TIMES 510 - ($ - $$) DB 0
DW 0xaa55
