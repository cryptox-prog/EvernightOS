/* second_stage.ld */
ENTRY(_start)
SECTIONS {
    . = 0x1000;           /* Load second stage at 0x1000 */
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : {
        __bss_start = .;
        *(.bss) *(COMMON)
        __bss_end = .;
    }
}
