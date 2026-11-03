#ifndef MEMORY_H
#define MEMORY_H

#include "utils/types.h"

void memory_copy(void *destination, const void *source, uint32_t byte_count);
void memory_set(void *destination, uint8_t value, uint32_t byte_count);

#endif // MEMORY_H
