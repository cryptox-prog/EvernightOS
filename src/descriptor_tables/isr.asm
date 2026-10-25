GLOBAL default_isr
GLOBAL isr_keyboard

extern keypress_buffer_enqueue

default_isr:
    PUSHA                           ; Save all general-purpose registers
    MOV AL, 'K'                     ; Character to display
    MOV AH, 0x07                    ; Attribute: light gray on black
    MOV EDI, 0xb8000
    MOV [EDI], AX                   ; Write to top-left cell of screen
    MOV AL, 0x20
    OUT 0x20, AL
    POPA                            ; Restore registers
    IRETD

isr_keyboard:
    PUSHA
    CLD
    JMP .keyboard_output_buffer_consume

.keyboard_output_buffer_consume:
    IN AL, keyboard_status_port

    TEST AL, keyboard_output_buffer_full
    JZ .done
    
    IN AL, keyboard_data_port

    CMP AL, keyboard_acknowledge_code
    JE .keyboard_output_buffer_consume
    CMP AL, keyboard_resend_code
    JE .keyboard_output_buffer_consume
    ; Forward make/break codes and prefixes so C can track modifiers.

    PUSH EAX
    CALL keypress_buffer_enqueue
    ADD ESP, 4

    JMP .keyboard_output_buffer_consume

.done:
    MOV AL, 0x20
    OUT 0x20, AL
    POPA
    IRETD


keyboard_data_port EQU 0x60
keyboard_status_port EQU 0x64
keyboard_output_buffer_full EQU 0x01
keyboard_acknowledge_code EQU 0xfa
keyboard_resend_code EQU 0xfe
keyboard_key_release_code EQU 0x80
keyboard_prefix_code EQU 0xe0           ; For exteneded scan codes
