[BITS 16]

print:
    MOV SI, msg
    
    JMP print_looper

print_looper:
    LODSB
    CMP AL, 0
    JZ print_complete

    MOV AH, 0x0e
    INT 0x10

    JMP print_looper

print_complete:
    RET								; RET is used in conjuction wit CALL to return to the last return address on the stack
