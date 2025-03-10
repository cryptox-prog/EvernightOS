[BITS 16]
[ORG 0x7c00]

start:
    MOV SI, msg							; msg points to the start of the string it was assigned to, so the address of that start is now loaded to SI
    
    JMP print_loop

print_loop:
    LODSB							; This loads [SI] ,i.e., the value at pointer (pointing somewhere in memory) of SI, into AL, and then increments SI (direction flag = 0, reverse if DF = 1, should have used CLD before loop to make sure it was 0)
    CMP AL, 0							; This does 0 - AL, the result can be called a TEMP, this isnt stored anywhere, instead it updates several flag including zero flag, sign flag, carry flag, overflow flag, parity flag
    JZ hang							; Jump if Zero, checks the zero flag (ZF = 1 => jump)

    MOV AH, 0x0e
    INT 0x10

    JMP print_loop

hang:
    JMP hang

msg DB 'Hello There', 0

TIMES 510 - ($ - $$) DB 0
DW 0xaa55
