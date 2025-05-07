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

#define NT_ESC_KEY_COUNT 25

struct nt_term
{
    struct nt_esc_input_code keys[NT_ESC_KEY_COUNT];

    void (*show_cursor_func)();
    void (*hide_cursor_func)();
    void (*move_cursor_func)(size_t x, size_t y);

    void (*set_fg_c8_func)(uint8_t code);
    void (*set_fg_c256_func)(uint8_t code);
    void (*set_fg_rgb_func)(uint8_t r, uint8_t g, uint8_t b);

    void (*set_bg_c8_func)(uint8_t code);
    void (*set_bg_c256_func)(uint8_t code);
    void (*set_bg_rgb_func)(uint8_t r, uint8_t g, uint8_t b);

    void (*set_style_bold_func)();
    void (*set_style_faint_func)();
    void (*set_style_italic_func)();
    void (*set_style_underline_func)();
    void (*set_style_blink_func)();
    void (*set_style_reverse_func)();
    void (*set_style_hidden_func)();
    void (*set_style_strikethrough_func)();

    void (*reset_gfx_func)();

    void (*erase_screen_func)();
    void (*erase_scrollback_func)();
    void (*erase_line_func)();

    void (*enter_alt_buff_func)();
    void (*exit_alt_buff_func)();
};

void nt_terms_init();

const struct nt_term* nt_terms_get_active();

void nt_terms_destroy();

#endif // __NT_TERMS_H__
