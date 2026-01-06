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
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "uconv.h"
#include "nt_internal.h"

#define STDIN_POLL_FD 0
#define RESIZE_POLL_FD 1
#define SIGNAL_POLL_FD 2
#define CUSTOM_POLL_FD 3
#define POLL_FD_COUNT 4

/* ------------------------------------------------------------------------- */
/* GENERAL */
/* ------------------------------------------------------------------------- */

static pthread_t sigthread = 0;
static pthread_mutex_t sigthread_lock;
static bool sigthread_stop = false;

static int signal_pipe[2];
static int resize_pipe[2];
static int custom_event_pipe[2];
static struct pollfd poll_fds[POLL_FD_COUNT];
static struct termios init_term_opts;

static char* stdout_buff = NULL;
static size_t stdout_buff_pos = 0;
static size_t stdout_buff_cap = 0;

static void inline write_to_stdout(const char* str, size_t str_len)
{
    if(str_len == 0) return;

    if(stdout_buff == NULL) // not buffered
    {
        write(STDOUT_FILENO, str, str_len);
        return;
    }

    if(stdout_buff_pos + str_len <= stdout_buff_cap)
    {
        memcpy(stdout_buff + stdout_buff_pos, str, str_len);
        stdout_buff_pos += str_len;
    }
    else
    {
        write(STDOUT_FILENO, stdout_buff, stdout_buff_pos);
        stdout_buff_pos = 0;

        if(str_len <= stdout_buff_cap)
        {
            memcpy(stdout_buff, str, str_len);
            stdout_buff_pos = str_len;
        }
        else
        {
            write(STDOUT_FILENO, str, str_len);
        }
    }
}

static void* sigthread_fn(void* data)
{
    sigset_t set;
    sigfillset(&set);
    int signal = 0;
    unsigned int usignal;
    while(true)
    {
        pthread_mutex_lock(&sigthread_lock);
        if(sigthread_stop)
        {
            pthread_mutex_unlock(&sigthread_lock);
            break;
        }
        else pthread_mutex_unlock(&sigthread_lock);

        sigwait(&set, &signal);

        if(signal == SIGWINCH)
        {
            write(resize_pipe[1], &usignal, sizeof(unsigned int));
        }

        usignal = (unsigned int)signal;
        write(signal_pipe[1], &usignal, sizeof(unsigned int));
    }

    return NULL;
}

static inline void term_opts_raw(struct termios* term_opts)
{
    term_opts->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
            | INLCR | IGNCR | ICRNL | IXON);
    term_opts->c_oflag &= ~OPOST;
    term_opts->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    term_opts->c_cflag &= ~(CSIZE | PARENB);
    term_opts->c_cflag |= CS8;

    term_opts->c_cc[VMIN] = 1;
    term_opts->c_cc[VTIME] = 0;
}

void nt_init(nt_status* out_status)
{
    nt_status _status;
    int status;

    status = tcgetattr(STDIN_FILENO, &init_term_opts);
    if(status == -1)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return;
    }

    struct termios raw_opts = init_term_opts;
    term_opts_raw(&raw_opts);
    status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_opts);
    if(status == -1)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return;
    }

    int pipe_status;
    pipe_status = pipe(signal_pipe);
    if(pipe_status != 0)
    {
        SET_OUT(out_status, NT_ERR_INIT_PIPE);
        return;
    }

    pipe_status = pipe(custom_event_pipe);
    if(pipe_status != 0)
    {
        SET_OUT(out_status, NT_ERR_INIT_PIPE);
        return;
    }

    pipe_status = pipe(resize_pipe);
    if(pipe_status != 0)
    {
        SET_OUT(out_status, NT_ERR_INIT_PIPE);
        return;
    }

    poll_fds[STDIN_POLL_FD] = (struct pollfd) {
        .fd = STDIN_FILENO,
        .events = POLLIN,
        .revents = 0
    };
    poll_fds[RESIZE_POLL_FD] = (struct pollfd) {
        .fd = resize_pipe[0],
        .events = POLLIN,
        .revents = 0
    };
    poll_fds[SIGNAL_POLL_FD] = (struct pollfd) {
        .fd = signal_pipe[0],
        .events = POLLIN,
        .revents = 0
    };
    poll_fds[CUSTOM_POLL_FD] = (struct pollfd) {
        .fd = custom_event_pipe[0],
        .events = POLLIN,
        .revents = 0
    };

    sigset_t set;
    sigfillset(&set);
    int mask_status = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if(mask_status != 0)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return;
    }

    int thread_status = pthread_create(&sigthread, NULL, sigthread_fn, NULL);
    if(thread_status != 0)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return;
    }

    int lock_status = pthread_mutex_init(&sigthread_lock, NULL);
    if(lock_status != 0)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return;
    }

    _nt_term_init(&_status);
    switch(_status)
    {
        case NT_SUCCESS:
            SET_OUT(out_status, NT_SUCCESS);
            return;
        case NT_ERR_TERM_NOT_SUPP:
            SET_OUT(out_status, NT_ERR_TERM_NOT_SUPP);
            return;
        case NT_ERR_INIT_TERM_ENV:
            SET_OUT(out_status, NT_ERR_INIT_TERM_ENV);
            return;
        default:
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return;
    }
}

void nt_deinit()
{
    pthread_mutex_lock(&sigthread_lock);
    sigthread_stop = true;
    pthread_mutex_unlock(&sigthread_lock);
    kill(0, SIGINT);

    pthread_join(sigthread, NULL);

    stdout_buff = NULL;
    stdout_buff_cap = 0;
    stdout_buff_pos = 0;

    nt_status _status;
    nt_write_str("", 0, NT_GFX_DEFAULT, &_status);
    assert(_status == NT_SUCCESS);

    pthread_mutex_destroy(&sigthread_lock);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &init_term_opts);
    _nt_term_deinit();
}

/* -------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* -------------------------------------------------------------------------- */

static void execute_used_term_func(enum nt_esc_func func,
        bool use_va, nt_status* out_status, ...)
{
    int status;

    struct nt_term_info used_term = _nt_term_get_used();

    const char* esc_func = used_term.esc_func_seqs[func];
    if(esc_func == NULL)
    {
        SET_OUT(out_status, NT_ERR_FUNC_NOT_SUPP);
        return;
    }

    const char* _func;
    if(use_va)
    {
        char buff[100];

        va_list list;
        va_start(list, out_status);

        status = vsprintf(buff, esc_func, list);
        if(status < 0)
        {
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return;
        }

        va_end(list);

        _func = buff;
    }
    else _func = esc_func;

    write_to_stdout(_func, strlen(_func));

    SET_OUT(out_status, NT_SUCCESS);
}

/* -------------------------------------------------------------------------- */

void nt_buffer_enable(char* buff, size_t cap, nt_status* out_status)
{
    if((buff == NULL) || (cap == 0))
    {
        SET_OUT(out_status, NT_ERR_INVALID_ARG);
        return;
    }

    if(stdout_buff != NULL)
    {
        SET_OUT(out_status, NT_ERR_ALR_BUFF);
        return;
    }

    stdout_buff = buff;
    stdout_buff_cap = cap;
    stdout_buff_pos = 0;

    SET_OUT(out_status, NT_SUCCESS);
}

char* nt_buffer_disable(nt_buffact buffact)
{
    if(stdout_buff != NULL)
    {
        if((buffact == NT_BUFF_FLUSH) && (stdout_buff_pos > 0))
            write(STDOUT_FILENO, stdout_buff, stdout_buff_pos);

        stdout_buff = NULL;
        stdout_buff_pos = 0;
        stdout_buff_cap = 0;
    }

    return stdout_buff;
}

void nt_buffer_flush()
{
    if((stdout_buff != NULL) && (stdout_buff_pos > 0))
    {
        write(STDOUT_FILENO, stdout_buff, stdout_buff_pos);
        stdout_buff_pos = 0;
    }
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
    execute_used_term_func(NT_ESC_FUNC_CURSOR_HIDE, false, out_status);
}

void nt_cursor_show(nt_status* out_status)
{
    execute_used_term_func(NT_ESC_FUNC_CURSOR_SHOW, false, out_status);
}

void nt_erase_screen(nt_status* out_status)
{
    execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false, NULL);
    execute_used_term_func(NT_ESC_FUNC_ERASE_SCREEN, false, out_status);
}

void nt_erase_line(nt_status* out_status)
{
    execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false, NULL);
    execute_used_term_func(NT_ESC_FUNC_ERASE_LINE, false, out_status);
}

void nt_erase_scrollback(nt_status* out_status)
{
    execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false, NULL);
    execute_used_term_func(NT_ESC_FUNC_ERASE_SCROLLBACK, false, out_status);
}

void nt_alt_screen_enable(nt_status* out_status)
{
    execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_ENTER, false, out_status);
}

void nt_alt_screen_disable(nt_status* out_status)
{
    execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_EXIT, false, out_status);
}

void nt_mouse_mode_enable(nt_status* out_status)
{
    execute_used_term_func(NT_ESC_FUNC_MOUSE_ENABLE, false, out_status);
}

void nt_mouse_mode_disable(nt_status* out_status)
{
    execute_used_term_func(NT_ESC_FUNC_MOUSE_DISABLE, false, out_status);
}

/* ------------------------------------------------------------------------- */
/* WRITE TO TERMINAL */
/* ------------------------------------------------------------------------- */


/* This function assumes:
 * 1) The terminal has the capability to set default fg and bg colors.
 * 2) If the terminal supports RGB, then the library holds the terminal's
 * esc sequence to set the RGB color for bg/fg. Same with 256 colors and
 * 8 colors */
static void set_gfx(struct nt_gfx gfx, nt_status* out_status)
{
    nt_status _status;
    nt_term_color_count colors = _nt_term_get_color_count();

    /* Set foreground --------------------------------------------------- */

    if(nt_color_are_equal(NT_COLOR_DEFAULT, gfx.fg))
    {
        execute_used_term_func(NT_ESC_FUNC_FG_SET_DEFAULT, false, &_status);
    }
    else
    {
        if(colors == NT_TERM_COLOR_TC)
        {
            execute_used_term_func(NT_ESC_FUNC_FG_SET_RGB, true,
                    &_status, gfx.fg._rgb.r, gfx.fg._rgb.g, gfx.fg._rgb.b);
        }
        else if(colors == NT_TERM_COLOR_C256)
        {
            execute_used_term_func(NT_ESC_FUNC_FG_SET_C256, true,
                    &_status, gfx.fg._code256);
        }
        else if(colors == NT_TERM_COLOR_C8)
        {
            execute_used_term_func(NT_ESC_FUNC_FG_SET_C8, true,
                    &_status, gfx.fg._code8);
        }
        else
        {
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return;
        }
    }

    if(_status != NT_SUCCESS)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return;
    }

    /* Set background --------------------------------------------------- */

    if(nt_color_are_equal(NT_COLOR_DEFAULT, gfx.bg))
    {
        execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false, &_status);
    }
    else
    {
        if(colors == NT_TERM_COLOR_TC)
        {
            execute_used_term_func(NT_ESC_FUNC_BG_SET_RGB, true,
                    &_status, gfx.bg._rgb.r, gfx.bg._rgb.g, gfx.bg._rgb.b);
        }
        else if(colors == NT_TERM_COLOR_C256)
        {
            execute_used_term_func(NT_ESC_FUNC_BG_SET_C256, true,
                    &_status, gfx.bg._code256);
        }
        else if(colors == NT_TERM_COLOR_C8)
        {
            execute_used_term_func(NT_ESC_FUNC_BG_SET_C8, true,
                    &_status, gfx.bg._code8);
        }
        else 
        {
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return;
        }
    }

    if(_status != NT_SUCCESS)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return;
    }

    /* Set style -------------------------------------------------------- */

    uint8_t style;

    if(colors == NT_TERM_COLOR_TC)
        style = gfx.style._value_c8;
    else if(colors == NT_TERM_COLOR_C256)
        style = gfx.style._value_c256;
    else if(colors == NT_TERM_COLOR_C8)
        style = gfx.style._value_rgb;
    else 
    { 
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return;
    }

    size_t i;
    size_t count = 8;
    for(i = 0; i < count; i++)
    {
        if(style & (NT_STYLE_VAL_BOLD << i))
        {
            execute_used_term_func(NT_ESC_FUNC_STYLE_SET_BOLD + i, true, &_status);
            if((_status != NT_SUCCESS) && (_status != NT_ERR_FUNC_NOT_SUPP))
            {
                SET_OUT(out_status, _status);
                return;
            }
        }
    }

    SET_OUT(out_status, NT_SUCCESS);
    return;
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
            SET_OUT(out_status, NT_ERR_INVALID_UTF32);
            return;
        case UC_ERR_INVALID_CODEPOINT:
            SET_OUT(out_status, NT_ERR_INVALID_UTF32);
            return;
        default:
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return;
    }

    utf8[utf8_len] = '\0';

    nt_write_str(utf8, utf8_len, gfx, out_status);
}

// UTF-8
void nt_write_str(const char* str, size_t len,
        struct nt_gfx gfx, nt_status* out_status)
{
    nt_status _status;

    execute_used_term_func(NT_ESC_FUNC_GFX_RESET, false, &_status);
    if(_status != NT_SUCCESS)
    {
        SET_OUT(out_status, _status);
        return;
    }

    set_gfx(gfx, &_status);
    if(_status != NT_SUCCESS)
    {
        SET_OUT(out_status, _status);
        return;
    }

    /* In some terminals, a newline will fill the next row with currently set bg.
     * To avoid this, any time we run into a newline, we will reset the gfx,
     * print it in default GFX, and then resume printing */
    if(len > 0) 
    {
        const char *it_begin = str, *it_end;

        while(true)
        {
            it_end = memchr(it_begin, '\n', len);
            
            if(it_end != NULL)
            {
                write_to_stdout(it_begin, it_end - it_begin);

                execute_used_term_func(NT_ESC_FUNC_GFX_RESET, false, &_status);
                if(_status != NT_SUCCESS)
                {
                    SET_OUT(out_status, _status);
                    return;
                }

                write_to_stdout("\n", 1);

                set_gfx(gfx, &_status);
                if(_status != NT_SUCCESS)
                {
                    SET_OUT(out_status, _status);
                    return;
                }

                if(it_end < (str + len - 1))
                    it_begin = (it_end + 1);
                else
                    break;
            }
            else
            {
                write_to_stdout(it_begin, (str + len) - it_begin);
                break;
            }

        }
    }

    SET_OUT(out_status, NT_SUCCESS);
}

void nt_write_char_at(uint32_t codepoint, struct nt_gfx gfx, size_t x, size_t y,
        nt_status* out_status)
{
    size_t _width, _height;
    nt_get_term_size(&_width, &_height);

    if((x >= _width) || (y >= _height))
    {
        SET_OUT(out_status, NT_ERR_OUT_OF_BOUNDS);
        return;
    }

    nt_status _status;

    execute_used_term_func(NT_ESC_FUNC_CURSOR_MOVE, true, &_status, y + 1, x + 1);
    if(_status != NT_SUCCESS)
    {
        SET_OUT(out_status, _status);
        return;
    }

    nt_write_char(codepoint, gfx, out_status);
}

void nt_write_str_at(const char* str, size_t len, struct nt_gfx gfx,
        size_t x, size_t y, nt_status* out_status)
{
    size_t _width, _height;
    nt_get_term_size(&_width, &_height);

    if((x >= _width) || (y >= _height))
    {
        SET_OUT(out_status, NT_ERR_OUT_OF_BOUNDS);
        return;
    }

    nt_status _status;

    execute_used_term_func(NT_ESC_FUNC_CURSOR_MOVE, true, &_status, y + 1, x + 1);
    if(_status != NT_SUCCESS)
    {
        SET_OUT(out_status, _status);
        return;
    }

    nt_write_str(str, len, gfx, out_status);
}

/* -------------------------------------------------------------------------- */
/* EVENT */
/* -------------------------------------------------------------------------- */

struct nt_event_header
{
    uint8_t type;
    uint8_t data_size;
};

/* Called by nt_event_wait() internally. */
static struct nt_key_event process_key_event(nt_status* out_status);

/* -------------------------------------------------------------------------- */

static const struct nt_event NT_EVENT_EMPTY = {0};

unsigned int nt_event_wait(struct nt_event* out_event,
        unsigned int timeout, nt_status* out_status)
{
    struct timespec _time1, _time2;
    clock_gettime(CLOCK_REALTIME, &_time1);

    int poll_status = poll(poll_fds, 4, timeout);

    clock_gettime(CLOCK_REALTIME, &_time2);

    unsigned long elapsed = ((_time2.tv_sec - _time1.tv_sec) * 1e3) +
        ((_time2.tv_nsec - _time1.tv_nsec) / 1e6);

    elapsed = (elapsed <= timeout) ? elapsed : timeout;

    struct nt_event event = NT_EVENT_EMPTY;

    if(poll_status == -1)
    {
        SET_OUT(out_event, event);
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return elapsed;
    }

    if(poll_status == 0)
    {
        event.type = NT_EVENT_TIMEOUT;
        event.custom = false;
        event.data_size = 0;

        SET_OUT(out_event, event);
        SET_OUT(out_status, NT_SUCCESS);
        return elapsed;
    }

    nt_status _status;
    if(poll_fds[STDIN_POLL_FD].revents & POLLIN)
    {
        event.type = NT_EVENT_KEY;
        event.custom = false;
        event.data_size = sizeof(struct nt_key_event);

        struct nt_key_event key = process_key_event(&_status);
        if(_status != NT_SUCCESS)
        {
            SET_OUT(out_event, event);
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return elapsed;
        }
        memcpy(event.data, &key, sizeof(struct nt_key_event));
    }
    else if(poll_fds[RESIZE_POLL_FD].revents & POLLIN)
    {
    }
    else if(poll_fds[SIGNAL_POLL_FD].revents & POLLIN)
    {
        event.type = NT_EVENT_SIGNAL;
        event.custom = false;
        event.data_size = sizeof(unsigned int);

        unsigned int buff = 0;
        int read_status = read(signal_pipe[0], &buff, sizeof(unsigned int));
        if(read_status == -1)
        {
            SET_OUT(out_event, event);
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return elapsed;
        }

        memcpy(event.data, &buff, sizeof(unsigned int));
    }
    else // poll_fds[CUSTOM_POLL_FD].revents & POLLIN
    {
        // Read header to determine type and data_size
        struct nt_event_header header = {0};
        int read_status = read(
                custom_event_pipe[0],
                &header,
                sizeof(struct nt_event_header));

        if(read_status == -1)
        {
            SET_OUT(out_event, event);
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return elapsed;
        }

        event.data_size = header.data_size;
        event.type = header.type;
        event.custom = true;
        
        // If event has data, read it
        if(header.data_size > 0)
        {
            uint8_t buff[NT_EVENT_DATA_MAX_SIZE] = {0};
            read_status = read(custom_event_pipe[0], buff, header.data_size);
            if(read_status == -1)
            {
                SET_OUT(out_event, event);
                SET_OUT(out_status, NT_ERR_UNEXPECTED);
                return elapsed;
            }

            memcpy(event.data, buff, header.data_size);
        }

    }

    SET_OUT(out_event, event);
    SET_OUT(out_status, NT_SUCCESS);
    return elapsed;
}

void nt_event_push(uint32_t type, void* data, uint8_t data_size, nt_status* out_status)
{
    if((type == NT_EVENT_INVALID) || ((type & (type - 1)) != 0))
    {
        SET_OUT(out_status, NT_ERR_INVALID_ARG);
        return;
    }

    if((data_size > NT_EVENT_DATA_MAX_SIZE) || ((data_size > 0) && (data == NULL)))
    {
        SET_OUT(out_status, NT_ERR_INVALID_ARG);
        return;
    }

    // Prepare buffer for writing
    uint8_t buff[sizeof(struct nt_event_header) + NT_EVENT_DATA_MAX_SIZE] = {0};
    buff[0] = type;
    buff[1] = data_size;

    // If there's data, write it to buffer
    if(data_size > 0) memcpy(buff + 2, data, data_size);

    // Write to the pipe
    int write_status = write(
            custom_event_pipe[1],
            buff,
            sizeof(struct nt_event_header) + data_size);
    if(write_status == -1)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return;
    }

    SET_OUT(out_status, NT_SUCCESS);
}

/* ------------------------------------------------------ */
/* KEY EVENT */
/* ------------------------------------------------------ */

#define ESC_TIMEOUT 2

static const struct nt_key_event NT_KEY_EMPTY = {0};

/* Called by _process_key_event() internally.
 * `utf8_sbyte` - ptr to first byte of utf8.
 * Other bytes will be read into `utf8_sbyte` in the function. */
static struct nt_key_event process_key_event_utf32(uint8_t* utf8_sbyte,
        bool alt, nt_status* out_status);

enum process_esc_key_rv_type { ESC_KEY, MOUSE_CLICK };

struct process_esc_key_rv
{
    enum process_esc_key_rv_type type;
    // union
    // {
    //     struct 
    // };
};

/* Called by _process_key_event() internally.
 * `buff` - ptr to the first byte of the ESC sequence(ESC char).
 * The whole escape sequences will be read into the buffer inside the function. */
static struct nt_key_event process_key_event_esc_key(uint8_t* buff,
        nt_status* out_status);

/* ------------------------------------------------------ */

static struct nt_key_event process_key_event(nt_status* out_status)
{
    unsigned char buff[64];
    int poll_status, read_status;
    nt_status _status;

    read_status = read(STDIN_FILENO, buff, 1);
    if(read_status < 0)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return NT_KEY_EMPTY;
    }

    if(buff[0] == 0x1b) // ESC or ESC SEQ or ALT + PRINTABLE
    {
        poll_status = poll(poll_fds, 1, ESC_TIMEOUT);
        if(poll_status == -1)
        {
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return NT_KEY_EMPTY;
        }

        if(poll_status == 0) // timeout - just ESC
        {
            struct nt_key_event key_event = {
                .type = NT_KEY_EVENT_UTF32,
                .utf32 = { .cp = 27, .alt = false }
            };

            SET_OUT(out_status, NT_SUCCESS);
            return key_event;
        }

        read_status = read(STDIN_FILENO, buff + 1, 1);
        if(read_status == -1)
        {
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return NT_KEY_EMPTY;
        }

        /* Probably ESC KEY, will be sure after poll() */
        if((buff[1] == '[') || (buff[1] == 'O'))
        {
            poll_status = poll(poll_fds, 1, ESC_TIMEOUT);
            if(poll_status == -1)
            {
                SET_OUT(out_status, NT_ERR_UNEXPECTED);
                return NT_KEY_EMPTY;
            }

            if(poll_status == 0) // Not in fact an ESC key...
            {
                struct nt_key_event key_event = {
                    .type = NT_KEY_EVENT_UTF32,
                    .utf32 = {
                        .cp = buff[1],
                        .alt = true,
                    }
                };
                SET_OUT(out_status, NT_SUCCESS);
                return key_event;
            }

            // ESC KEY

            struct nt_key_event key_event = process_key_event_esc_key(buff, &_status);

            switch(_status)
            {
                case NT_SUCCESS:
                    SET_OUT(out_status, NT_SUCCESS);
                    return key_event;
                default:
                    SET_OUT(out_status, NT_ERR_UNEXPECTED);
                    return NT_KEY_EMPTY;
            }

        }
        else
        {
            struct nt_key_event key = process_key_event_utf32(buff + 1, true, &_status);

            switch(_status)
            {
                case NT_SUCCESS:
                    SET_OUT(out_status, NT_SUCCESS);
                    return key;
                default:
                    SET_OUT(out_status, NT_ERR_UNEXPECTED);
                    return NT_KEY_EMPTY;
            }
        }
    }
    else // UTF32
    {
        struct nt_key_event key = process_key_event_utf32(buff, false, &_status);

        switch(_status)
        {
            case NT_SUCCESS:
                SET_OUT(out_status, NT_SUCCESS);
                return key;
            default:
                SET_OUT(out_status, NT_ERR_UNEXPECTED);
                return NT_KEY_EMPTY;
        }
    }
}

static struct nt_key_event process_key_event_utf32(uint8_t* utf8_sbyte,
        bool alt, nt_status* out_status)
{
    size_t utf32_len;
    utf32_len = uc_utf8_unit_len(utf8_sbyte[0]);
    if(utf32_len == SIZE_MAX)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return NT_KEY_EMPTY;
    }

    // read()
    int read_status = read(STDIN_FILENO, utf8_sbyte + 1, utf32_len - 1);
    if(read_status < 0)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return NT_KEY_EMPTY;
    }
    
    if(read_status != (utf32_len - 1))
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return NT_KEY_EMPTY;
    }

    uint32_t utf32;
    size_t utf32_width;
    uc_status _status;
    uc_utf8_to_utf32(utf8_sbyte, utf32_len, &utf32, 1, 0, &utf32_width, &_status);
    if(_status != UC_SUCCESS)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return NT_KEY_EMPTY;
    }

    struct nt_key_event key_event = {
        .type = NT_KEY_EVENT_UTF32,
        .utf32 = { .cp = utf32, .alt = alt }
    };

    SET_OUT(out_status, NT_SUCCESS);
    return key_event;
}

static struct nt_key_event process_key_event_esc_key(uint8_t* buff,
        nt_status* out_status)
{
    int read_status;
    int read_count = 2;
    while(true)
    {
        read_status = read(STDIN_FILENO, buff + read_count, 1);
        if(read_status < 0)
        {
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            return NT_KEY_EMPTY;
        }

        if((buff[read_count] >= 0x40) && (buff[read_count] <= 0x7E))
            break;

        read_count++;
    }

    buff[read_count + 1] = 0;

    char* cbuff = (char*)buff;

    int i;
    struct nt_key_event ret;
    struct nt_term_info term = _nt_term_get_used();
    for(i = 0; i < NT_ESC_KEY_OTHER; i++)
    {
        if(strcmp(cbuff, term.esc_key_seqs[i]) == 0)
        {
            ret = (struct nt_key_event) {
                .type = NT_KEY_EVENT_ESC,
                .esc = { .val = i }
            };

            SET_OUT(out_status, NT_SUCCESS);
            return ret;
        }
    }

    ret = (struct nt_key_event) {
        .type = NT_KEY_EVENT_ESC,
        .esc = { .val = NT_ESC_KEY_OTHER }
    };

    SET_OUT(out_status, NT_SUCCESS);
    return ret;
}
