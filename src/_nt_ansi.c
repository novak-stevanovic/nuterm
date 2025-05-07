// TODO: Merge into src/_xterm.c

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#define CODE_BUFFER_SIZE 30

static char code_buffer[CODE_BUFFER_SIZE];

#define NT_PRIM_PRINT(fmt, ...)                                                \
{                                                                              \
    snprintf(code_buffer, CODE_BUFFER_SIZE, fmt, ##__VA_ARGS__);               \
    printf("%s", code_buffer);                                                 \
}

#define CURSOR_MOVE_HOME_CODE "\e[H"
#define CURSOR_MOVE_LINE_COL_CODE "\e[%zu;%zuH"
#define CURSOR_MOVE_UP_CODE "\e[%zuA"
#define CURSOR_MOVE_DOWN_CODE "\e[%zuB"
#define CURSOR_MOVE_RIGHT_CODE "\e[%zuC"
#define CURSOR_MOVE_LEFT_CODE "\e[%zuD"
#define CURSOR_MOVE_TO_COLUMN_CODE "\e[%zuG"
#define CURSOR_HIDE_CODE "\e[?25l"
#define CURSOR_SHOW_CODE "\e[?25h"

/* Cursor ------------------------------------------------------------------- */

void _nt_ansi_cursor_move_home()
{
    NT_PRIM_PRINT(CURSOR_MOVE_HOME_CODE);
}

void _nt_ansi_cursor_move_to_line_col(size_t line, size_t col)
{
    NT_PRIM_PRINT(CURSOR_MOVE_LINE_COL_CODE, line, col);
}

void _nt_ansi_cursor_move_up(size_t row_diff)
{
    NT_PRIM_PRINT(CURSOR_MOVE_UP_CODE, row_diff);
}

void _nt_ansi_cursor_move_down(size_t row_diff)
{
    NT_PRIM_PRINT(CURSOR_MOVE_DOWN_CODE, row_diff);
}

void _nt_ansi_cursor_move_right(size_t col_diff)
{
    NT_PRIM_PRINT(CURSOR_MOVE_RIGHT_CODE, col_diff);
}

void _nt_ansi_cursor_move_left(size_t col_diff)
{
    NT_PRIM_PRINT(CURSOR_MOVE_LEFT_CODE, col_diff);
}

void _nt_ansi_cursor_move_to_col(size_t col)
{
    NT_PRIM_PRINT(CURSOR_MOVE_TO_COLUMN_CODE, col);
}

void _nt_ansi_cursor_hide()
{
    NT_PRIM_PRINT(CURSOR_HIDE_CODE);
}

void _nt_ansi_cursor_show()
{
    NT_PRIM_PRINT(CURSOR_SHOW_CODE);
}

/* Screen ------------------------------------------------------------------- */

#define SCREEN_ERASE_SCREEN_CODE "\e[2J"
#define SCREEN_ERASE_SCROLLBACK_BUFFER_CODE "\e[3J"
#define SCREEN_ERASE_LINE_CODE "\e[2K"

#define SCREEN_ENABLE_ALT_BUFFER_CODE "\e[?1049h"
#define SCREEN_DISABLE_ALT_BUFFER_CODE "\e[?1049l"

void _nt_ansi_screen_erase_screen()
{
    NT_PRIM_PRINT(SCREEN_ERASE_SCREEN_CODE);
}

void _nt_ansi_screen_erase_scrollback_buffer()
{
    NT_PRIM_PRINT(SCREEN_ERASE_SCROLLBACK_BUFFER_CODE);
}

void _nt_ansi_screen_erase_line()
{
    NT_PRIM_PRINT(SCREEN_ERASE_LINE_CODE);
}

void _nt_ansi_screen_enable_alt_buffer()
{
    NT_PRIM_PRINT(SCREEN_ENABLE_ALT_BUFFER_CODE);
}

void _nt_ansi_screen_disable_alt_buffer()
{
    NT_PRIM_PRINT(SCREEN_DISABLE_ALT_BUFFER_CODE);
}

/* GFX ---------------------------------------------------------------------- */

#define GFX_RESET_CODE "\e[0m"
#define GFX_ENABLE_STYLE_CODE "\e[%zum"

#define GFX_SET_FG_COLOR_C8 "\e[3%dm"
#define GFX_SET_BG_COLOR_C8 "\e[4%dm"
#define GFX_SET_FG_COLOR_DEFAULT "\e[39m"
#define GFX_SET_BG_COLOR_DEFAULT "\e[49m"

#define GFX_SET_FG_COLOR_C16 "\e[9%dm"
#define GFX_SET_BG_COLOR_C16 "\e[10%dm"

#define GFX_SET_FG_COLOR_C256 "\e[38;5;%dm"
#define GFX_SET_BG_COLOR_C256 "\e[48;5;%dm"

#define GFX_SET_FG_COLOR_TC "\e[38;2;%d;%d;%dm"
#define GFX_SET_BG_COLOR_TC "\e[48;2;%d;%d;%dm"

void _nt_ansi_gfx_reset()
{
    NT_PRIM_PRINT(GFX_RESET_CODE);
}

void _nt_ansi_gfx_set_style(size_t style_code)
{
    NT_PRIM_PRINT(GFX_ENABLE_STYLE_CODE, style_code);
}

void _nt_ansi_gfx_set_fg_color_c8(uint8_t color_code)
{
    assert(color_code < 8);
    NT_PRIM_PRINT(GFX_SET_FG_COLOR_C8, color_code);
}

void _nt_ansi_gfx_set_bg_color_c8(uint8_t color_code)
{
    assert(color_code < 8);
    NT_PRIM_PRINT(GFX_SET_BG_COLOR_C8, color_code);
}

void _nt_ansi_gfx_set_fg_color_default()
{
    NT_PRIM_PRINT(GFX_SET_FG_COLOR_DEFAULT);
}
void _nt_ansi_gfx_set_bg_color_default()
{
    NT_PRIM_PRINT(GFX_SET_BG_COLOR_DEFAULT);
}

void _nt_ansi_gfx_set_fg_color_c16(uint8_t color_code)
{
    assert(color_code < 16);
    if(color_code < 8)
    {
        NT_PRIM_PRINT(GFX_SET_FG_COLOR_C8, color_code);
    }
    else 
    {
        NT_PRIM_PRINT(GFX_SET_FG_COLOR_C16, color_code - 8);
    }
}

void _nt_ansi_gfx_set_bg_color_c16(uint8_t color_code)
{
    assert(color_code < 16);
    if(color_code < 8) 
    {
        NT_PRIM_PRINT(GFX_SET_BG_COLOR_C8, color_code);
    }
    else 
    {
        NT_PRIM_PRINT(GFX_SET_BG_COLOR_C16, color_code - 8);
    }
}

void _nt_ansi_gfx_set_fg_color_c256(uint8_t color_code)
{
    NT_PRIM_PRINT(GFX_SET_FG_COLOR_C256, color_code);
}

void _nt_ansi_gfx_set_bg_color_c256(uint8_t color_code)
{
    NT_PRIM_PRINT(GFX_SET_BG_COLOR_C256, color_code);
}

void _nt_ansi_gfx_set_fg_color_tc(uint8_t red, uint8_t green, uint8_t blue)
{
    NT_PRIM_PRINT(GFX_SET_FG_COLOR_TC, red, green, blue);
}

void _nt_ansi_gfx_set_bg_color_tc(uint8_t red, uint8_t green, uint8_t blue)
{
    NT_PRIM_PRINT(GFX_SET_BG_COLOR_TC, red, green, blue);
}
