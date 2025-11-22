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

#define _UCONV_IMPLEMENTATION_
#include "_uconv.h"
#undef _UCONV_IMPLEMENTATION_

#include "nt_esc.h"
#include "_nt_shared.h"

/* ------------------------------------------------------------------------- */
/* GENERAL */
/* ------------------------------------------------------------------------- */

static int _resize_fds[2];
static struct pollfd _poll_fds[2];
static struct termios _init_term_opts;

static nt_charbuff* _buff = NULL;

static void inline _buff_flush()
{
    const char* content;
    size_t content_len;

    nt_charbuff_rewind(_buff, &content, &content_len);
    _nt_awrite(STDOUT_FILENO, content, content_len);
}

static void inline _buff_append(const char* str, size_t str_len)
{
    if(_buff == NULL)
    {
        _nt_awrite(STDOUT_FILENO, str, str_len);
        return;
    }

    nt_status _status;
    nt_charbuff_append(_buff, str, str_len, &_status);
    if(_status == NT_ERR_OUT_OF_BOUNDS)
    {
        _buff_flush();
        nt_charbuff_append(_buff, str, str_len, &_status);
        if(_status == NT_ERR_OUT_OF_BOUNDS)
            _nt_awrite(STDOUT_FILENO, str, str_len);
    }
}

static void _sa_handler(int signum)
{
    char c = 'a';
    _nt_awrite(_resize_fds[1], &c, 1);
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

void __nt_init__(nt_status* out_status)
{
    nt_status _status;
    int status;

    status = tcgetattr(STDIN_FILENO, &_init_term_opts);
    if(status == -1)
        _nt_vreturn(out_status, NT_ERR_UNEXPECTED);

    struct termios _raw_term_opts = _init_term_opts;
    _term_opts_raw(&_raw_term_opts);

    status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &_raw_term_opts);
    if(status == -1)
        _nt_vreturn(out_status, NT_ERR_UNEXPECTED);

    struct sigaction sw_sa = {0};
    sw_sa.sa_handler = _sa_handler;
    status = sigaction(SIGWINCH, &sw_sa, NULL);
    if(status == -1)
        _nt_vreturn(out_status, NT_ERR_UNEXPECTED);

    int pipe_status = pipe(_resize_fds);
    if(pipe_status != 0)
        _nt_vreturn(out_status, NT_ERR_INIT_PIPE);

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
            _nt_vreturn(out_status, NT_ERR_INIT_TERM_ENV);
        default:
            _nt_vreturn(out_status, NT_ERR_UNEXPECTED);
    }

    _nt_vreturn(out_status, NT_SUCCESS);
}

void __nt_deinit__()
{
    _buff = NULL;

    nt_status _status;
    nt_write_str("", NT_GFX_DEFAULT, &_status);
    assert(_status == NT_SUCCESS);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &_init_term_opts);
    nt_term_destroy();
}

/* -------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* -------------------------------------------------------------------------- */

static void _execute_used_term_func(enum nt_esc_func func, bool use_va,
        nt_status* out_status, ...)
{
    int status;

    const struct nt_term_info* used_term = nt_term_get_used();

    const char* esc_func = used_term->esc_func_seqs[func];
    if(esc_func == NULL)
        _nt_vreturn(out_status, NT_ERR_FUNC_NOT_SUPPORTED);

    const char* _func;
    if(use_va)
    {
        char buff[50];

        va_list list;
        va_start(list, out_status);

        status = vsprintf(buff, esc_func, list);
        if(status < 0)
            _nt_vreturn(out_status, NT_ERR_UNEXPECTED);

        va_end(list);

        _func = buff;
    }
    else _func = esc_func;

    _buff_append(_func, strlen(_func));

    _nt_vreturn(out_status, NT_SUCCESS);
}

/* -------------------------------------------------------------------------- */

void nt_buffer_enable(nt_charbuff* buff)
{
    if(buff == NULL) return;
    if(_buff != NULL) return;

    _buff = buff;
}

void nt_buffer_disable(nt_buffact buffact)
{
    if(_buff == NULL) return;

    switch(buffact)
    {
        case NT_BUFF_KEEP:
            _buff = NULL;
            break;
        case NT_BUFF_DISCARD:
            nt_charbuff_rewind(_buff, NULL, NULL);
            _buff = NULL;
            break;
        case NT_BUFF_FLUSH:
            _buff_flush();
            _buff = NULL;
            break;
    }
}

nt_charbuff* nt_buffer_get()
{
    return _buff;
}

void nt_buffer_flush()
{
    if(_buff == NULL) return;

    _buff_flush();
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

void nt_cursor_hide(nt_status* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_CURSOR_HIDE, false, out_status);
}

void nt_cursor_show(nt_status* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_CURSOR_SHOW, false, out_status);
}

void nt_erase_screen(nt_status* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false, NULL);
    _execute_used_term_func(NT_ESC_FUNC_ERASE_SCREEN, false, out_status);
}

void nt_erase_line(nt_status* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false, NULL);
    _execute_used_term_func(NT_ESC_FUNC_ERASE_LINE, false, out_status);
}

void nt_erase_scrollback(nt_status* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false, NULL);
    _execute_used_term_func(NT_ESC_FUNC_ERASE_SCROLLBACK, false, out_status);
}

void nt_alt_screen_enable(nt_status* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_ENTER, false, out_status);
}

void nt_alt_screen_disable(nt_status* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_EXIT, false, out_status);
}

/* ------------------------------------------------------------------------- */
/* WRITE TO TERMINAL */
/* ------------------------------------------------------------------------- */


/* This function assumes:
 * 1) The terminal has the capability to set default fg and bg colors.
 * 2) If the terminal supports RGB, then the library holds the terminal's
 * esc sequence to set the RGB color for bg/fg. Same with 256 colors and
 * 8 colors */
static void _set_gfx(struct nt_gfx gfx, nt_status* out_status)
{
    nt_status _status;
    nt_term_color_count colors = nt_term_get_color_count();

    /* Set foreground --------------------------------------------------- */

    if(nt_color_are_equal(NT_COLOR_DEFAULT, gfx.fg))
    {
        _execute_used_term_func(
                NT_ESC_FUNC_FG_SET_DEFAULT,
                false, &_status);
    }
    else
    {
        if(colors == NT_TERM_COLOR_TC)
        {
            _execute_used_term_func(
                    NT_ESC_FUNC_FG_SET_RGB,
                    true, &_status,
                    gfx.fg._rgb.r,
                    gfx.fg._rgb.g,
                    gfx.fg._rgb.b);
        }
        else if(colors == NT_TERM_COLOR_C256)
        {
            _execute_used_term_func(
                    NT_ESC_FUNC_FG_SET_C256,
                    true, &_status,
                    gfx.fg._code256);
        }
        else if(colors == NT_TERM_COLOR_C8)
        {
            _execute_used_term_func(
                    NT_ESC_FUNC_FG_SET_C8,
                    true, &_status,
                    gfx.fg._code8);
        }
        else { _nt_vreturn(out_status, NT_ERR_UNEXPECTED); }
    }

    if(_status != NT_SUCCESS)
    {
        _nt_vreturn(out_status, NT_ERR_UNEXPECTED);
    }

    /* Set background --------------------------------------------------- */

    if(nt_color_are_equal(NT_COLOR_DEFAULT, gfx.bg))
    {
        _execute_used_term_func(
                NT_ESC_FUNC_BG_SET_DEFAULT,
                false, &_status);
    }
    else
    {
        if(colors == NT_TERM_COLOR_TC)
        {
            _execute_used_term_func(
                    NT_ESC_FUNC_BG_SET_RGB,
                    true, &_status,
                    gfx.bg._rgb.r,
                    gfx.bg._rgb.g,
                    gfx.bg._rgb.b);
        }
        else if(colors == NT_TERM_COLOR_C256)
        {
            _execute_used_term_func(
                    NT_ESC_FUNC_BG_SET_C256,
                    true, &_status,
                    gfx.bg._code256);
        }
        else if(colors == NT_TERM_COLOR_C8)
        {
            _execute_used_term_func(
                    NT_ESC_FUNC_BG_SET_C8,
                    true, &_status,
                    gfx.bg._code8);
        }
        else { _nt_vreturn(out_status, NT_ERR_UNEXPECTED); }
    }

    if(_status != NT_SUCCESS)
    {
        _nt_vreturn(out_status, NT_ERR_UNEXPECTED);
    }

    /* Set style -------------------------------------------------------- */

    uint8_t style;

    if(colors == NT_TERM_COLOR_TC)
        style = gfx.style._value_c8;
    else if(colors == NT_TERM_COLOR_C256)
        style = gfx.style._value_c256;
    else if(colors == NT_TERM_COLOR_C8)
        style = gfx.style._value_rgb;
    else { _nt_vreturn(out_status, NT_ERR_UNEXPECTED); }

    size_t i;
    size_t count = 8;
    for(i = 0; i < count; i++)
    {
        if(style & (NT_STYLE_VAL_BOLD << i))
        {
            _execute_used_term_func(NT_ESC_FUNC_STYLE_SET_BOLD + i, true, &_status);
            if(_status != NT_SUCCESS)
            {
                if(_status != NT_ERR_FUNC_NOT_SUPPORTED)
                    _nt_vreturn(out_status, _status);
            }
        }
    }

    _nt_vreturn(out_status, NT_SUCCESS);
}

// UTF-32
void nt_write_char(uint32_t codepoint, struct nt_gfx gfx, nt_status* out_status)
{
    char utf8[5];
    size_t utf8_len;
    nt_status _status;

    uc_utf32_to_utf8(&codepoint, 1, (uint8_t*)utf8,
            4, 0, NULL, &utf8_len, &_status);

    switch(_status)
    {
        case UC_SUCCESS:
            break;
        case UC_ERR_SURROGATE:
            _nt_vreturn(out_status, NT_ERR_INVALID_UTF32);
        case UC_ERR_INVALID_CODEPOINT:
            _nt_vreturn(out_status, NT_ERR_INVALID_UTF32);
        default:
            _nt_vreturn(out_status, NT_ERR_UNEXPECTED);
    }

    utf8[utf8_len] = '\0';

    nt_write_str(utf8, gfx, out_status);
}

// UTF-8
void nt_write_str(const char* str, struct nt_gfx gfx, nt_status* out_status)
{
    nt_status _status;

    _execute_used_term_func(NT_ESC_FUNC_GFX_RESET, false, &_status);
    if(_status != NT_SUCCESS)
        _nt_vreturn(out_status, _status);

    _set_gfx(gfx, &_status);
    if(_status != NT_SUCCESS)
        _nt_vreturn(out_status, _status);

    _buff_append(str, strlen(str));

    _nt_vreturn(out_status, NT_SUCCESS);
}

void nt_write_char_at(uint32_t codepoint, struct nt_gfx gfx, size_t x, size_t y,
        nt_status* out_status)
{
    size_t _width, _height;
    nt_get_term_size(&_width, &_height);

    if((x >= _width) || (y >= _height))
    {
        _nt_vreturn(out_status, NT_ERR_OUT_OF_BOUNDS);
    }

    nt_status _status;

    _execute_used_term_func(NT_ESC_FUNC_CURSOR_MOVE, true, &_status, y + 1, x + 1);
    if(_status != NT_SUCCESS)
        _nt_vreturn(out_status, _status);

    nt_write_char(codepoint, gfx, out_status);
}

void nt_write_str_at(const char* str, struct nt_gfx gfx, size_t x, size_t y,
        nt_status* out_status)
{
    size_t _width, _height;
    nt_get_term_size(&_width, &_height);

    if((x >= _width) || (y >= _height))
    {
        _nt_vreturn(out_status, NT_ERR_OUT_OF_BOUNDS);
    }

    nt_status _status;

    _execute_used_term_func(NT_ESC_FUNC_CURSOR_MOVE, true, &_status, y + 1, x + 1);
    if(_status != NT_SUCCESS)
        _nt_vreturn(out_status, _status);

    nt_write_str(str, gfx, out_status);
}

/* -------------------------------------------------------------------------- */
/* EVENT */
/* -------------------------------------------------------------------------- */

/* Called by nt_wait_for_event() internally. */
static struct nt_key_event _process_key_event(nt_status* out_status);

/* Called by nt_wait_for_event() internally. */
static struct nt_resize_event _process_resize_event(nt_status* out_status);

/* -------------------------------------------------------------------------- */

static const struct nt_event _NT_EVENT_EMPTY = {0};

struct nt_event nt_wait_for_event(int timeout, nt_status* out_status)
{
    struct timespec _time1, _time2;
    clock_gettime(CLOCK_REALTIME, &_time1);

    int poll_status = _nt_apoll(_poll_fds, 2, timeout);

    clock_gettime(CLOCK_REALTIME, &_time2);

    ulong elapsed = ((_time2.tv_sec - _time1.tv_sec) * 1e3) +
        ((_time2.tv_nsec - _time1.tv_nsec) / 1e6);

    elapsed = (elapsed <= timeout) ? elapsed : timeout;

    if(poll_status == -1)
    {
        _nt_return(_NT_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }
    if(poll_status == 0)
    {
        struct nt_event ret = {
            .type = NT_EVENT_TIMEOUT,
            .elapsed = timeout
        };

        _nt_return(ret, out_status, NT_SUCCESS);
    }

    struct nt_event event;
    nt_status _status;
    if(_poll_fds[0].revents & POLLIN)
    {
        struct nt_key_event key_event = _process_key_event(&_status);
        event = (struct nt_event) {
            .type = NT_EVENT_KEY,
            .key_data = key_event,
            .elapsed = elapsed
        };
    }
    else // (poll_fds[1].revents & POLLIN)
    {
        struct nt_resize_event resize_event = _process_resize_event(&_status);
        event = (struct nt_event) {
            .type = NT_EVENT_RESIZE,
            .resize_data = resize_event,
            .elapsed = elapsed
        };
    }

    _nt_return(event, out_status, NT_SUCCESS);
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
        bool alt, nt_status* out_status);

/* Called by _process_key_event() internally.
 * `buff` - ptr to the first byte of the ESC sequence(ESC char).
 * The whole escape sequences will be read into the buffer inside the function. */
static struct nt_key_event _process_key_event_esc_key(uint8_t* buff,
        nt_status* out_status);

/* ------------------------------------------------------ */

static struct nt_key_event _process_key_event(nt_status* out_status)
{
    unsigned char buff[64];
    int poll_status, read_status;
    nt_status _status;

    read_status = _nt_aread(STDIN_FILENO, buff, 1);
    if(read_status < 0)
    {
        _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    if(buff[0] == 0x1b) // ESC or ESC SEQ or ALT + PRINTABLE
    {
        poll_status = _nt_apoll(_poll_fds, 1, ESC_TIMEOUT);
        if(poll_status == -1)
        {
            _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
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

            _nt_return(key_event, out_status, NT_SUCCESS);
        }

        read_status = _nt_aread(STDIN_FILENO, buff + 1, 1);
        if(read_status == -1)
        {
            _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
        }

        /* Probably ESC KEY, will be sure after poll() */
        if((buff[1] == '[') || (buff[1] == 'O'))
        {
            poll_status = _nt_apoll(_poll_fds, 1, ESC_TIMEOUT);
            if(poll_status == -1)
            {
                _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
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
                _nt_return(key_event, out_status, NT_SUCCESS);
            }

            // ESC KEY

            struct nt_key_event key_event =
                _process_key_event_esc_key(buff, &_status);

            switch(_status)
            {
                case NT_SUCCESS:
                    _nt_return(key_event, out_status, NT_SUCCESS);
                default:
                    _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
            }

        }
        else
        {
            struct nt_key_event key_event =
                _process_key_event_utf32(buff + 1, true, &_status);

            switch(_status)
            {
                case NT_SUCCESS:
                    _nt_return(key_event, out_status, NT_SUCCESS);
                default:
                    _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
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
                _nt_return(key_event, out_status, NT_SUCCESS);
            default:
                _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
        }
    }
}

static struct nt_key_event _process_key_event_utf32(uint8_t* utf8_sbyte,
        bool alt, nt_status* out_status)
{
    size_t utf32_len;
    utf32_len = uc_utf8_unit_len(utf8_sbyte[0]);
    if(utf32_len == SIZE_MAX)
    {
        _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    // read()
    int read_status = _nt_aread(STDIN_FILENO, utf8_sbyte + 1, utf32_len - 1);
    if(read_status < 0)
    {
        _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }
    
    if(read_status != (utf32_len - 1))
    {
        _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED); // ?
    }

    uint32_t utf32;
    size_t utf32_width;
    uc_status _status;
    uc_utf8_to_utf32(utf8_sbyte, utf32_len, &utf32, 1, 0, &utf32_width, &_status);
    if(_status != UC_SUCCESS)
    {
        _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    struct nt_key_event key_event = {
        .type = NT_KEY_EVENT_UTF32,
        .utf32_data = {
            .codepoint = utf32,
            .alt = alt
        }
    };

    _nt_return(key_event, out_status, NT_SUCCESS);
}

static struct nt_key_event _process_key_event_esc_key(uint8_t* buff,
        nt_status* out_status)
{
    int read_status;
    int read_count = 2;
    while(true)
    {
        read_status = _nt_aread(STDIN_FILENO, buff + read_count, 1);
        if(read_status < 0)
        {
            _nt_return(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
        }

        if((buff[read_count] >= 0x40) && (buff[read_count] <= 0x7E))
            break;

        read_count++;
    }

    buff[read_count + 1] = 0;

    char* cbuff = (char*)buff;

    int i;
    struct nt_key_event ret;
    const struct nt_term_info* term = nt_term_get_used();
    for(i = 0; i < NT_ESC_KEY_OTHER; i++)
    {
        if(strcmp(cbuff, term->esc_key_seqs[i]) == 0)
        {
            ret = (struct nt_key_event) {
                .type = NT_KEY_EVENT_ESC_KEY,
                .esc_key_data = {
                    .esc_key = i
                }
            };

            _nt_return(ret, out_status, NT_SUCCESS);
        }
    }

    ret = (struct nt_key_event) {
        .type = NT_KEY_EVENT_ESC_KEY,
        .esc_key_data = {
            .esc_key = NT_ESC_KEY_OTHER
        }
    };

    _nt_return(ret, out_status, NT_SUCCESS);
}

/* ------------------------------------------------------ */
/* RESIZE EVENT */
/* ------------------------------------------------------ */

static const struct nt_resize_event _NT_RESIZE_EVENT_EMPTY = {0};

static struct nt_resize_event _process_resize_event(nt_status* out_status)
{
    char buff[1];
    int read_status = _nt_aread(_resize_fds[0], buff, 1);
    if(read_status == -1)
    {
        _nt_return(_NT_RESIZE_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    size_t _width, _height;
    nt_get_term_size(&_width, &_height);

    struct nt_resize_event resize_event = {
        .width = _width,
        .height = _height
    };

    _nt_return(resize_event, out_status, NT_SUCCESS);
}
