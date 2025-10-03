#ifndef IO_H
#define IO_H

#include "utils/types.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("out %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif // IO_H
