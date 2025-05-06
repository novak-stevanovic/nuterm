#ifndef __NT_TERM_H__
#define __NT_TERM_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct nt_key
{
    size_t id;
    uint8_t seq[20];
};

#define NT_KEY_COUNT 25

struct nt_term
{
    struct nt_key keys[NT_KEY_COUNT];

    void (*show_cursor_func)();
    void (*hide_cursor_func)();
    void (*move_cursor_func)();

    void (*set_fg_ansi_func)(uint8_t code);
    void (*set_fg_aixterm_func)(uint8_t code);
    void (*set_fg_rgb_func)(uint8_t r, uint8_t g, uint8_t b);

    void (*set_bg_ansi_func)(uint8_t code);
    void (*set_bg_aixterm_func)(uint8_t code);
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
    void (*erase_line_func)();

    void (*enter_alt_buff_func)();
    void (*exit_alt_buff_func)();
};

void nt_term_init();

void nt_term_destroy();

#endif // __NT_TERM_H__
