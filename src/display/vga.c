#include "display/vga.h"
#include "utils/ports.h"

uint32_t get_vga_linear_idx(uint32_t row, uint32_t col) {
	return (row * VGA_WIDTH + col) * 2; // multiplied by 2 since each VGA cell has 2 bytes
}

void set_cursor_position(uint16_t row, uint16_t col) {
	uint16_t pos = get_vga_linear_idx(row, col) / 2; // divide by 2 since each VGA cell has 2 bytes

	uint16_t cursor_location_lower_byte_register_idx = 0x0f;
	uint16_t cursor_location_high_byte_register_idx = 0x0e;

	uint8_t get_lower_8_bit_mask = 0xff;
	uint16_t right_shift_to_get_higher_8_bits = 8;


	// Send the low byte
	outb(CRTC_ADDRESS_PORT, cursor_location_lower_byte_register_idx);
	outb(CRTC_DATA_PORT, pos & get_lower_8_bit_mask);

	// Send the high byte
	outb(CRTC_ADDRESS_PORT, cursor_location_high_byte_register_idx);
	outb(CRTC_DATA_PORT, (pos >> right_shift_to_get_higher_8_bits) & get_lower_8_bit_mask);
}

void clear_screen(void) {
	uint32_t idx;
	uint8_t color = vga_color_encode(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	for (uint32_t y = 0; y < VGA_HEIGHT; y++) {
		for (uint32_t x = 0; x < VGA_WIDTH; x++) {
			idx = get_vga_linear_idx(y, x);
			VGA_MEMORY[idx] = ' ';
			VGA_MEMORY[idx + 1] = color;
		}
	}
}

void display_char_col_offset(uint8_t c, uint32_t* row, uint32_t* col, uint8_t color, uint32_t col_offset) {
	*col += col_offset;
	if (c == '\n') {
		(*row)++;
		(*col) = col_offset;
		return;
	}
	uint32_t idx = get_vga_linear_idx(*row, *col);
	VGA_MEMORY[idx] = c;
	VGA_MEMORY[idx + 1] = color;
	(*col)++;
}

void display_string_col_offset(const string_t* str, uint32_t* row, uint32_t* col, uint8_t color, uint32_t col_offset) {
	*col += col_offset;
	for (uint32_t i = 0; i < str->length; i++) {
        char c = str->str[i];
        if (c == '\n') {
            (*row)++;
            (*col) = col_offset;
            continue;
        }
        uint32_t idx = get_vga_linear_idx(*row, *col);
        VGA_MEMORY[idx] = c;
        VGA_MEMORY[idx + 1] = color;
        (*col)++;
    }
}

void display_string(const string_t* str, uint32_t* row, uint32_t* col, uint8_t color) {
    display_string_col_offset(str, row, col, color, 0);
}
