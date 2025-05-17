#include "nuterm.h"

#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "_nt_charbuff.h"
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

bool _buff_enabled;
static nt_charbuff_t _buff;

static void _sa_handler(int signum)
{
    char c = 'a';
    nt_awrite(_resize_fds[1], &c, 1);
}

void nt_init(nt_status_t* out_status)
{
    nt_status_t _status;
    int status;
    struct termios raw_term_opts;
    cfmakeraw(&raw_term_opts); // TODO - set flags manually for compatibility

    status = tcgetattr(STDIN_FILENO, &_init_term_opts);
    if(status == -1)
    {
        _VRETURN(out_status, NT_ERR_UNEXPECTED);
    }
    status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_term_opts);
    if(status == -1)
    {
        _VRETURN(out_status, NT_ERR_UNEXPECTED);
    }

    struct sigaction sw_sa = {0};
    sw_sa.sa_handler = _sa_handler;
    status = sigaction(SIGWINCH, &sw_sa, NULL);
    if(status == -1)
    {
        _VRETURN(out_status, NT_ERR_UNEXPECTED);
    }

    int pipe_status = pipe(_resize_fds);
    if(pipe_status != 0)
    {
        _VRETURN(out_status, NT_ERR_INIT_PIPE);
    }

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
        case NT_ERR_INIT_TERM_ENV:
            _VRETURN(out_status, NT_ERR_INIT_TERM_ENV);
        case NT_ERR_TERM_NOT_SUPPORTED:
            _VRETURN(out_status, NT_ERR_TERM_NOT_SUPPORTED);
        default:
            _VRETURN(out_status, NT_ERR_UNEXPECTED);
    }

    nt_charbuff_init(&_buff, &_status);
    if(_status != NT_SUCCESS)
    {
        _VRETURN(out_status, NT_ERR_ALLOC_FAIL);
    }

    _buff_enabled = false;

    _VRETURN(out_status, NT_SUCCESS);
}

void nt_destroy()
{
    struct nt_gfx reset = {
        .fg = NT_COLOR_DEFAULT,
        .bg = NT_COLOR_DEFAULT,
        .style = NT_STYLE_DEFAULT
    };

    nt_write_str("", reset, NT_WRITE_INPLACE, NT_WRITE_INPLACE, NULL, NULL);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &_init_term_opts);
    nt_term_destroy();
    nt_charbuff_destroy(&_buff);
}

/* -------------------------------------------------------------------------- */
/* COLOR & STYLE */
/* -------------------------------------------------------------------------- */

const nt_color_t NT_COLOR_DEFAULT = {
    ._code8 = 0,
    ._code256 = 0,
    ._rgb = { 0, 0, 0 },
    .__default = true
};

static inline bool _color_is_default(nt_color_t color)
{
    return color.__default;
}

nt_color_t nt_color_new(uint8_t r, uint8_t g, uint8_t b)
{
    return (nt_color_t) {
        ._rgb =  { .r = r, .g = g, .b = b },
        ._code256 = nt_rgb_to_c256(r, g, b),
        ._code8 = nt_rgb_to_c8(r, g, b)
    };
}

bool nt_color_cmp(nt_color_t c1, nt_color_t c2)
{
    return (memcmp(&c1, &c2, sizeof(nt_color_t)) == 0);
}

/* -------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* -------------------------------------------------------------------------- */

static void _execute_used_term_func(nt_esc_func_t func,
        nt_status_t* out_status, ...)
{
    int status;
    nt_status_t _status;

    const struct nt_term_info* used_term = nt_term_get_used();
    if(used_term == NULL)
    {
        _VRETURN(out_status, NT_ERR_TERM_UNKNOWN);
    }

    const char* esc_func = used_term->esc_func_seqs[func];
    if(esc_func == NULL)
    {
        _VRETURN(out_status, NT_ERR_FUNC_NOT_SUPPORTED);
    }

    char buff[50];

    va_list list;
    va_start(list, out_status);

    status = vsprintf(buff, esc_func, list);
    if(status < 0)
    {
        _VRETURN(out_status, NT_ERR_UNEXPECTED);
    }

    va_end(list);

    nt_charbuff_append(&_buff, buff, &_status);
    if(_status == NT_ERR_ALLOC_FAIL)
    {
        _VRETURN(out_status, NT_ERR_ALLOC_FAIL);
    }

    if(!_buff_enabled)
    {
        nt_buffer_flush();
    }

    _VRETURN(out_status, NT_SUCCESS);
}

/* -------------------------------------------------------------------------- */

void nt_buffer_enable()
{
    _buff_enabled = true;
}

void nt_buffer_flush()
{
    // Assume write() doesn't fail.
    nt_awrite(STDOUT_FILENO, _buff.data, _buff.len);

    nt_charbuff_rewind(&_buff, NULL);
}

void nt_buffer_disable(nt_buffact_t action)
{
    if(action == NT_BUFF_FLUSH)
        nt_buffer_flush();
    else
        nt_charbuff_rewind(&_buff, NULL);

    _buff_enabled = false;
}

void nt_cursor_hide(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_CURSOR_HIDE, out_status);
}

void nt_cursor_show(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_CURSOR_SHOW, out_status);
}

void nt_erase_screen(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, NULL);
    _execute_used_term_func(NT_ESC_FUNC_ERASE_SCREEN, out_status);
}

void nt_erase_line(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, NULL);
    _execute_used_term_func(NT_ESC_FUNC_ERASE_LINE, out_status);
}

void nt_erase_scrollback(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, NULL);
    _execute_used_term_func(NT_ESC_FUNC_ERASE_SCROLLBACK, out_status);
}

void nt_alt_screen_enable(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_ENTER, out_status);
}

void nt_alt_screen_disable(nt_status_t* out_status)
{
    _execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_EXIT, out_status);
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

    if(_color_is_default(color)) // DEFAULT COLOR
    {
        _execute_used_term_func(NT_ESC_FUNC_FG_SET_DEFAULT + func_offset,
                &_status);
        _VRETURN(out_status, _status);
    }

    switch(colors)
    {
        case NT_TERM_COLOR_TC:
            _execute_used_term_func(NT_ESC_FUNC_FG_SET_RGB + func_offset,
                    &_status, color._rgb.r, color._rgb.g, color._rgb.b);

            /* If func is not supported by the terminal, proceed to the
             * next case. */
            if(_status != NT_ERR_FUNC_NOT_SUPPORTED)
            {
                _VRETURN(out_status, _status);
            }

        case NT_TERM_COLOR_C256:
            _execute_used_term_func(NT_ESC_FUNC_FG_SET_C256 + func_offset,
                    &_status, color._code256);

            /* If func is not supported by the terminal, proceed to the
             * next case. */
            if(_status != NT_ERR_FUNC_NOT_SUPPORTED)
            {
                _VRETURN(out_status, _status);
            }

            _VRETURN(out_status, _status);

        case NT_TERM_COLOR_C8:
            _execute_used_term_func(NT_ESC_FUNC_FG_SET_C8 + func_offset,
                    &_status, color._code8);

            /* No more color palletes to fall back to. */
            _VRETURN(out_status, _status);

        default:
            _VRETURN(out_status, NT_ERR_UNEXPECTED);
    }
}

#define _SRETURN(out_style_param, out_style, out_status_param, out_status)    \
    if((out_status_param) != NULL)                                             \
        (*out_status_param) = out_status;                                      \
    if((out_style_param) != NULL)                                              \
        (*out_style_param) = out_style;                                        \
    return                                                                     \

/* Assumes styles are not set before calling. */
static void _set_style(nt_style_t style, nt_style_t* out_style,
        nt_status_t* out_status)
{
    const struct nt_term_info* used_term = nt_term_get_used();
    nt_status_t _status;

    if(used_term == NULL)
    {
        _SRETURN(out_style, NT_STYLE_DEFAULT, out_status, NT_ERR_TERM_UNKNOWN);
    }

    size_t i;
    size_t count = 8;
    nt_style_t used = NT_STYLE_DEFAULT;
    for(i = 0; i < count; i++)
    {
        if(style & (NT_STYLE_BOLD << i))
        {
            _execute_used_term_func(NT_ESC_FUNC_STYLE_SET_BOLD + i, &_status);
            if(_status != NT_SUCCESS)
            {
                if(_status != NT_ERR_FUNC_NOT_SUPPORTED)
                {
                    _SRETURN(out_style, used, out_status, _status);
                }
            }
            else used |= (NT_STYLE_BOLD << i);
        }
    }

    _SRETURN(out_style, used, out_status, NT_SUCCESS);
}

// UTF-32
void nt_write_char(uint32_t codepoint, struct nt_gfx gfx, ssize_t x, ssize_t y,
        nt_style_t* out_styles, nt_status_t* out_status)
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
            _SRETURN(out_styles, NT_STYLE_DEFAULT,
                    out_status, NT_ERR_INVALID_UTF32);
        case UC_ERR_INVALID_CODEPOINT:
            _SRETURN(out_styles, NT_STYLE_DEFAULT,
                    out_status, NT_ERR_INVALID_UTF32);
        default:
            _SRETURN(out_styles, NT_STYLE_DEFAULT,
                    out_status, NT_ERR_UNEXPECTED);
    }

    utf8[utf8_len] = '\0';

    nt_write_str(utf8, gfx, x, y, out_styles, out_status);
}

// UTF-8
void nt_write_str(const char* str, struct nt_gfx gfx, ssize_t x, ssize_t y,
        nt_style_t* out_styles, nt_status_t* out_status)
{
    nt_status_t _status;

    _execute_used_term_func(NT_ESC_FUNC_GFX_RESET, &_status);
    if(_status != NT_SUCCESS)
    {
        _SRETURN(out_styles, NT_STYLE_DEFAULT, out_status, _status);
    }

    if((x != NT_WRITE_INPLACE) || (y != NT_WRITE_INPLACE))
    {
        _execute_used_term_func(NT_ESC_FUNC_CURSOR_MOVE, &_status, y, x);
        if(_status != NT_SUCCESS)
        {
            _SRETURN(out_styles, NT_STYLE_DEFAULT, out_status, _status);
        }
    }

    _set_color(gfx.fg, SET_COLOR_FG, &_status);
    if(_status != NT_SUCCESS)
    {
        _SRETURN(out_styles, NT_STYLE_DEFAULT, out_status, _status);
    }

    _set_color(gfx.bg, SET_COLOR_BG, &_status);
    if(_status != NT_SUCCESS)
    {
        _SRETURN(out_styles, NT_STYLE_DEFAULT, out_status, _status);
    }

    nt_style_t used_styles;
    _set_style(gfx.style, &used_styles, &_status);
    if(_status != NT_SUCCESS)
    {
        _SRETURN(out_styles, used_styles, out_status, _status);
    }

    nt_charbuff_append(&_buff, str, &_status);
    switch(_status)
    {
        case NT_SUCCESS:
            break;
        case NT_ERR_ALLOC_FAIL:
            _SRETURN(out_styles, used_styles, out_status, NT_ERR_ALLOC_FAIL);
        default:
            _SRETURN(out_styles, used_styles, out_status, NT_ERR_UNEXPECTED);
    }

    if(!_buff_enabled)
        nt_buffer_flush();

    _SRETURN(out_styles, used_styles, out_status, NT_SUCCESS);
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
    int poll_status = nt_apoll(_poll_fds, 2, timeout);
    if(poll_status == -1)
    {
        _RETURN(_NT_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }
    if(poll_status == 0)
    {
        struct nt_timeout_event timeout_event = {
            .elapsed = timeout
        };

        struct nt_event ret = {
            .type = NT_EVENT_TYPE_TIMEOUT,
            .timeout_data = timeout_event
        };

        _RETURN(ret, out_status, NT_SUCCESS);
    }

    struct nt_event event;
    nt_status_t _status;
    if(_poll_fds[0].revents & POLLIN)
    {
        struct nt_key_event key_event = _process_key_event(&_status);
        event.type = NT_EVENT_TYPE_KEY;
        event.key_data = key_event;
    }
    else // (poll_fds[1].revents & POLLIN)
    {
        struct nt_resize_event resize_event = _process_resize_event(&_status);
        event.type = NT_EVENT_TYPE_RESIZE;
        event.resize_data = resize_event;
    }

    _RETURN(event, out_status, NT_SUCCESS);
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
        _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    if(buff[0] == 0x1b) // ESC or ESC SEQ or ALT + PRINTABLE
    {
        poll_status = nt_apoll(_poll_fds, 1, ESC_TIMEOUT);
        if(poll_status == -1)
        {
            _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
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

            _RETURN(key_event, out_status, NT_SUCCESS);
        }

        read_status = nt_aread(STDIN_FILENO, buff + 1, 1);
        if(read_status == -1)
        {
            _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
        }

        /* Probably ESC KEY, will be sure after poll() */
        if((buff[1] == '[') || (buff[1] == 'O'))
        {
            poll_status = nt_apoll(_poll_fds, 1, ESC_TIMEOUT);
            if(poll_status == -1)
            {
                _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
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
                _RETURN(key_event, out_status, NT_SUCCESS);
            }

            // ESC KEY

            struct nt_key_event key_event =
                _process_key_event_esc_key(buff, &_status);

            switch(_status)
            {
                case NT_SUCCESS:
                    _RETURN(key_event, out_status, NT_SUCCESS);
                default:
                    _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
            }

        }
        else
        {
            struct nt_key_event key_event =
                _process_key_event_utf32(buff + 1, true, &_status);

            switch(_status)
            {
                case NT_SUCCESS:
                    _RETURN(key_event, out_status, NT_SUCCESS);
                default:
                    _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
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
                _RETURN(key_event, out_status, NT_SUCCESS);
            default:
                _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
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
        _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    // read()
    int read_status = nt_aread(STDIN_FILENO, utf8_sbyte + 1, utf32_len - 1);
    if(read_status < 0)
    {
        _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }
    
    if(read_status != (utf32_len - 1))
    {
        _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED); // ?
    }

    uint32_t utf32;
    size_t utf32_width;
    uc_status_t _status;
    uc_utf8_to_utf32(utf8_sbyte, utf32_len, &utf32, 1, 0, &utf32_width, &_status);
    if(_status != UC_SUCCESS)
    {
        _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    struct nt_key_event key_event = {
        .type = NT_KEY_EVENT_UTF32,
        .utf32_data = {
            .codepoint = utf32,
            .alt = alt
        }
    };

    _RETURN(key_event, out_status, NT_SUCCESS);
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
            _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
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

            _RETURN(ret, out_status, NT_SUCCESS);
        }
    }

    ret = (struct nt_key_event) {
        .type = NT_KEY_EVENT_ESC_KEY,
        .esc_key_data = {
            .esc_key = NT_ESC_KEY_OTHER
        }
    };

    _RETURN(ret, out_status, NT_SUCCESS);
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
        _RETURN(_NT_RESIZE_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    nt_status_t _status;
    struct nt_xy term_size = nt_get_term_size(&_status);
    if(_status == NT_ERR_FUNC_NOT_SUPPORTED)
    {
        _RETURN(_NT_RESIZE_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    struct nt_resize_event resize_event = {
        .new_size = term_size
    };

    _RETURN(resize_event, out_status, NT_SUCCESS);
}
