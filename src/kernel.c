// kernel.c
void kernel_main(void);

/// @brief The main function managing all the kernel operations, is called by the assembly bootloader
/// @param  None
void kernel_main(void) {
    // Write a character 'X' to video memory
    volatile char *vidptr = (char*)0xb8000;
    *vidptr = 'Y';
    while(1);
}
