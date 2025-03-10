[BITS 16]
[ORG 0x7c00]

start:
    ; AX 
    MOV AL, 'X'							; Store the ASCII char X in the lower part of AX
    MOV AH, 0x0e						; Store the instruction 0e in upper part of AX, 0e is instruction for teletype output, just think of it as print waht is in AL
	
    INT 0x10							; Execute the the instruction in AL

JMP $

TIMES 510 - ($ - $$) db 0
DW 0xaa55
