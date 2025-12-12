GLOBAL default_isr
GLOBAL isr_keyboard

default_isr:
    PUSHA                           ; Save all general-purpose registers
    MOV AL, 'K'                     ; Character to display
    MOV AH, 0x07                    ; Attribute: light gray on black
    MOV EDI, 0xb8000
    MOV [EDI], AX                   ; Write to top-left cell of screen
    MOV AL, 0x20
    OUT 0x20, AL
    POPA                            ; Restore registers
    IRET

isr_keyboard:
    PUSHA                           ; Save all general-purpose registers

    IN AL, 0x60                     ; read scancode to acknowledge

    MOV AH, 0x07                    ; Attribute: light gray on black
    MOV EDI, 0xb8000
    MOV [EDI], AX                   ; Write to top-left cell of screen

    MOV AL, 0x20
    OUT 0x20, AL

    POPA                            ; Restore registers
    IRET
