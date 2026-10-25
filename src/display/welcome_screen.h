#ifndef WELCOME_SCREEN_H
#define WELCOME_SCREEN_H

#include "utils/types.h"
#include "display/vga.h"

static inline void display_welcome_screen(void) {
	uint32_t curr_vga_row = 0;
	uint32_t curr_vga_col = 0;

	uint32_t evernight_logo_row_offset = 2;
	uint32_t evernight_logo_col_offset = 19;

	static const char evernight_logo[] = "\
                   *\n\
                   **\n\
           ***     **     **\n\
             ***  ***** *\n\
           **** *********  **\n\
     *** ** **  **********   *  *\n\
        **  **  ***********   ** *\n\
       **   **** ******** *    *\n\
       **    **     ** *  *  ** *\n\
    ** ***            *  *    * ***\n\
     * *****       **  **  *  * * *\n\
         **********  ***  *  ** *\n\
        ****       **** **   * *\n\
      ****     * ****  **   *****\n\
       *  ** ** ***** **    **\n\
            ** *****  ***\n\
            *   ****  **  **\n\
                  ****\n\
                   *\n";


    uint8_t vga_red_on_black = vga_color_encode(VGA_COLOR_RED, VGA_COLOR_BLACK);
    uint8_t vga_black_on_red = vga_color_encode(VGA_COLOR_BLACK, VGA_COLOR_RED);

	clear_screen();
	curr_vga_row += evernight_logo_row_offset;
	display_string_col_offset(evernight_logo, sizeof(evernight_logo) - 1, &curr_vga_row, &curr_vga_col, vga_red_on_black, evernight_logo_col_offset); // red on black

    // Top border
    curr_vga_row = 0; curr_vga_col = 0;
    for (uint32_t i = 0; i < VGA_WIDTH; i++) {
        display_char_col_offset(' ', &curr_vga_row, &curr_vga_col, vga_black_on_red, 0);
    }

    // Bottom border
    curr_vga_row = VGA_HEIGHT - 1; curr_vga_col = 0;
    for (uint32_t i = 0; i < VGA_WIDTH; i++) {
        display_char_col_offset(' ', &curr_vga_row, &curr_vga_col, vga_black_on_red, 0);
    }

    // Left and right borders
    for (curr_vga_row = 1; curr_vga_row < VGA_HEIGHT - 1; curr_vga_row++) {
        curr_vga_col = 0;
        display_char_col_offset(' ', &curr_vga_row, &curr_vga_col, vga_black_on_red, 0);             // left edge
        curr_vga_col = VGA_WIDTH - 1;
        display_char_col_offset(' ', &curr_vga_row, &curr_vga_col, vga_black_on_red, 0);             // right edge
    }

    // Overwrite with footer
    static const char footer[] = " Welcome to EvernightOS ";
    curr_vga_row = VGA_HEIGHT - 1; curr_vga_col = 0;
    display_string_col_offset(footer, sizeof(footer) - 1, &curr_vga_row, &curr_vga_col, vga_black_on_red, (VGA_WIDTH - (sizeof(footer) - 1)) / 2);

    // Overwrite with press enter
    static const char press_key[] = " Press Enter key to continue... ";
    curr_vga_row = 0; curr_vga_col = 0;
    display_string_col_offset(press_key, sizeof(press_key) - 1, &curr_vga_row, &curr_vga_col, vga_red_on_black, (VGA_WIDTH - (sizeof(press_key) - 1)) / 2);

    set_cursor_position(curr_vga_row, curr_vga_col);
}

#endif // WELCOME_SCREEN_H
