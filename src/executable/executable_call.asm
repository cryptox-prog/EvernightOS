[BITS 32]

section .text
    global executable_call_entry

; uint32_t executable_call_entry(
;     uint32_t entry_address,
;     uint32_t stack_top,
;     const program_services_t *services,
;     const program_startup_t *startup
; )
executable_call_entry:
    PUSH EBP
    MOV EBP, ESP
    PUSH EBX
    PUSH ESI
    PUSH EDI

    MOV EAX, [EBP + 8]
    MOV EDX, [EBP + 12]
    MOV EBX, [EBP + 16]
    MOV ESI, [EBP + 20]
    MOV ECX, ESP
    MOV ESP, EDX
    PUSH ECX
    PUSH ESI
    PUSH EBX
    CALL EAX
    ADD ESP, 8
    POP ECX
    MOV ESP, ECX

    POP EDI
    POP ESI
    POP EBX
    POP EBP
    RET
