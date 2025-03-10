; nasm -f bin blinker.asm -o build/blinker.bin 
; qemu-system-i386 -drive format=raw,file=build/blinker.bin
[BITS 16]						; Directive to indicate 16 bit code, the square brackets are not derefrencing in this case
[ORG 0x7c00]						; Directive indicating that the into the memory address 7C00, the CPU starts running code from 7C00 in 16 bit real mode

hang:							; Define a label, this label can be considered a number pointer can use it calculate lenght of some section of code given another pointer
    JMP hang						; Jumps to the start of the code in hang pointer

; syntax -  times <number of times> <operation>
; DB - define byte, write 8 bits
; $ - numeric pointer of current location in memory
; $$ - numeric pointer to start of file (7C00) in memory
; fills the remaining space from (7C00 + space already taken) to (0x7COO + 510 - space already taken)
TIMES 510 - ($ - $$) DB 0

; DW - define word, write 16 bits, or 2 bytes in memory
; aa55 is the magic number which need to be at the end of sector for BIOS to recognize it as bootable
; aa55 - 10101010 01010101
DW 0xaa55

; NOTE: Memory is RAM
