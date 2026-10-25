#include "drivers/keyboard.h"
#include "utils/types.h"
#include "utils/ports.h"

static uint8_t KEYPRESS_BUFFER[MAX_KEYPRESS_BUFFER_SIZE];
static volatile uint8_t KEYPRESS_BUFFER_FRONT = 0;
static volatile uint8_t KEYPRESS_BUFFER_REAR = 0;

static const char scancode_set1_map[SCANCODE_SET1_TABLE_SIZE] = {
    0,   27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,   '*',
    0,   ' ',
};

static const char scancode_set1_shift_map[SCANCODE_SET1_TABLE_SIZE] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '{', '}', 0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   ':', '"', '~',
    0,   '|', 0,   0,   0,   0,   0,   0,   0,   '<', '>', '?',
};

static uint8_t shift_left, shift_right, caps_lock, extended;

/// @brief Get ascii? char from scancode
/// @param scan The scancode byte
/// @return The ascii? char
static char scan_code_to_char(uint8_t scan) {
    // TODO: Determine if chars returned are ascii
    if (scan == SCANCODE_EXTENDED_PREFIX || scan == SCANCODE_EXTENDED_PAUSE_PREFIX) {
        extended = 1;
        return 0;
    }
    if (extended) {
        // TODO: Add extended key handling
        extended = 0;
        return 0;
    }
    uint8_t released = scan & SCANCODE_RELEASE_BIT;
    scan &= SCANCODE_KEY_CODE_MASK;
    if (scan == SCANCODE_LEFT_SHIFT) { shift_left = !released; return 0; }
    if (scan == SCANCODE_RIGHT_SHIFT) { shift_right = !released; return 0; }
    if (released) return 0;
    if (scan == SCANCODE_CAPS_LOCK) { caps_lock ^= 1; return 0; }
    char c = scancode_set1_map[scan];
    uint8_t shift = shift_left || shift_right;
    if (c >= 'a' && c <= 'z') {
        if (shift ^ caps_lock) c -= 'a' - 'A';
    } else if (shift && scancode_set1_shift_map[scan]) {
        c = scancode_set1_shift_map[scan];
    }
    return c;
}

/// @brief Wait until PS/2 controller can receive another command from kernel
static inline void keyboard_wait_free_input_buffer(void) {
    while (read_from_port(KEYBOARD_STATUS_PORT) & KEYBOARD_INPUT_BUFFER_BUSY); 
}

/// @brief Discard unread bytes sent by the PS/2 controller
static inline void keyboard_flush_output_buffer(void) {
    while (read_from_port(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTPUT_BUFFER_FULL) {
        (void)read_from_port(KEYBOARD_DATA_PORT);
    }
}

void disable_mouse(void) {
    keyboard_wait_free_input_buffer();
    write_to_port(KEYBOARD_STATUS_PORT, MOUSE_DISABLE);
}

/// @brief Enable keyboard and disable mouse in PS/2 controller
void init_keyboard(void) {
    disable_mouse();

    keyboard_wait_free_input_buffer();
    write_to_port(KEYBOARD_STATUS_PORT, KEYBOARD_ENABLE);

    keyboard_flush_output_buffer();
    // Enable IRQ1 and enable set-2 to set-1 translation
    // cause i dont want the set-2 state machine stuff
    keyboard_wait_free_input_buffer();
    write_to_port(KEYBOARD_STATUS_PORT, KEYBOARD_READ_CONTROLLER_CONFIG);
    while (!(read_from_port(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTPUT_BUFFER_FULL)) {}
    uint8_t config = read_from_port(KEYBOARD_DATA_PORT);
    config |= KEYBOARD_CONFIG_ENABLE_IRQ1 | KEYBOARD_CONFIG_ENABLE_TRANSLATION;
    config &= ~(KEYBOARD_CONFIG_ENABLE_IRQ12 | KEYBOARD_CONFIG_DISABLE_FIRST_PORT);
    keyboard_wait_free_input_buffer();
    write_to_port(KEYBOARD_STATUS_PORT, KEYBOARD_WRITE_CONTROLLER_CONFIG);
    keyboard_wait_free_input_buffer();
    write_to_port(KEYBOARD_DATA_PORT, config);

    keyboard_wait_free_input_buffer();
    write_to_port(KEYBOARD_DATA_PORT, KEYBOARD_ENABLE_SCAN);

    while (!(read_from_port(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTPUT_BUFFER_FULL));
    (void)read_from_port(KEYBOARD_DATA_PORT);
    keyboard_flush_output_buffer();
}

static inline uint8_t keypress_buffer_increment_idx(uint8_t idx) {
    return (idx + 1) % MAX_KEYPRESS_BUFFER_SIZE;
}

uint8_t keypress_buffer_is_full(void) {
    return (keypress_buffer_increment_idx(KEYPRESS_BUFFER_REAR) == KEYPRESS_BUFFER_FRONT);
}

uint8_t keypress_buffer_is_empty(void) {
    return KEYPRESS_BUFFER_FRONT == KEYPRESS_BUFFER_REAR;
}

void keypress_buffer_enqueue(uint32_t scan_code) {
    uint8_t scan = scan_code & SCANCODE_BYTE_MASK;
    if (keypress_buffer_is_full()) KEYPRESS_BUFFER_FRONT = keypress_buffer_increment_idx(KEYPRESS_BUFFER_FRONT);
    KEYPRESS_BUFFER[KEYPRESS_BUFFER_REAR] = scan;
    KEYPRESS_BUFFER_REAR = keypress_buffer_increment_idx(KEYPRESS_BUFFER_REAR);
}

int keypress_buffer_dequeue(void) {
    if (keypress_buffer_is_empty()) {
        return -1;
    } else {
        int scan_code = KEYPRESS_BUFFER[KEYPRESS_BUFFER_FRONT];
        KEYPRESS_BUFFER_FRONT = keypress_buffer_increment_idx(KEYPRESS_BUFFER_FRONT);
        return scan_code;
    }
}

/// @brief Entrypoint to keyboard, atomic
/// @return The char of key pressed
/// @note Atomic as in while checking can not be interrupted
char keyboard_get_char(void) {
    for (;;) {
        __asm__ volatile ("cli" ::: "memory"); // "memory" to prevent compiler optimiztions
        int scan = keypress_buffer_dequeue();
        if (scan == -1) {
            __asm__ volatile ("sti; hlt" ::: "memory"); // hlt so we dont continuosly check empty queue
            continue;
        }
        __asm__ volatile ("sti" ::: "memory");
        char c = scan_code_to_char((uint8_t)scan);
        if (c) return c;
    }
}
