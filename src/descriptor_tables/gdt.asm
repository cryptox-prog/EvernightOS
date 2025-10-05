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
; GOTO docs/global_descriptor_table for a more detailed explaination
;----------------------
[BITS 16]

gdt_start:
    ; Null Segment
    TIMES 8 DB 0
    
    ; Code Segment
    DW lsb16limit
    DW 0
    DB 0
    DB present_ring0_code_executable_conforming_readable
    DB paging_32operands_32code_limit4msb
    DB 0

    ; Data Segment
    DW lsb16limit
    DW 0
    DB 0
    DB present_ring0_data_unexecutable_nonconforming_readable
    DB paging_32operands_32code_limit4msb
    DB 0

; This tag acts as a marker for the end of the GDT, then used to calculate the size of the GDT
gdt_end:

gdt_descriptor:
    DW gdt_end - gdt_start - 1              ; '-1' since count starts from 0
    DD gdt_start                            ; DD is define double word since the address is in 32 bit mode

; EQU lets us assign cosntant values to a symbol (works like include the exact value replaces the symbok upon assembly of code
code_segment EQU 0x08                       ; Starts after the 8 bits (0 - 7) of the null segment
data_segment EQU 0x10                       ; starts after the 16 bits (0 - 15) of the code and null segment

lsb16limit EQU 0xffff
present_ring0_code_executable_conforming_readable EQU 0x9e
present_ring0_data_unexecutable_nonconforming_readable EQU 0x92
paging_32operands_32code_limit4msb EQU 0xcf