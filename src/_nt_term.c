#include "_nt_term.h"
#include <stdint.h>
#include <stdlib.h>

static struct nt_term* _used = NULL;

static char* _xterm_esc_key_seqs[] = {
    "\x1b\x4f\x50",
    "\x1b\x4f\x51",
    "\x1b\x4f\x52",
    "\x1b\x4f\x53",
    "\x1b\x5b\x31\x35\x7e",
    "\x1b\x5b\x31\x37\x7e",
    "\x1b\x5b\x31\x38\x7e",
    "\x1b\x5b\x31\x39\x7e",
    "\x1b\x5b\x32\x30\x7e",
    "\x1b\x5b\x32\x31\x7e",
    "\x1b\x5b\x32\x33\x7e",
    "\x1b\x5b\x32\x34\x7e",

    "\x1b\x5b\x41",
    "\x1b\x5b\x43",
    "\x1b\x5b\x42",
    "\x1b\x5b\x44",

    "\x1b\x5b\x32\x7e",
    "\x1b\x5b\x33\x7e",

    "\x1b\x5b\x31\x7e",
    "\x1b\x5b\x34\x7e",

    "\x1b\x5b\x35\x7e",
    "\x1b\x5b\x36\x7e",

    "\x1b\x5b\x5a"
};

static char* _xterm_esc_func_seqs[] = {
    "\x1b[?25h",
    "\x1b[?25l",

    "\x1b[%d;%dH",

    "\x1b[3%dm",
    "\x1b[38;5;%dm",
    "\x1b[38;2;%d;%d;%dm",

    "\x1b[4%dm",
    "\x1b[48;5;%dm",
    "\x1b[48;2;%d;%d;%dm",

    "\x1b[1m",
    "\x1b[2m",
    "\x1b[3m",
    "\x1b[4m",
    "\x1b[5m",
    "\x1b[7m",
    "\x1b[8m",
    "\x1b[9m",

    "\x1b[0m",

    "\x1b[2J",
    "\x1b[3J",

    "\x1b[2K",

    "\x1b[?1049h",
    "\x1b[?1049l"
};

static struct nt_term _xterm = {
    .esc_key_seqs = _xterm_esc_key_seqs,
    .esc_func_seqs = _xterm_esc_func_seqs
};

int nt_term_init()
{
    _used = &_xterm;

    return 0;
}

const struct nt_term* nt_term_get_used()
{
    return _used;
}

void nt_term_destroy()
{
    _used = NULL;
}
