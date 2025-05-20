/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_nt_term.h"
#include "_nt_shared.h"

static nt_term_color_count_t _color = NT_TERM_COLOR_OTHER;
static struct nt_term_info* _term = NULL;

static char* _xterm_esc_key_seqs[] = {
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

static char* _xterm_esc_func_seqs[] = {
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
    "\x1b[?1049h", "\x1b[?1049l"
};

static char* _rxvt_esc_key_seqs[] = {
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

static char* _rxvt_esc_func_seqs[] = {
    // Show/hide/move cursor
    "\x1b[?25h", "\x1b[?25l", "\x1b[%d;%dH",

    // FG(c8, c256, tc, reset)
    "\x1b[3%dm", "\x1b[38;5;%dm", NULL, "\x1b[39m",

    // BG(c8, c256, tc, reset)
    "\x1b[4%dm", "\x1b[48;5;%dm", NULL, "\x1b[49m",

    // Style funcs - TODO: check FAINT support
    "\x1b[1m", "\x1b[2m", "\x1b[3m", "\x1b[4m",
    NULL, "\x1b[7m", NULL, NULL,

    // Reset GFX
    "\x1b(B\x1b[m",

    // Erase
    "\x1b[2J", NULL, "\x1b[2K",

    // Alt buffer
    "\x1b[?1049h", "\x1b[?1049l"
};

static char* _alacritty_esc_key_seqs[] = {
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

static char* _alacritty_esc_func_seqs[] = {
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
    "\x1b[?1049h", "\x1b[?1049l"
};

static struct nt_term_info _terms[] = {
    { 
        .esc_key_seqs = _xterm_esc_key_seqs,
        .esc_func_seqs = _xterm_esc_func_seqs,
        .name = "xterm"
    },
    { 
        .esc_key_seqs = _rxvt_esc_key_seqs,
        .esc_func_seqs = _rxvt_esc_func_seqs,
        .name = "rxvt"
    },
    { 
        .esc_key_seqs = _alacritty_esc_key_seqs,
        .esc_func_seqs = _alacritty_esc_func_seqs,
        .name = "alacritty"
    }
};

void nt_term_init(nt_status_t* out_status)
{
    char* env_term = getenv("TERM");
    char* env_colorterm = getenv("COLORTERM");

    if(env_term == NULL)
    {
        _vreturn(out_status, NT_ERR_INIT_TERM_ENV);
    }

    size_t i;
    bool found = false;
    for(i = 0; i < sizeof(_terms) / sizeof(struct nt_term_info); i++)
    {
        if(strstr(env_term, _terms[i].name) != NULL)
        {
            _term = &_terms[i];
            found = true;
            break;
        }
    }

    if(!found)
    {
        _term = &_terms[0]; // Assume emulator is compatible with xterm
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

    nt_status_t ret = found ? NT_SUCCESS : NT_ERR_TERM_NOT_SUPPORTED;
    _vreturn(out_status, ret);
}

const struct nt_term_info* nt_term_get_used()
{
    return _term;
}

nt_term_color_count_t nt_term_get_color_count()
{
    return _color;
}

void nt_term_destroy()
{
    _color = NT_TERM_COLOR_OTHER;
    _term = NULL;
}
