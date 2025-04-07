GLOBAL isr_0x20

isr_0x20:
    PUSHA                           ; Save all general-purpose registers
    MOV AL, 'Z'                     ; Character to display
    MOV AH, 0x07                    ; Attribute: light gray on black
    MOV EDI, 0xb8000
    MOV [EDI], AX                   ; Write to top-left cell of screen
    POPA                            ; Restore registers
    IRET
