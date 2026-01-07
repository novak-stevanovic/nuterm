#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nt_internal.h"

static nt_term_color_count _color = NT_TERM_COLOR_OTHER;
static struct nt_term_info _term = {0};

static char* xterm_esc_key_seqs[] = {
    // F keys
    "\x1b\x4f\x50", "\x1b\x4f\x51", "\x1b\x4f\x52", "\x1b\x4f\x53",
    "\x1b\x5b\x31\x35\x7e", "\x1b\x5b\x31\x37\x7e", "\x1b\x5b\x31\x38\x7e",
    "\x1b\x5b\x31\x39\x7e", "\x1b\x5b\x32\x30\x7e", "\x1b\x5b\x32\x31\x7e",
    "\x1b\x5b\x32\x33\x7e", "\x1b\x5b\x32\x34\x7e",

    // Arrow keys
    "\x1b\x5b\x41", "\x1b\x5b\x43", "\x1b\x5b\x42", "\x1b\x5b\x44",

    // INS, DEL...
    "\x1b\x5b\x32\x7e", "\x1b\x5b\x33\x7e",
    "\x1b\x5b\x48", "\x1b\x5b\x46",
    "\x1b\x5b\x35\x7e", "\x1b\x5b\x36\x7e",

    // STAB
    "\x1b\x5b\x5a"
};

static char* xterm_esc_func_seqs[] = {
    // Show/hide/move cursor
    "\x1b[?25h", "\x1b[?25l", "\x1b[%d;%dH",

    // FG(c8, c256, tc, reset)
    "\x1b[3%dm", "\x1b[38;5;%dm", "\x1b[38;2;%d;%d;%dm", "\x1b[39m",

    // BG(c8, c256, tc, reset)
    "\x1b[4%dm", "\x1b[48;5;%dm", "\x1b[48;2;%d;%d;%dm", "\x1b[49m",

    // Style funcs
    "\x1b[1m", "\x1b[2m", "\x1b[3m", "\x1b[4m",
    "\x1b[5m", "\x1b[7m", "\x1b[8m", "\x1b[9m",

    // Reset GFX
    "\x1b[m\017",

    // Erase
    "\x1b[2J", "\x1b[3J", "\x1b[2K",

    // Alt buffer
    "\x1b[?1049h", "\x1b[?1049l",

    "\x1b[?1006h\x1b[?1000h",
    "\x1b[?1006l\x1b[?1000l",
};

static char* rxvt_esc_key_seqs[] = {
    // F keys
    "\x1b\x5b\x31\x31\x7e", "\x1b\x5b\x31\x32\x7e", "\x1b\x5b\x31\x33\x7e",
    "\x1b\x5b\x31\x34\x7e", "\x1b\x5b\x31\x35\x7e", "\x1b\x5b\x31\x37\x7e",
    "\x1b\x5b\x31\x38\x7e", "\x1b\x5b\x31\x39\x7e", "\x1b\x5b\x32\x30\x7e",
    "\x1b\x5b\x32\x31\x7e", "\x1b\x5b\x32\x33\x7e", "\x1b\x5b\x32\x34\x7e",

    // Arrow keys
    "\x1b\x5b\x41", "\x1b\x5b\x43", "\x1b\x5b\x42", "\x1b\x5b\x44",

    // INS, DEL...
    "\x1b\x5b\x32\x7e", "\x1b\x5b\x33\x7e",
    "\x1b\x5b\x37\x7e", "\x1b\x5b\x38\x7e",
    "\x1b\x5b\x35\x7e", "\x1b\x5b\x36\x7e",

    // STAB
    "\x1b\x5b\x5a"
};

static char* rxvt_esc_func_seqs[] = {
    // Show/hide/move cursor
    "\x1b[?25h", "\x1b[?25l", "\x1b[%d;%dH",

    // FG(c8, c256, tc, reset)
    "\x1b[3%dm", "\x1b[38;5;%dm", NULL, "\x1b[39m",

    // BG(c8, c256, tc, reset)
    "\x1b[4%dm", "\x1b[48;5;%dm", NULL, "\x1b[49m",

    // Style funcs
    "\x1b[1m", "\x1b[2m", "\x1b[3m", "\x1b[4m",
    NULL, "\x1b[7m", NULL, NULL,

    // Reset GFX
    "\x1b(B\x1b[m",

    // Erase
    "\x1b[2J", NULL, "\x1b[2K",

    // Alt buffer
    "\x1b[?1049h", "\x1b[?1049l",

    "\x1b[?1006h\x1b[?1000h",
    "\x1b[?1006l\x1b[?1000l",
};

static char* alacritty_esc_key_seqs[] = {
    // F keys
    "\x1b\x4f\x50", "\x1b\x4f\x51", "\x1b\x4f\x52", "\x1b\x4f\x53",
    "\x1b\x5b\x31\x35\x7e", "\x1b\x5b\x31\x37\x7e", "\x1b\x5b\x31\x38\x7e",
    "\x1b\x5b\x31\x39\x7e", "\x1b\x5b\x32\x30\x7e", "\x1b\x5b\x32\x31\x7e",
    "\x1b\x5b\x32\x33\x7e", "\x1b\x5b\x32\x34\x7e",

    // Arrow keys
    "\x1b\x5b\x41", "\x1b\x5b\x43", "\x1b\x5b\x42", "\x1b\x5b\x44",

    // INS, DEL...
    "\x1b\x5b\x32\x7e", "\x1b\x5b\x33\x7e",
    "\x1b\x5b\x48", "\x1b\x5b\x46",
    "\x1b\x5b\x35\x7e", "\x1b\x5b\x36\x7e",

    // STAB
    "\x1b\x5b\x5a"
};

static char* alacritty_esc_func_seqs[] = {
    // Show/hide/move cursor
    "\x1b[?25h", "\x1b[?25l", "\x1b[%d;%dH",

    // FG(c8, c256, tc, reset)
    "\x1b[3%dm", "\x1b[38;5;%dm", "\x1b[38;2;%d;%d;%dm", "\x1b[39m",

    // BG(c8, c256, tc, reset)
    "\x1b[4%dm", "\x1b[48;5;%dm", "\x1b[48;2;%d;%d;%dm", "\x1b[49m",

    // Style funcs
    "\x1b[1m", "\x1b[2m", "\x1b[3m", "\x1b[4m",
    NULL, "\x1b[7m", "\x1b[8m", "\x1b[9m",

    // Reset GFX
    "\x1b[m\017",

    // Erase
    "\x1b[2J", "\x1b[3J", "\x1b[2K",

    // Alt buffer
    "\x1b[?1049h", "\x1b[?1049l",

    "\x1b[?1006h\x1b[?1000h",
    "\x1b[?1006l\x1b[?1000l",
};

static char* tmux_esc_key_seqs[] = {
    // F keys
    "\x1b\x4f\x50",      // F1  kf1
    "\x1b\x4f\x51",      // F2  kf2
    "\x1b\x4f\x52",      // F3  kf3
    "\x1b\x4f\x53",      // F4  kf4
    "\x1b\x5b\x31\x35\x7e", // F5
    "\x1b\x5b\x31\x37\x7e", // F6
    "\x1b\x5b\x31\x38\x7e", // F7
    "\x1b\x5b\x31\x39\x7e", // F8
    "\x1b\x5b\x32\x30\x7e", // F9
    "\x1b\x5b\x32\x31\x7e", // F10
    "\x1b\x5b\x32\x33\x7e", // F11
    "\x1b\x5b\x32\x34\x7e", // F12

    // Arrow keys (application cursor mode!)
    "\x1b\x4f\x41",      // UP    kcuu1
    "\x1b\x4f\x43",      // RIGHT kcuf1
    "\x1b\x4f\x42",      // DOWN  kcud1
    "\x1b\x4f\x44",      // LEFT  kcub1

    // INS, DEL...
    "\x1b\x5b\x32\x7e",  // INSERT kich1
    "\x1b\x5b\x33\x7e",  // DELETE kdch1
    "\x1b\x5b\x31\x7e",  // HOME  khome
    "\x1b\x5b\x34\x7e",  // END   kend
    "\x1b\x5b\x35\x7e",  // PG UP kpp
    "\x1b\x5b\x36\x7e",  // PG DN knp

    // STAB (Shift-Tab)
    "\x1b\x5b\x5a"       // kcbt
};

static char* tmux_esc_func_seqs[] = {
    // Cursor
    "\x1b[?25h",             // CURSOR_SHOW (cnorm simplified)
    "\x1b[?25l",             // CURSOR_HIDE (civis)
    "\x1b[%d;%dH",           // CURSOR_MOVE

    // FG
    "\x1b[3%dm",             // FG_SET_C8
    "\x1b[38;5;%dm",         // FG_SET_C256
    "\x1b[38;2;%d;%d;%dm",   // FG_SET_RGB
    "\x1b[39m",              // FG_SET_DEFAULT

    // BG
    "\x1b[4%dm",             // BG_SET_C8
    "\x1b[48;5;%dm",         // BG_SET_C256
    "\x1b[48;2;%d;%d;%dm",   // BG_SET_RGB
    "\x1b[49m",              // BG_SET_DEFAULT

    // Styles
    "\x1b[1m",               // BOLD
    "\x1b[2m",               // FAINT
    "\x1b[3m",               // ITALIC
    "\x1b[4m",               // UNDERLINE
    "\x1b[5m",               // BLINK
    "\x1b[7m",               // REVERSE
    "\x1b[8m",               // HIDDEN
    "\x1b[9m",               // STRIKETHROUGH

    // Reset
    "\x1b[m\017",            // GFX_RESET (sgr0)

    // Erase
    "\x1b[2J",               // ERASE_SCREEN
    "\x1b[3J",               // ERASE_SCROLLBACK
    "\x1b[2K",               // ERASE_LINE

    // Alt buffer
    "\x1b[?1049h",           // ALT_BUFF_ENTER
    "\x1b[?1049l",           // ALT_BUFF_EXIT

    "\x1b[?1006h\x1b[?1000h",
    "\x1b[?1006l\x1b[?1000l",
};

static struct nt_term_info terms[] = {
    { 
        .esc_key_seqs = xterm_esc_key_seqs,
        .esc_func_seqs = xterm_esc_func_seqs,
        .name = "xterm"
    },
    { 
        .esc_key_seqs = rxvt_esc_key_seqs,
        .esc_func_seqs = rxvt_esc_func_seqs,
        .name = "rxvt"
    },
    { 
        .esc_key_seqs = alacritty_esc_key_seqs,
        .esc_func_seqs = alacritty_esc_func_seqs,
        .name = "alacritty"
    },
    {
        .esc_key_seqs = tmux_esc_key_seqs,
        .esc_func_seqs = tmux_esc_func_seqs,
        .name = "tmux"
    }
};

void _nt_term_init(nt_status* out_status)
{
    char* env_term = getenv("TERM");
    char* env_colorterm = getenv("COLORTERM");

    if(env_term == NULL)
    {
        SET_OUT(out_status, NT_ERR_INIT_TERM_ENV);
        return;
    }

    size_t i;
    bool found = false;
    for(i = 0; i < sizeof(terms) / sizeof(struct nt_term_info); i++)
    {
        if(strstr(env_term, terms[i].name) != NULL)
        {
            _term = terms[i];
            found = true;
            break;
        }
    }

    if(!found)
    {
        _term = terms[0]; // Assume emulator is compatible with xterm
    }

    if((env_colorterm != NULL) && (strstr(env_colorterm, "truecolor")))
        _color = NT_TERM_COLOR_TC;
    else
    {
        if(strstr(env_term, "256"))
            _color = NT_TERM_COLOR_C256;
        else
            _color = NT_TERM_COLOR_C8;
    }

    nt_status ret = found ? NT_SUCCESS : NT_ERR_TERM_NOT_SUPP;
    SET_OUT(out_status, ret);
}

struct nt_term_info _nt_term_get_used()
{
    return _term;
}

nt_term_color_count _nt_term_get_color_count()
{
    return _color;
}

void _nt_term_deinit()
{
    _color = NT_TERM_COLOR_OTHER;
    _term = (struct nt_term_info) {0};
}
