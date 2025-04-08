[BITS 32]

section .data
; Define the data segment selector (as set in your GDT)
%DEFINE data_segment 0x10

section .text
    global _start                           ; Entry point for the second stage
    extern kernel_main                      ; The C function to be called

_start:
    ; Set up flat segmentation (DS, ES, FS, GS, and SS all point to data_segment)
    MOV AX, data_segment
    MOV DS, AX
    MOV ES, AX
    MOV FS, AX
    MOV GS, AX
    MOV SS, AX

    ; Set up a stack pointer (choose an address above your code/data; adjust as needed)
    MOV esp, 0x90000

    ; Call the C kernel entry point (kernel_main)
    CALL kernel_main

hang:
    JMP hang                                ; Infinite loop if kernel_main ever returns
