#ifndef __NT_TERMS_H__
#define __NT_TERMS_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct nt_esc_input_code
{
    uint32_t codepoint;
    uint8_t seq[20];
};

struct nt_term
{
    struct nt_esc_input_code* keys;

    char* show_cursor_code;
    char* hide_cursor_code;
    char* move_cursor_code;

    char* set_fg_c8_code;
    char* set_fg_c256_code;
    char* set_fg_rgb_code;

    char* set_bg_c8_code;
    char* set_bg_c256_code;
    char* set_bg_rgb_code;

    char* set_style_bold_code;
    char* set_style_faint_code;
    char* set_style_italic_code;
    char* set_style_underline_code;
    char* set_style_blink_code;
    char* set_style_reverse_code;
    char* set_style_hidden_code;
    char* set_style_strikethrough_code;

    char* reset_gfx_code;

    char* erase_screen_code;
    char* erase_scrollback_code;
    char* erase_line_code;

    char* enter_alt_buff_code;
    char* exit_alt_buff_code;
};

void nt_terms_init();

const struct nt_term* nt_terms_get_active();

void nt_terms_destroy();

#endif // __NT_TERMS_H__
