#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "utils/types.h"

#define MAX_KEYPRESS_BUFFER_SIZE 64
#define SCANCODE_SET1_TABLE_SIZE 128

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

#define KEYBOARD_INPUT_BUFFER_BUSY 0x02
#define KEYBOARD_INPUT_BUFFER_IDLE 0x00

#define KEYBOARD_OUTPUT_BUFFER_FULL 0x01
#define KEYBOARD_OUTPUT_BUFFER_EMPTY 0x00

#define KEYBOARD_ENABLE 0xae
#define KEYBOARD_PS2_EXPECT 0xd4
#define KEYBOARD_ENABLE_SCAN 0xf4
#define MOUSE_DISABLE 0xa7

// PS/2 controller commands
#define KEYBOARD_READ_CONTROLLER_CONFIG 0x20
#define KEYBOARD_WRITE_CONTROLLER_CONFIG 0x60

// PS/2 controller configuration byte
#define KEYBOARD_CONFIG_ENABLE_IRQ1 0x01
#define KEYBOARD_CONFIG_ENABLE_IRQ12 0x02
#define KEYBOARD_CONFIG_DISABLE_FIRST_PORT 0x10
#define KEYBOARD_CONFIG_ENABLE_TRANSLATION 0x40

// Scan-code set 1 values
#define SCANCODE_EXTENDED_PREFIX 0xe0
#define SCANCODE_EXTENDED_PAUSE_PREFIX 0xe1
#define SCANCODE_RELEASE_BIT 0x80
#define SCANCODE_KEY_CODE_MASK 0x7f
#define SCANCODE_LEFT_SHIFT 0x2a
#define SCANCODE_RIGHT_SHIFT 0x36
#define SCANCODE_CAPS_LOCK 0x3a
#define SCANCODE_BYTE_MASK 0xff

void init_keyboard(void);
uint8_t keypress_buffer_is_full(void);
uint8_t keypress_buffer_is_empty(void);
void keypress_buffer_enqueue(uint32_t scan_code);
int keypress_buffer_dequeue(void);
char keyboard_get_char(void);

#endif // KEYBOARD_H
