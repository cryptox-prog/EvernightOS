;----------------------
; There are going to be 3 segments in this GDT
; 1. Null Segment at 0
; 2. Code Segment at 1
; 3. Data Segment at 2
; 
; The following is the structure in which each segment will be defined:
; 1. Lower 16-bits of Limit
; 2. Lower 16-bits of Base Address
; 3. Middle 8-bits of Base Address
; 4. Access Flags
; 5. Granularity and Upper 4-bits of Limit
; 6. Upper 8-bits of Base Address
; 
; GOTO theory/gloabal_descriptor_table for a more detailed explaination
;----------------------
[BITS 16]

gdt_start:
    ; Null Segment
    TIMES 8 DB 0
    
    ; Code Segment
    DW 0xffff
    DW 0
    DB 0
    DB 0x9e                                 ; Executable, ring0, conforming, present
    DB 0xcf                                 ; limit in units of pages, operand size 32 bit
    DB 0

    DW 0xffff
    DW 0
    DB 0
    DB 0x92                                 ; Non Executable, ring0, non conforming, present
    DB 0xcf                                 ; same as above
    DB 0

gdt_end:

gdt_descriptor:
    DW gdt_end - gdt_start - 1              ; '-1' since count starts from 0
    DD gdt_start                            ; DD is define double word since the address is in 32 bit mode

; EQU lets us assign cosntant values to a symbol (works like include the exact value replaces the symbok upon assembly of code
code_segment EQU 0x08                       ; Starts after the 8 bits (0 - 7) of the null segment
data_segment EQU 0x10                       ; starts after the 16 bits (0 - 15) of the code and null segment
