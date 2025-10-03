GLOBAL isr_0x20

isr_0x20:
    CLI         ; Disable interrupts
    HLT         ; Stop CPU
