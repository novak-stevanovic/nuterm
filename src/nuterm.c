#include "nuterm.h"

#include <errno.h>
#include <math.h>
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

/* -------------------------------------------------------------------------- */
/* HELPER */
/* -------------------------------------------------------------------------- */

static inline int _get_term_size(struct nt_xy* out_xy)
{
    struct winsize size;
    int status = ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
    if(status == -1) return -1;

    out_xy->x = size.ws_col;
    out_xy->y = size.ws_row;
    return 0;
}

/* -------------------------------------------------------------------------- */

static inline int _aread(int fd, void* dest, size_t count)
{
    int status;
    do
    {
        status = read(fd, dest, count);
    } while((status == -1) && (errno == EINTR));

    return status;
}

static inline int _apoll(struct pollfd pollfds[], size_t count, size_t timeout)
{
    int status;
    do
    {
        status = poll(pollfds, count, timeout);
    } while((status == -1) && (errno == EINTR));

    return status;
}

static inline int _awrite(int fd, const void* src, size_t count)
{
    int status;
    do
    {
        status = write(fd, src, count);
    } while((status == -1) && (errno == EINTR));

    return status;
}

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
    _awrite(_resize_fds[1], &c, 1);
}

void nt_init(nt_status_t* out_status)
{
    nt_status_t _status;
    int status;
    struct termios raw_term_opts;
    cfmakeraw(&raw_term_opts);

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
        _VRETURN(out_status, NT_ERR_PIPE);
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
        case NT_ERR_TERM_INIT_ENV_FAIL:
            _VRETURN(out_status, NT_ERR_TERM_INIT_ENV_FAIL);
        default:
            _VRETURN(out_status, NT_ERR_UNEXPECTED);
    }

    struct nt_xy term_size;
    status = _get_term_size(&term_size);
    if(status == -1)
    {
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
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &_init_term_opts);
    nt_term_destroy();
    nt_charbuff_destroy(&_buff);
}

/* -------------------------------------------------------------------------- */
/* COLOR & STYLE */
/* -------------------------------------------------------------------------- */

static uint8_t _rgb_to_c8(uint8_t r, uint8_t g, int8_t b)
{
    // TODO
    //
    return 1;
}

static uint8_t _rgb_to_c256(uint8_t r, uint8_t g, int8_t b)
{
    // TODO
    if((r == g) && (g == b)) // gray
    {
        uint8_t val_adj = floor(r / 10.667);

    }

    uint8_t r_adj = floor(r / 42.6667);
    uint8_t g_adj = floor(g / 42.6667);
    uint8_t b_adj = floor(b / 42.6667);

    return 16 + (36 * r_adj) + 6 * (6 * g_adj) + b_adj;
}

nt_color_t nt_color_create(uint8_t r, uint8_t g, int8_t b)
{
    return (nt_color_t) {
        ._color_rgb = (struct nt_rgb) { .r = r, .g = g, .b = b },
        ._color_c256 = _rgb_to_c256(r, g, b),
        ._color_c8 = _rgb_to_c8(r, g, b)
    };
}

bool nt_color_are_equal(nt_color_t c1, nt_color_t c2)
{
    return (memcmp(&c1, &c2, sizeof(nt_color_t)) == 0);
}

bool nt_style_are_equal(nt_style_t s1, nt_style_t s2)
{
    return (s1 == s2);
}

/* -------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* -------------------------------------------------------------------------- */

static void _execute_used_term_func(nt_esc_func_t func, nt_status_t* out_status, ...)
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

    if(_buff_enabled == true)
    {
        nt_charbuff_append(&_buff, buff, &_status);
        if(_status == NT_ERR_ALLOC_FAIL)
        {
            _VRETURN(out_status, NT_ERR_ALLOC_FAIL);
        }
    }
    else
    {
        int write_status = _awrite(STDIN_FILENO, buff, strlen(buff));
        if(write_status == -1)
        {
            _VRETURN(out_status, NT_ERR_UNEXPECTED);
        }
    }

    _VRETURN(out_status, NT_SUCCESS);
}

static inline void _cursor_show(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_CURSOR_SHOW, &_status);
    _VRETURN(out_status, _status);
}

static inline void _cursor_hide(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_CURSOR_HIDE, &_status);
    _VRETURN(out_status, _status);
}

static inline void _cursor_move(size_t x, size_t y, nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_CURSOR_MOVE, &_status, x, y);
    _VRETURN(out_status, _status);
}

static inline void _fg_set_c8(uint8_t code, nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_FG_SET_C8, &_status, code);
    _VRETURN(out_status, _status);
}

static inline void _fg_set_c256(uint8_t code, nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_FG_SET_C256, &_status, code);
    _VRETURN(out_status, _status);
}

static inline void _fg_set_rgb(struct nt_rgb rgb, nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_FG_SET_RGB, &_status, rgb.r, rgb.g, rgb.b);
    _VRETURN(out_status, _status);
}

static inline void _bg_set_c8(uint8_t code, nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_C8, &_status, code);
    _VRETURN(out_status, _status);
}

static inline void _bg_set_c256(uint8_t code, nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_C256, &_status, code);
    _VRETURN(out_status, _status);
}

static inline void _bg_set_rgb(struct nt_rgb rgb, nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_BG_SET_RGB, &_status, rgb.r, rgb.g, rgb.b);
    _VRETURN(out_status, _status);
}

static void _gfx_reset(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_GFX_RESET, &_status);
    _VRETURN(out_status, _status);
}

static inline void _erase_screen(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_ERASE_SCREEN, &_status);
    _VRETURN(out_status, _status);
}

static inline void _erase_line(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_ERASE_LINE, &_status);
    _VRETURN(out_status, _status);
}

static inline void _erase_scrollback(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_ERASE_SCROLLBACK, &_status);
    _VRETURN(out_status, _status);
}

static inline void _alt_screen_enable(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_ENTER, &_status);
    _VRETURN(out_status, _status);
}

static inline void _alt_screen_disable(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_EXIT, &_status);
    _VRETURN(out_status, _status);
}

static inline void _style_set_bold(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_STYLE_SET_BOLD, &_status);
    _VRETURN(out_status, _status);
}

static inline void _style_set_faint(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_STYLE_SET_FAINT, &_status);
    _VRETURN(out_status, _status);
}

static inline void _style_set_italic(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_STYLE_SET_ITALIC, &_status);
    _VRETURN(out_status, _status);
}

static inline void _style_set_underline(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_STYLE_SET_UNDERLINE, &_status);
    _VRETURN(out_status, _status);
}

static void _style_set_blink(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_STYLE_SET_BLINK, &_status);
    _VRETURN(out_status, _status);
}

static inline void _style_set_hidden(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_STYLE_SET_HIDDEN, &_status);
    _VRETURN(out_status, _status);
}

static inline void _style_set_strikethrough(nt_status_t* out_status)
{
    nt_status_t _status;
    _execute_used_term_func(NT_ESC_FUNC_STYLE_SET_STRIKETHROUGH, &_status);
    _VRETURN(out_status, _status);
}

/* -------------------------------------------------------------------------- */

void nt_buffer_enable()
{
    _buff_enabled = true;
}

void nt_buffer_flush()
{
    // Assume write() doesn't fail.
    _awrite(STDOUT_FILENO, _buff.data, _buff.len);

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
    _cursor_hide(out_status);
}

void nt_cursor_show(nt_status_t* out_status)
{
    _cursor_show(out_status);
}

void nt_erase_screen(nt_status_t* out_status)
{
    _erase_screen(out_status);
}

void nt_erase_line(nt_status_t* out_status)
{
    _erase_line(out_status);
}

void nt_erase_scrollback(nt_status_t* out_status)
{
    _erase_scrollback(out_status);
}

void nt_alt_screen_enable(nt_status_t* out_status)
{
    _alt_screen_enable(out_status);
}

void nt_alt_screen_disable(nt_status_t* out_status)
{
    _alt_screen_disable(out_status);
}

/* ------------------------------------------------------------------------- */
/* WRITE TO TERMINAL */
/* ------------------------------------------------------------------------- */

typedef enum set_color_opt {
    SET_COLOR_FG,
    SET_COLOR_BG
} set_color_opt_t;

static void _set_color_fg(nt_color_t color, set_color_opt_t opt,
        nt_status_t* out_status)
{
    nt_status_t _status;
    nt_term_color_count_t colors = nt_term_get_color_count();

    size_t func_offset = (opt == SET_COLOR_BG) ?
        (NT_ESC_FUNC_BG_SET_C8 - NT_ESC_FUNC_FG_SET_C8) : 0;

    nt_esc_func_t func;
    switch(colors)
    {
        case NT_TERM_COLOR_TC:
            func = NT_ESC_FUNC_FG_SET_RGB + func_offset;

            _execute_used_term_func(func, &_status, color._color_rgb);

            /* If func is not supported by the terminal, proceed to the
             * next case. */
            if(_status != NT_ERR_FUNC_NOT_SUPPORTED)
            {
                _VRETURN(out_status, _status);
            }

        case NT_TERM_COLOR_C256:
            func = NT_ESC_FUNC_FG_SET_C256 + func_offset;

            _execute_used_term_func(func, &_status, color._color_c256);

            /* If func is not supported by the terminal, proceed to the
             * next case. */
            if(_status != NT_ERR_FUNC_NOT_SUPPORTED)
            {
                _VRETURN(out_status, _status);
            }

            _VRETURN(out_status, _status);

        case NT_TERM_COLOR_C8:
            func = NT_ESC_FUNC_FG_SET_C8 + func_offset;

            _execute_used_term_func(func, &_status, color._color_c8);

            /* No more color palletes to fall back to. */
            _VRETURN(out_status, _status);

        default:
            _VRETURN(out_status, NT_ERR_UNEXPECTED);
    }
}

static void _set_style(nt_style_t style, nt_status_t* out_status)
{
}

// UTF-32
void nt_write_char(uint32_t codepoint, struct nt_gfx gfx, size_t x, size_t y)
{
}

// UTF-8
void nt_write_str(const char* str, struct nt_gfx gfx, size_t x, size_t y)
{
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
    int poll_status = _apoll(_poll_fds, 2, timeout);
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

    read_status = _aread(STDIN_FILENO, buff, 1);
    if(read_status < 0)
    {
        _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    if(buff[0] == 0x1b) // ESC or ESC SEQ or ALT + PRINTABLE
    {
        poll_status = _apoll(_poll_fds, 1, ESC_TIMEOUT);
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

        read_status = _aread(STDIN_FILENO, buff + 1, 1);
        if(read_status == -1)
        {
            _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
        }

        /* Probably ESC KEY, will be sure after poll() */
        if((buff[1] == '[') || (buff[1] == 'O'))
        {
            poll_status = _apoll(_poll_fds, 1, ESC_TIMEOUT);
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
    int read_status = _aread(STDIN_FILENO, utf8_sbyte + 1, utf32_len - 1);
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
        read_status = _aread(STDIN_FILENO, buff + read_count, 1);
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
    int read_status = _aread(_resize_fds[0], buff, 1);
    if(read_status == -1)
    {
        _RETURN(_NT_RESIZE_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    struct nt_xy term_size;
    int status = _get_term_size(&term_size);
    if(status == -1)
    {
        _RETURN(_NT_RESIZE_EVENT_EMPTY, out_status, NT_ERR_UNEXPECTED);
    }

    struct nt_resize_event resize_event = {
        .new_size = term_size
    };

    _RETURN(resize_event, out_status, NT_SUCCESS);
}
