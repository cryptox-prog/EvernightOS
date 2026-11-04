/* second_stage.ld */
ENTRY(_start)
SECTIONS {
    . = 0x1000;           /* Load second stage at 0x1000 */
    .text : { *(.text) }
    .data : { *(.data) }

    /* Keep loaded kernel bytes away from the boot sector and its temporary GDT. */
    ASSERT(. < 0x7000, "Kernel code/data has grown into the reserved boot area")

    /* Zero-initialized kernel state begins after the boot sector at 0x7c00. */
    . = 0x8000;
    .bss  : {
        __bss_start = .;
        *(.bss) *(COMMON)
        __bss_end = .;
    }
}
