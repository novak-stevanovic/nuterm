#include <stddef.h>
#include <stdint.h>

static void _show_cursor();
static void _hide_cursor();
static void _move_cursor(size_t x, size_t y);

static void _set_fg_c8(uint8_t color_code);
static void _set_fg_c256(uint8_t color_code);
static void _set_fg_tc(uint8_t red, uint8_t green, uint8_t blue);

static void _set_bg_c8(uint8_t color_code);
static void _set_bg_c256(uint8_t color_code);
static void _set_bg_tc(uint8_t red, uint8_t green, uint8_t blue);

static void _set_style_bold();
static void _set_style_faint();
static void _set_style_italic();
static void _set_style_underline();
static void _set_style_blink();
static void _set_style_reverse();
static void _set_style_hidden();
static void _set_style_strikethrough();

static void _reset_gfx();

static void _erase_screen();
static void erase_scrollback();
static void erase_line();

static void enter_alt_buff();
static void exit_alt_buff();
