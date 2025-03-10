[BITS 16]
[ORG 0x7c00]

start:
    CALL print								; Used for jumping with the expectation of being returned here, JMP does not expect coming back to this point
    JMP hang

%include "include_directive_printer.asm"				; THe include directive, this essentialy just adds the code in the file passed in it to this file, so no need to worry about what was defined where

hang:
    JMP hang

msg DB "Hey There", 0

TIMES 510 - ($ - $$) DB 0
DW 0xaa55
