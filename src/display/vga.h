#ifndef VGA_H
#define VGA_H

#include "utils/types.h"

#define VGA_MEMORY ((volatile char*)0xb8000)
#define VGA_WIDTH ((uint32_t)80)
#define VGA_HEIGHT ((uint32_t)25)

#define CRTC_ADDRESS_PORT ((uint16_t)0x3d4)
#define CRTC_DATA_PORT ((uint16_t)0x3d5)

typedef enum {
    VGA_COLOR_BLACK        = 0,
    VGA_COLOR_BLUE         = 1,
    VGA_COLOR_GREEN        = 2,
    VGA_COLOR_CYAN         = 3,
    VGA_COLOR_RED          = 4,
    VGA_COLOR_MAGENTA      = 5,
    VGA_COLOR_BROWN        = 6,
    VGA_COLOR_LIGHT_GREY   = 7,
    VGA_COLOR_DARK_GREY    = 8,
    VGA_COLOR_LIGHT_BLUE   = 9,
    VGA_COLOR_LIGHT_GREEN  = 10,
    VGA_COLOR_LIGHT_CYAN   = 11,
    VGA_COLOR_LIGHT_RED    = 12,
    VGA_COLOR_LIGHT_MAGENTA= 13,
    VGA_COLOR_LIGHT_BROWN  = 14,
    VGA_COLOR_WHITE        = 15,
} vga_color_t;

static inline uint8_t vga_color_encode(vga_color_t foreground, vga_color_t background) {
    uint8_t msb_shift = 4; // background uses the most significant 4 bits
    uint8_t lsb_mask = 0x0f; // foreground uses the least significant 4 bits
    return (uint8_t)((background << msb_shift) | (foreground & lsb_mask));
}

uint32_t get_vga_linear_idx(uint32_t row, uint32_t col);

void set_cursor_position(uint16_t row, uint16_t col);
void clear_screen(void);

void display_char_col_offset(uint8_t c, uint32_t* row, uint32_t* col, uint8_t color, uint32_t col_offset);
void display_string_col_offset(const char* str, uint32_t length, uint32_t* row, uint32_t* col, uint8_t color, uint32_t col_offset);
void display_string(const char* str, uint32_t length, uint32_t* row, uint32_t* col, uint8_t color);

#endif // VGA_H
