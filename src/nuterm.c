#include "nuterm.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "_nt_term.h"
#include "_uconv.h"
#include "nt_esc_defs.h"

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

#define _RETURN(ret_val, out_status_param, out_status)                         \
    if((out_status_param) != NULL)                                             \
        (*out_status_param) = (out_status);                                    \
    return (ret_val)                                                           \

#define _VRETURN(out_status_param, out_status)                                 \
    if((out_status_param) != NULL)                                             \
        (*out_status_param) = out_status;                                      \
    return                                                                     \

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

/* -------------------------------------------------------------------------- */
/* GENERAL */
/* -------------------------------------------------------------------------- */

static struct nt_xy _term_size = {0};
static int _resize_fds[2];
static struct pollfd _poll_fds[2];
static struct termios _init_term_opts;

static void _sa_handler(int signum)
{
    char c;
    _awrite(_resize_fds[1], &c, 1);
}

void nt_init(nt_status_t* out_status)
{
    struct termios raw_term_opts;
    cfmakeraw(&raw_term_opts);

    tcgetattr(STDIN_FILENO, &_init_term_opts);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_term_opts);

    struct sigaction sw_sa = {0};
    sw_sa.sa_handler = _sa_handler;
    sigaction(SIGWINCH, &sw_sa, NULL);

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

    nt_term_init();

    struct nt_xy term_size;
    int status = _get_term_size(&term_size);
    if(status == -1)
    {
        _VRETURN(out_status, NT_ERR_UNEXPECTED);
    }

    _term_size = term_size;

    _VRETURN(out_status, NT_SUCCESS);
}

void nt_destroy()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &_init_term_opts);
}

/* -------------------------------------------------------------------------- */
/* NT EVENT */
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

/* -------------------------------------------------------------------------- */
/* KEY EVENT */
/* -------------------------------------------------------------------------- */

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

/* -------------------------------------------------------------------------- */

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
                .codepoint = 27,
                .alt = false,
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
                    .alt = true,
                    .codepoint = buff[1]
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
        .codepoint = utf32,
        .alt = alt
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

    size_t i;
    struct nt_key_event ret;
    const struct nt_term* term = nt_term_get_used();
    for(i = 0; i < _NT_ESC_KEY_COUNT; i++)
    {
        if(strcmp(_buff, term->esc_key_seqs[i]) == 0)
        {
            ret = (struct nt_key_event) {
                .type = NT_KEY_EVENT_ESC_KEY,
                .codepoint = nt_esc_key_to_codepoint(i),
                .alt = false
            };

            _RETURN(ret, out_status, NT_SUCCESS);
        }
    }

    ret = (struct nt_key_event) {
        .type = NT_KEY_EVENT_ESC_KEY,
        .codepoint = NT_ESC_CODEPOINT_UNKNOWN,
        .alt = false
    };

    _RETURN(ret, out_status, NT_SUCCESS);
}

/* -------------------------------------------------------------------------- */
/* RESIZE EVENT */
/* -------------------------------------------------------------------------- */

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
        .old_size = _term_size,
        .new_size = term_size
    };

    _term_size = term_size;

    _RETURN(resize_event, out_status, NT_SUCCESS);
}
