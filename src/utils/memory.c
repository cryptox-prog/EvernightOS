#include "utils/memory.h"

void memory_copy(void *destination, const void *source, uint32_t byte_count) {
    uint8_t *output = destination;
    const uint8_t *input = source;
    for (uint32_t index = 0; index < byte_count; index++) {
        output[index] = input[index];
    }
}

void memory_set(void *destination, uint8_t value, uint32_t byte_count) {
    uint8_t *output = destination;
    for (uint32_t index = 0; index < byte_count; index++) {
        output[index] = value;
    }
}
