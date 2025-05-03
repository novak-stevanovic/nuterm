#ifndef _NT_ANSI_H_
#define _NT_ANSI_H_

#include <stddef.h>
#include <stdint.h>

/* Cursor ------------------------------------------------------------------- */

void _nt_ansi_cursor_move_home();
void _nt_ansi_cursor_move_to_line_col(size_t line, size_t col);
void _nt_ansi_cursor_move_up(size_t row_diff);
void _nt_ansi_cursor_move_down(size_t row_diff);
void _nt_ansi_cursor_move_right(size_t col_diff);
void _nt_ansi_cursor_move_left(size_t col_diff);
void _nt_ansi_cursor_move_to_col(size_t col);

void _nt_ansi_cursor_hide();
void _nt_ansi_cursor_show();

/* Screen ------------------------------------------------------------------- */

void _nt_ansi_screen_erase_screen();
void _nt_ansi_screen_erase_scrollback_buffer();
void _nt_ansi_screen_erase_line();
void _nt_ansi_screen_enable_alt_buffer();
void _nt_ansi_screen_disable_alt_buffer();

/* GFX ---------------------------------------------------------------------- */

#define _NT_ANSI_GFX_STYLE_RESET 0
#define _NT_ANSI_GFX_STYLE_BOLD 1
#define _NT_ANSI_GFX_STYLE_FAINT 2
#define _NT_ANSI_GFX_STYLE_ITALIC 3
#define _NT_ANSI_GFX_STYLE_UNDERLINE 4
#define _NT_ANSI_GFX_STYLE_BLINK 5
#define _NT_ANSI_GFX_STYLE_REVERSE 7
#define _NT_ANSI_GFX_STYLE_HIDDEN 8
#define _NT_ANSI_GFX_STYLE_STRIKETHROUGH 9

void _nt_ansi_gfx_reset();

void _nt_ansi_gfx_set_style(size_t style_code);
// void _nt_ansi_gfx_reset_style();

void _nt_ansi_gfx_set_fg_color_c8(uint8_t color_code);
void _nt_ansi_gfx_set_bg_color_c8(uint8_t color_code);
void _nt_ansi_gfx_set_fg_color_default();
void _nt_ansi_gfx_set_bg_color_default();

void _nt_ansi_gfx_set_fg_color_c256(uint8_t color_code);
void _nt_ansi_gfx_set_bg_color_c256(uint8_t color_code);

// True-color
void _nt_ansi_gfx_set_fg_color_tc(uint8_t red, uint8_t green, uint8_t blue);
void _nt_ansi_gfx_set_bg_color_tc(uint8_t red, uint8_t green, uint8_t blue);

#endif // _NT_ANSI_H_
