[BITS 16]
[ORG 0x7c00]

start:
    ; Initialize the stack to all values as 0 (XOR of A to itself is always 0)
    XOR AX, AX
    MOV DS, AX
    MOV ES, AX
    MOV SS, AX
    MOV SP, 0x7c00                      ; The stack pointer is set to start of code so the stack growing down doesn't eat the code

    MOV SI, msg
    MOV CX, 4
    ADD SI, CX							; displace the stack index by the length of the string
    DEC SI                              ; it was pointing at 0 after offet so set it to current - 1
    
    JMP push_loop

push_loop:
    MOV AL, [SI]                        ; Load the value of pointer of SI to AL
    PUSH AX							    ; Push AX to Stack, thus emptying AX
    DEC SI							    ; Decrement SI by 1
    LOOP push_loop						; if CX != 0  then decrement CX and goto push_loop label
    MOV CX, 4							; The loop would have made CX equal to 0 so refresh it for next loop
    JMP print_loop

print_loop:
    POP AX							    ; Get the top value from the stack that initially will be '4' in this case
    MOV AH, 0x0e
    INT 0x10
    LOOP print_loop
    JMP hang

hang:
    JMP hang

msg DB "1234", 0

TIMES 510 - ($ - $$) DB 0
DW 0xaa55

;---------------
; The call functionality could not be utilize in a direct manner here as that adds the return address to the stack interfering with our data
; TDOO: look into utilizing stack and call at same time in future
;---------------
