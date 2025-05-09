#include <nt_terms/_nt_terms.h>

static struct nt_term* _active = NULL;

static char _xterm_show_cursor_code[] = "";
static char _xterm_hide_cursor_code[] = "";
static char _xterm_move_cursor_code[] = "";

static char _xterm_set_fg_c8_code[] = "";
static char _xterm_set_fg_c256_code[] = "";
static char _xterm_set_fg_rgb_code[] = "";

static char _xterm_set_bg_c8_code[] = "";
static char _xterm_set_bg_c256_code[] = "";
static char _xterm_set_bg_rgb_code[] = "";

static char _xterm_set_style_bold_code[] = "";
static char _xterm_set_style_faint_code[] = "";
static char _xterm_set_style_italic_code[] = "";
static char _xterm_set_style_underline_code[] = "";
static char _xterm_set_style_blink_code[] = "";
static char _xterm_set_style_reverse_code[] = "";
static char _xterm_set_style_hidden_code[] = "";
static char _xterm_set_style_strikethrough_code[] = "";

static char _xterm_reset_gfx_code[] = "";

static char _xterm_erase_screen_code[] = "";
static char _xterm_erase_scrollback_code[] = "";
static char _xterm_erase_line_code[] = "";

static char _xterm_enter_alt_buff_code[] = "";
static char _xterm_exit_alt_buff_code[] = "";

static char _xterm_

struct nt_term xterm = {
};

void nt_terms_init()
{
    // TODO: determine active terminal
}

const struct nt_term* nt_terms_get_active()
{
    return _active;
}

void nt_terms_destroy()
{
    _active = NULL;
}
