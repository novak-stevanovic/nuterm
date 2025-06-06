/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#include "nt.h"

#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "nt_charbuff.h"
#include "_nt_term.h"
#include "_uconv.h"
#include "nt_esc.h"
#include "_nt_shared.h"

/* ------------------------------------------------------------------------- */
/* GENERAL */
/* ------------------------------------------------------------------------- */

static int _resize_fds[2];
static struct pollfd _poll_fds[2];
static struct termios _init_term_opts;

static nt_charbuff_t* _buff;

static void _sa_handler(int signum)
{
    char c = 'a';
    nt_awrite(_resize_fds[1], &c, 1);
}

static inline void _term_opts_raw(struct termios* term_opts)
{
    term_opts->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
            | INLCR | IGNCR | ICRNL | IXON);
    term_opts->c_oflag &= ~OPOST;
    term_opts->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    term_opts->c_cflag &= ~(CSIZE | PARENB);
    term_opts->c_cflag |= CS8;

    term_opts->c_cc[VMIN]  = 1;
    term_opts->c_cc[VTIME] = 0;
}

void __nt_init__(nt_status_t* out_status)
{
    nt_status_t _status;
    int status;

    status = tcgetattr(STDIN_FILENO, &_init_term_opts);
    if(status == -1)
        _vreturn(out_status, NT_ERR_UNEXPECTED);

    struct termios _raw_term_opts = _init_term_opts;
    _term_opts_raw(&_raw_term_opts);

    status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &_raw_term_opts);
    if(status == -1)
        _vreturn(out_status, NT_ERR_UNEXPECTED);

    struct sigaction sw_sa = {0};
    sw_sa.sa_handler = _sa_handler;
    status = sigaction(SIGWINCH, &sw_sa, NULL);
    if(status == -1)
        _vreturn(out_status, NT_ERR_UNEXPECTED);

    int pipe_status = pipe(_resize_fds);
    if(pipe_status != 0)
        _vreturn(out_status, NT_ERR_INIT_PIPE);

    _poll_fds[0] = (struct pollfd) {
        .fd = STDIN_FILENO,
        .events = POLLIN,
        .revents = 0
    };
    _poll_fds[1] = (struct pollfd) {
        .fd = _resize_fds[0],
        .events = POLLIN,
        .revents = 0
    };

    nt_term_init(&_status);
    switch(_status)
    {
        case NT_SUCCESS:
            break;
        case NT_ERR_TERM_NOT_SUPPORTED:
            break;
        case NT_ERR_INIT_TERM_ENV:
            _vreturn(out_status, NT_ERR_INIT_TERM_ENV);
        default:
            _vreturn(out_status, NT_ERR_UNEXPECTED);
    }

    _buff = nt_charbuff_new(0);
    if(_buff == NULL)
        _vreturn(out_status, NT_ERR_ALLOC_FAIL);

    _vreturn(out_status, NT_SUCCESS);
}

void __nt_deinit__()
{
    nt_write_str("", NT_GFX_DEFAULT,
            NULL, NULL);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &_init_term_opts);
    nt_term_destroy();
    nt_charbuff_destroy(_buff);
}

/* -------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* -------------------------------------------------------------------------- */

static void _execute_used_term_func(enum nt_esc_func func, bool use_va,
        nt_status_t* out_status, ...)
{
    int status;

    const struct nt_term_info* used_term = nt_term_get_used();

    const char* esc_func = used_term->esc_func_seqs[func];
    if(esc_func == NULL)
        _vreturn(out_status, NT_ERR_FUNC_NOT_SUPPORTED);

    const char* _func;
    if(use_va)
    {
        char buff[50];

        va_list list;
        va_start(list, out_status);

        status = vsprintf(buff, esc_func, list);
        if(status < 0)
            _vreturn(out_status, NT_ERR_UNEXPECTED);

        va_end(list);

        _func = buff;
    }
    else _func = esc_func;

    nt_charbuff_append(_buff, _func);

    _vreturn(out_status, NT_SUCCESS);
}

/* -------------------------------------------------------------------------- */

void nt_buffer_set_cap(size_t buff_cap, nt_status_t* out_status)
{
    int status = nt_charbuff_set_cap(_buff, buff_cap);

    nt_status_t ret = (status == 0) ? NT_SUCCESS : NT_ERR_ALLOC_FAIL;
    _vreturn(out_status, ret);
}

void nt_buffer_flush()
{
    nt_charbuff_flush(_buff);
}

/* ----------------------------------------------------- */

void nt_get_term_size(size_t* out_width, size_t* out_height)
{
    struct winsize size;
    int status = ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
    size_t ret_width, ret_height;
    if(status == -1)
    {
        ret_width = 0;
        ret_height =  0;
        return;
    }
    else
    {
        ret_width = size.ws_col;
        ret_height = size.ws_row;
    }

    if(out_width != NULL) *out_width = ret_width;
    if(out_height != NULL) *out_height = ret_height;
}

void nt_cursor_hide(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_CURSOR_HIDE, false, out_status);
}

void nt_cursor_show(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_CURSOR_SHOW, false, out_status);
}

void nt_erase_screen(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false, NULL);
    _execute_used_term_func(NT_ESC_FUNC_ERASE_SCREEN, false, out_status);
}

void nt_erase_line(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false, NULL);
    _execute_used_term_func(NT_ESC_FUNC_ERASE_LINE, false, out_status);
}

void nt_erase_scrollback(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false, NULL);
    _execute_used_term_func(NT_ESC_FUNC_ERASE_SCROLLBACK, false, out_status);
}

void nt_alt_screen_enable(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_ENTER, false, out_status);
}

void nt_alt_screen_disable(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_EXIT, false, out_status);
}

/* ------------------------------------------------------------------------- */
/* WRITE TO TERMINAL */
/* ------------------------------------------------------------------------- */

typedef enum set_color_opt {
    SET_COLOR_FG,
    SET_COLOR_BG
} set_color_opt_t;

static void _set_color(nt_color_t color, set_color_opt_t opt,
        nt_status_t* out_status)
{
    nt_status_t _status;
    nt_term_color_count_t colors = nt_term_get_color_count();

    size_t func_offset = (opt == SET_COLOR_FG) ? 0 : 4;

    if(nt_color_cmp(NT_COLOR_DEFAULT, color)) // DEFAULT COLOR
    {
        _execute_used_term_func(NT_ESC_FUNC_FG_SET_DEFAULT + func_offset,
                false, &_status);
        _vreturn(out_status, _status);
    }

    switch(colors)
    {
        case NT_TERM_COLOR_TC:
            _execute_used_term_func(NT_ESC_FUNC_FG_SET_RGB + func_offset, true,
                    &_status, color._rgb.r, color._rgb.g, color._rgb.b);

            /* If func is not supported by the terminal, proceed to the
             * next case. */
            if(_status != NT_ERR_FUNC_NOT_SUPPORTED)
                _vreturn(out_status, _status);

        case NT_TERM_COLOR_C256:
            _execute_used_term_func(NT_ESC_FUNC_FG_SET_C256 + func_offset, true,
                    &_status, color._code256);

            /* If func is not supported by the terminal, proceed to the
             * next case. */
            if(_status != NT_ERR_FUNC_NOT_SUPPORTED)
                _vreturn(out_status, _status);

        case NT_TERM_COLOR_C8:
            _execute_used_term_func(NT_ESC_FUNC_FG_SET_C8 + func_offset, true,
                    &_status, color._code8);

            /* No more color palletes to fall back to. */
            _vreturn(out_status, _status);

        default:
            _vreturn(out_status, NT_ERR_UNEXPECTED);
    }
}

#define _sreturn(out_style_param, out_style, out_status_param, out_status)     \
    do                                                                         \
    {                                                                          \
        if((out_status_param) != NULL)                                         \
            (*out_status_param) = out_status;                                  \
        if((out_style_param) != NULL)                                          \
            (*out_style_param) = out_style;                                    \
        return;                                                                \
    } while(0)                                                                 \

/* Assumes styles are not set before calling. */
static void _set_style(nt_style_t style, nt_style_t* out_style,
        nt_status_t* out_status)
{
    nt_status_t _status;

    size_t i;
    size_t count = 8;
    nt_style_t used = NT_STYLE_DEFAULT;
    for(i = 0; i < count; i++)
    {
        if(style & (NT_STYLE_BOLD << i))
        {
            _execute_used_term_func(NT_ESC_FUNC_STYLE_SET_BOLD + i, true, &_status);
            if(_status != NT_SUCCESS)
            {
                if(_status != NT_ERR_FUNC_NOT_SUPPORTED)
                    _sreturn(out_style, used, out_status, _status);
            }
            else used |= (NT_STYLE_BOLD << i);
        }
    }

    _sreturn(out_style, used, out_status, NT_SUCCESS);
}

// UTF-32
void nt_write_char(uint32_t codepoint, struct nt_gfx gfx, nt_style_t* out_styles,
        nt_status_t* out_status)
{
    char utf8[5];
    size_t utf8_len;
    nt_status_t _status;

    uc_utf32_to_utf8(&codepoint, 1, (uint8_t*)utf8,
            4, 0, NULL, &utf8_len, &_status);

    switch(_status)
    {
        case UC_SUCCESS:
            break;
        case UC_ERR_SURROGATE:
            _sreturn(out_styles, NT_STYLE_DEFAULT,
                    out_status, NT_ERR_INVALID_UTF32);
        case UC_ERR_INVALID_CODEPOINT:
            _sreturn(out_styles, NT_STYLE_DEFAULT,
                    out_status, NT_ERR_INVALID_UTF32);
        default:
            _sreturn(out_styles, NT_STYLE_DEFAULT,
                    out_status, NT_ERR_UNEXPECTED);
    }

    utf8[utf8_len] = '\0';

    nt_write_str(utf8, gfx, out_styles, out_status);
}

// UTF-8
void nt_write_str(const char* str, struct nt_gfx gfx, nt_style_t* out_styles,
        nt_status_t* out_status)
{
    nt_status_t _status;

    _execute_used_term_func(NT_ESC_FUNC_GFX_RESET, false, &_status);
    if(_status != NT_SUCCESS)
        _sreturn(out_styles, NT_STYLE_DEFAULT, out_status, _status);

    _set_color(gfx.fg, SET_COLOR_FG, &_status);
    if(_status != NT_SUCCESS)
        _sreturn(out_styles, NT_STYLE_DEFAULT, out_status, _status);

    _set_color(gfx.bg, SET_COLOR_BG, &_status);
    if(_status != NT_SUCCESS)
        _sreturn(out_styles, NT_STYLE_DEFAULT, out_status, _status);

    nt_style_t used_styles;
    _set_style(gfx.style, &used_styles, &_status);
    if(_status != NT_SUCCESS)
        _sreturn(out_styles, used_styles, out_status, _status);

    nt_charbuff_append(_buff, str);

    _sreturn(out_styles, used_styles, out_status, NT_SUCCESS);
}

void nt_write_char_at(uint32_t codepoint, struct nt_gfx gfx, size_t x, size_t y,
        nt_style_t* out_styles, nt_status_t* out_status)
{
    size_t _width, _height;
    nt_get_term_size(&_width, &_height);

    if((x >= _width) || (y >= _height))
    {
        _sreturn(out_styles, NT_STYLE_DEFAULT, out_status,
                NT_ERR_OUT_OF_BOUNDS);
    }

    nt_status_t _status;

    _execute_used_term_func(NT_ESC_FUNC_CURSOR_MOVE, true, &_status, y + 1, x + 1);
    if(_status != NT_SUCCESS)
        _sreturn(out_styles, NT_STYLE_DEFAULT, out_status, _status);

    nt_write_char(codepoint, gfx, out_styles, out_status);
}

void nt_write_str_at(const char* str, struct nt_gfx gfx, size_t x, size_t y,
        nt_style_t* out_styles, nt_status_t* out_status)
{
    size_t _width, _height;
    nt_get_term_size(&_width, &_height);

    if((x >= _width) || (y >= _height))
    {
        _sreturn(out_styles, NT_STYLE_DEFAULT, out_status,
                NT_ERR_OUT_OF_BOUNDS);
    }

    nt_status_t _status;

    _execute_used_term_func(NT_ESC_FUNC_CURSOR_MOVE, true, &_status, y + 1, x + 1);
    if(_status != NT_SUCCESS)
        _sreturn(out_styles, NT_STYLE_DEFAULT, out_status, _status);

    nt_write_str(str, gfx, out_styles, out_status);
}

/* -------------------------------------------------------------------------- */
/* EVENT */
/* -------------------------------------------------------------------------- */

/* Called by nt_wait_for_event() internally. */
static struct nt_key_event _process_key_event(nt_status_t* out_status);

/* Called by nt_wait_for_event() internally. */
static struct nt_resize_event _process_resize_event(nt_status_t* out_status);

/* -------------------------------------------------------------------------- */

static const struct nt_event _NT_EVENT_EMPTY = {0};

struct nt_event nt_wait_for_event(int timeout, nt_status_t* out_status)
{
    struct timespec _time1, _time2;
    clock_gettime(CLOCK_REALTIME, &_time1);

    int poll_status = nt_apoll(_poll_fds, 2, timeout);

    clock_gettime(CLOCK_REALTIME, &_time2);

    ulong elapsed = ((_time2.tv_sec - _time1.tv_sec) * 1e3) +
        ((_time2.tv_nsec - _time1.tv_nsec) / 1e6);

    if(poll_status == -1)
    {
        _return(_NT_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }
    if(poll_status == 0)
    {
        struct nt_event ret = {
            .type = NT_EVENT_TYPE_TIMEOUT,
            .elapsed = timeout
        };

        _return(ret, out_status, NT_SUCCESS);
    }

    struct nt_event event;
    nt_status_t _status;
    if(_poll_fds[0].revents & POLLIN)
    {
        struct nt_key_event key_event = _process_key_event(&_status);
        event = (struct nt_event) {
            .type = NT_EVENT_TYPE_KEY,
            .key_data = key_event,
            .elapsed = elapsed
        };
    }
    else // (poll_fds[1].revents & POLLIN)
    {
        struct nt_resize_event resize_event = _process_resize_event(&_status);
        event = (struct nt_event) {
            .type = NT_EVENT_TYPE_RESIZE,
            .resize_data = resize_event,
            .elapsed = elapsed
        };
    }

    _return(event, out_status, NT_SUCCESS);
}

/* ------------------------------------------------------ */
/* KEY EVENT */
/* ------------------------------------------------------ */

#define ESC_TIMEOUT 10

static const struct nt_key_event _NT_KEY_EVENT_EMPTY = {0};

/* Called by _process_key_event() internally.
 * `utf8_sbyte` - ptr to first byte of utf8.
 * Other bytes will be read into `utf8_sbyte` in the function. */
static struct nt_key_event _process_key_event_utf32(uint8_t* utf8_sbyte,
        bool alt, nt_status_t* out_status);

/* Called by _process_key_event() internally.
 * `buff` - ptr to the first byte of the ESC sequence(ESC char).
 * The whole escape sequences will be read into the buffer inside the function. */
static struct nt_key_event _process_key_event_esc_key(uint8_t* buff,
        nt_status_t* out_status);

/* ------------------------------------------------------ */

static struct nt_key_event _process_key_event(nt_status_t* out_status)
{
    unsigned char buff[64];
    int poll_status, read_status;
    nt_status_t _status;

    read_status = nt_aread(STDIN_FILENO, buff, 1);
    if(read_status < 0)
    {
        _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    if(buff[0] == 0x1b) // ESC or ESC SEQ or ALT + PRINTABLE
    {
        poll_status = nt_apoll(_poll_fds, 1, ESC_TIMEOUT);
        if(poll_status == -1)
        {
            _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
        }

        if(poll_status == 0) // timeout - just ESC
        {
            struct nt_key_event key_event = {
                .type = NT_KEY_EVENT_UTF32,
                .utf32_data = {
                    .codepoint = 27,
                    .alt = false
                }
            };

            _return(key_event, out_status, NT_SUCCESS);
        }

        read_status = nt_aread(STDIN_FILENO, buff + 1, 1);
        if(read_status == -1)
        {
            _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
        }

        /* Probably ESC KEY, will be sure after poll() */
        if((buff[1] == '[') || (buff[1] == 'O'))
        {
            poll_status = nt_apoll(_poll_fds, 1, ESC_TIMEOUT);
            if(poll_status == -1)
            {
                _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
            }

            if(poll_status == 0) // Not in fact an ESC key...
            {
                struct nt_key_event key_event = {
                    .type = NT_KEY_EVENT_UTF32,
                    .utf32_data = {
                        .codepoint = buff[1],
                        .alt = true,
                    }
                };
                _return(key_event, out_status, NT_SUCCESS);
            }

            // ESC KEY

            struct nt_key_event key_event =
                _process_key_event_esc_key(buff, &_status);

            switch(_status)
            {
                case NT_SUCCESS:
                    _return(key_event, out_status, NT_SUCCESS);
                default:
                    _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
            }

        }
        else
        {
            struct nt_key_event key_event =
                _process_key_event_utf32(buff + 1, true, &_status);

            switch(_status)
            {
                case NT_SUCCESS:
                    _return(key_event, out_status, NT_SUCCESS);
                default:
                    _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
            }
        }
    }
    else // UTF32
    {
        struct nt_key_event key_event =
            _process_key_event_utf32(buff, false, &_status);

        switch(_status)
        {
            case NT_SUCCESS:
                _return(key_event, out_status, NT_SUCCESS);
            default:
                _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
        }
    }
}

static struct nt_key_event _process_key_event_utf32(uint8_t* utf8_sbyte,
        bool alt, nt_status_t* out_status)
{
    size_t utf32_len;
    utf32_len = uc_utf8_char_len(utf8_sbyte[0]);
    if(utf32_len == SIZE_MAX)
    {
        _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    // read()
    int read_status = nt_aread(STDIN_FILENO, utf8_sbyte + 1, utf32_len - 1);
    if(read_status < 0)
    {
        _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }
    
    if(read_status != (utf32_len - 1))
    {
        _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED); // ?
    }

    uint32_t utf32;
    size_t utf32_width;
    uc_status_t _status;
    uc_utf8_to_utf32(utf8_sbyte, utf32_len, &utf32, 1, 0, &utf32_width, &_status);
    if(_status != UC_SUCCESS)
    {
        _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    struct nt_key_event key_event = {
        .type = NT_KEY_EVENT_UTF32,
        .utf32_data = {
            .codepoint = utf32,
            .alt = alt
        }
    };

    _return(key_event, out_status, NT_SUCCESS);
}

static struct nt_key_event _process_key_event_esc_key(uint8_t* buff,
        nt_status_t* out_status)
{
    int read_status;
    int read_count = 2;
    while(true)
    {
        read_status = nt_aread(STDIN_FILENO, buff + read_count, 1);
        if(read_status < 0)
        {
            _return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
        }

        if((buff[read_count] >= 0x40) && (buff[read_count] <= 0x7E))
            break;

        read_count++;
    }

    buff[read_count + 1] = 0;

    char* _buff = (char*)buff;

    int i;
    struct nt_key_event ret;
    const struct nt_term_info* term = nt_term_get_used();
    for(i = 0; i < NT_ESC_KEY_OTHER; i++)
    {
        if(strcmp(_buff, term->esc_key_seqs[i]) == 0)
        {
            ret = (struct nt_key_event) {
                .type = NT_KEY_EVENT_ESC_KEY,
                .esc_key_data = {
                    .esc_key = i
                }
            };

            _return(ret, out_status, NT_SUCCESS);
        }
    }

    ret = (struct nt_key_event) {
        .type = NT_KEY_EVENT_ESC_KEY,
        .esc_key_data = {
            .esc_key = NT_ESC_KEY_OTHER
        }
    };

    _return(ret, out_status, NT_SUCCESS);
}

/* ------------------------------------------------------ */
/* RESIZE EVENT */
/* ------------------------------------------------------ */

static const struct nt_resize_event _NT_RESIZE_EVENT_EMPTY = {0};

static struct nt_resize_event _process_resize_event(nt_status_t* out_status)
{
    char buff[1];
    int read_status = nt_aread(_resize_fds[0], buff, 1);
    if(read_status == -1)
    {
        _return(_NT_RESIZE_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    size_t _width, _height;
    nt_get_term_size(&_width, &_height);

    struct nt_resize_event resize_event = {
        .width = _width,
        .height = _height
    };

    _return(resize_event, out_status, NT_SUCCESS);
}
