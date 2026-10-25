#ifndef PORTS_H
#define PORTS_H

#include "utils/types.h"


/*
asm ( "assembly template"
      : output operands
      : input operands
      : clobbers
      : goto labels
);
*/

/// @brief Write one byte to an I/O port
/// @param port The port to write to
/// @param val The value to write
/// @cite https://wiki.osdev.org/Inline_Assembly/Examples#I/O_access
static inline void write_to_port(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
    /* There's an outb %al, $imm8 encoding, for compile-time constant port numbers that fit in 8b. (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type
     * */
}

/// @brief Read one byte from an I/O port
/// @param port The port to read from
/// @return The byte read from the port
/// @cite https://wiki.osdev.org/Inline_Assembly/Examples#I/O_access
static inline uint8_t read_from_port(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %w1, %b0"
                     : "=a"(ret)
                     : "Nd"(port)
                     : "memory");
    return ret;
}


static inline void io_wait(void) {
    // Write to an unused I/O port (0x80 is traditional).
    // It takes a small amount of time to complete, acting as a delay.
    write_to_port(0x80, 0);
}

#endif // PORTS_H
