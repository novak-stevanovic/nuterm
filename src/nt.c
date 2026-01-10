/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#include "nt.h"

#include <assert.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>

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

static pthread_t sigthread;
static pthread_mutex_t sigthread_lock;
volatile static bool sigthread_stop;

static int signal_pipe[2];
static int resize_pipe[2];
static int custom_event_pipe[2];
static struct pollfd poll_fds[POLL_FD_COUNT];
static struct termios init_term_opts;

static char* stdout_buff;
static size_t stdout_buff_pos;
static size_t stdout_buff_cap;

static bool init_get_term_opts, init_set_term_opts,
            init_sigmask_set, init_sigthread_create,
            init_sigthread_lock, init_term;

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

        usignal = (unsigned int)signal;
        if(signal == SIGWINCH)
        {
            write(resize_pipe[1], &usignal, sizeof(unsigned int));
        }
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

static void init_default_values()
{
    sigthread = 0;
    sigthread_stop = false;

    signal_pipe[0] = -1;
    signal_pipe[1] = -1;
    resize_pipe[0] = -1;
    resize_pipe[1] = -1;
    custom_event_pipe[0] = -1;
    custom_event_pipe[1] = -1;
    
    size_t i;
    for(i = 0; i < POLL_FD_COUNT; i++)
        poll_fds[i] = (struct pollfd) {0};
        
    init_term_opts = (struct termios) {0};

    stdout_buff = NULL;
    stdout_buff_pos = 0;
    stdout_buff_cap = 0;

    init_get_term_opts = false;
    init_set_term_opts = false;
    init_sigmask_set = false;
    init_sigthread_create = false;
    init_sigthread_lock = false;
    init_term = false;
}

void nt_init(nt_status* out_status)
{
    init_default_values();

    nt_status _status;
    int status;

    status = tcgetattr(STDIN_FILENO, &init_term_opts);
    if(status == -1)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        nt_deinit();
        return;
    }
    init_get_term_opts = true;

    struct termios raw_opts = init_term_opts;
    term_opts_raw(&raw_opts);
    status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_opts);
    if(status == -1)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        nt_deinit();
        return;
    }
    init_set_term_opts = true;

    int pipe_status;
    pipe_status = pipe(signal_pipe);
    if(pipe_status != 0)
    {
        SET_OUT(out_status, NT_ERR_INIT_PIPE);
        nt_deinit();
        return;
    }

    pipe_status = pipe(custom_event_pipe);
    if(pipe_status != 0)
    {
        SET_OUT(out_status, NT_ERR_INIT_PIPE);
        nt_deinit();
        return;
    }

    pipe_status = pipe(resize_pipe);
    if(pipe_status != 0)
    {
        SET_OUT(out_status, NT_ERR_INIT_PIPE);
        nt_deinit();
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
        nt_deinit();
        return;
    }
    init_sigmask_set = true;

    int lock_status = pthread_mutex_init(&sigthread_lock, NULL);
    if(lock_status != 0)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        nt_deinit();
        return;
    }
    init_sigthread_lock = true;

    int thread_status = pthread_create(&sigthread, NULL, sigthread_fn, NULL);
    if(thread_status != 0)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        nt_deinit();
        return;
    }
    init_sigthread_create = true;

    _nt_term_init(&_status);
    switch(_status)
    {
        case NT_SUCCESS:
            SET_OUT(out_status, NT_SUCCESS);
            init_term = true;
            return;
        case NT_ERR_TERM_NOT_SUPP:
            SET_OUT(out_status, NT_ERR_TERM_NOT_SUPP);
            init_term = true;
            return;
        case NT_ERR_INIT_TERM_ENV:
            SET_OUT(out_status, NT_ERR_INIT_TERM_ENV);
            nt_deinit();
        default:
            SET_OUT(out_status, NT_ERR_UNEXPECTED);
            nt_deinit();
    }
}

void nt_deinit()
{
    if(init_sigthread_create)
    {
        pthread_mutex_lock(&sigthread_lock);
        sigthread_stop = true;
        pthread_mutex_unlock(&sigthread_lock);
        kill(0, SIGINT);

        pthread_join(sigthread, NULL);

        init_sigthread_create = false;
    }
    if(init_sigthread_lock)
    {
        pthread_mutex_destroy(&sigthread_lock);
        init_sigthread_lock = false;
    }
    if(init_term)
    {
        nt_status _status;
        nt_write_str("", 0, NT_GFX_DEFAULT, &_status);

        _nt_term_deinit();
        init_term = false;
    }
    if(init_set_term_opts)
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &init_term_opts);
        init_set_term_opts = false;
    }
    if(init_sigmask_set)
    {
        sigset_t set;
        sigemptyset(&set);
        pthread_sigmask(SIG_SETMASK, &set, NULL);
        init_sigmask_set = false;
    }

    init_default_values();
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

/* Called by nt_event_wait() internally. These functions only set `out_status` to
 * NT_ERR_UNEXPECTED or NT_SUCCESS. */
static struct nt_event process_stdin(nt_status* out_status, bool* out_ignore);
static struct nt_event process_resize(nt_status* out_status, bool* out_ignore);
static struct nt_event process_signal(nt_status* out_status, bool* out_ignore);
static struct nt_event process_custom(nt_status* out_status, bool* out_ignore);

/* -------------------------------------------------------------------------- */

static const struct nt_event NT_EVENT_EMPTY = {0};

unsigned int nt_event_wait(struct nt_event* out_event,
        unsigned int timeout, nt_status* out_status)
{
    struct timespec _time1, _time2;
    int poll_status;
    unsigned int elapsed;
    nt_status _status;

    struct nt_event event = NT_EVENT_EMPTY;
    bool _ignore = false;

    SET_OUT(out_event, NT_EVENT_EMPTY);
    SET_OUT(out_status, NT_ERR_UNEXPECTED);

    while(true)
    {
        clock_gettime(CLOCK_REALTIME, &_time1);
        poll_status = poll(poll_fds, POLL_FD_COUNT, timeout);
        clock_gettime(CLOCK_REALTIME, &_time2);
        elapsed = ((_time2.tv_sec - _time1.tv_sec) * 1e3) +
            ((_time2.tv_nsec - _time1.tv_nsec) / 1e6);
        elapsed = (elapsed <= timeout) ? elapsed : timeout;
        if(poll_status == -1) return elapsed;
        if(poll_status == 0)
        {
            SET_OUT(out_event, nt_event_new(NT_EVENT_TIMEOUT, NULL, 0));
            SET_OUT(out_status, NT_SUCCESS);
            return elapsed;
        }

        timeout -= elapsed; // for the next poll(), if _ignore == true

        if(poll_fds[STDIN_POLL_FD].revents & POLLIN)
        {
            event = process_stdin(&_status, &_ignore);
        }
        else if(poll_fds[RESIZE_POLL_FD].revents & POLLIN)
        {
            event = process_resize(&_status, &_ignore);
        }
        else if(poll_fds[SIGNAL_POLL_FD].revents & POLLIN)
        {
            event = process_signal(&_status, &_ignore);
        }
        else if(poll_fds[CUSTOM_POLL_FD].revents & POLLIN)
        {
            event = process_custom(&_status, &_ignore);
        }
        else return elapsed;

        if(_status != NT_SUCCESS)
            return elapsed;
        else // _status = NT_SUCCESS
        {
            if(_ignore) continue;
            else break;
        }
    }

    SET_OUT(out_event, event);
    SET_OUT(out_status, NT_SUCCESS);
    return elapsed;
}

void nt_event_push(struct nt_event event, nt_status* out_status)
{
    if(!nt_event_is_valid(event))
    {
        SET_OUT(out_status, NT_ERR_INVALID_ARG);
        return;
    }

    size_t i;
    uint8_t _type;
    for(i = 0; i < sizeof(uint32_t) * 8; i++)
    {
        if(event.type & (1 << i)) break;
    }
    _type = i;

    // Prepare buffer for writing
    uint8_t buff[sizeof(struct nt_event_header) + NT_EVENT_DATA_MAX_SIZE] = {0};
    buff[0] = _type;
    buff[1] = event.data_size;

    // If there's data, write it to buffer
    if(event.data_size > 0) memcpy(buff + 2, event.data, event.data_size);

    // Write to the pipe
    int write_status = write(
            custom_event_pipe[1],
            buff,
            sizeof(struct nt_event_header) + event.data_size);
    if(write_status == -1)
    {
        SET_OUT(out_status, NT_ERR_UNEXPECTED);
        return;
    }

    SET_OUT(out_status, NT_SUCCESS);
}

/* ------------------------------------------------------ */

static struct nt_event process_resize(nt_status* out_status, bool* out_ignore)
{
    SET_OUT(out_ignore, false);
    SET_OUT(out_status, NT_ERR_UNEXPECTED);

    int poll_status, read_status;
    char c;
    while(true)
    {
        // Already polled, so read first
        read_status = read(resize_pipe[0], &c, 1);
        if(read_status < 0) return NT_EVENT_EMPTY;

        poll_status = poll(poll_fds + RESIZE_POLL_FD, 1, 0);
        if(poll_status < 0) return NT_EVENT_EMPTY;
        if(poll_status == 0) break;
    }

    SET_OUT(out_status, NT_SUCCESS);
    struct nt_resize_event rsz;
    nt_get_term_size(&rsz.new_x, &rsz.new_y);
    return nt_event_new(NT_EVENT_RESIZE, &rsz, sizeof(rsz));
}

/* ------------------------------------------------------ */

static struct nt_event process_signal(nt_status* out_status, bool* out_ignore)
{
    SET_OUT(out_ignore, false);
    SET_OUT(out_status, NT_ERR_UNEXPECTED);

    int read_status;

    unsigned int signum = 0;
    read_status = read(signal_pipe[0], &signum, sizeof(unsigned int));
    if(read_status < 0) return NT_EVENT_EMPTY;

    SET_OUT(out_status, NT_SUCCESS);
    return nt_event_new(NT_EVENT_SIGNAL, &signum, sizeof(signum));
}

/* ------------------------------------------------------ */

static struct nt_event process_custom(nt_status* out_status, bool* out_ignore)
{
    SET_OUT(out_ignore, false);
    SET_OUT(out_status, NT_ERR_UNEXPECTED);

    int read_status;

    // Read header to determine type and data_size
    struct nt_event_header header = {0};
    read_status = read(custom_event_pipe[0], &header, sizeof(header));
    if(read_status < 0) return NT_EVENT_EMPTY;
   
    uint8_t buff[NT_EVENT_DATA_MAX_SIZE] = {0};
    // If event has data, read it
    if(header.data_size > 0)
    {
        read_status = read(custom_event_pipe[0], buff, header.data_size);
        if(read_status < 0) return NT_EVENT_EMPTY;
    }

    uint32_t type = (1 << header.type);

    SET_OUT(out_status, NT_SUCCESS);
    return nt_event_new(type, &buff, header.data_size);
}

/* ------------------------------------------------------ */

static struct nt_event process_stdin_utf32(uint8_t* utf8_sbyte,
        bool alt, nt_status* out_status, bool* out_ignore);

static struct nt_event process_stdin_esc(uint8_t* buff, size_t read_count,
        nt_status* out_status, bool* out_ignore);

enum process_stdin_state
{
    PROCESS_STDIN_BEGIN, // nothing had been pressed
    PROCESS_STDIN_ESC_BEGIN, // first key is ESC key
    PROCESS_STDIN_UTF32, // UTF32 with or without alt(ESC prefix)
    PROCESS_STDIN_ESC_SEQ_OR_ALT, // second key is a sequence opener
    PROCESS_STDIN_ESC_SEQ_READ, // read escape sequence
    PROCESS_STDIN_ESC_SEQ_PROCESS // process esc sequence
};

static struct nt_event process_stdin(nt_status* out_status, bool* out_ignore)
{
    uint8_t buff[64];
    int read_status, poll_status;

    SET_OUT(out_status, NT_ERR_UNEXPECTED);

    enum process_stdin_state state = PROCESS_STDIN_BEGIN;

    size_t esc_seq_read_count = 0;
    bool alt = false;
    while(true)
    {
        switch(state)
        {
            case PROCESS_STDIN_BEGIN:
                read_status = read(STDIN_FILENO, buff, 1);
                if(read_status < 0) return NT_EVENT_EMPTY;

                if(buff[0] == 0x1b)
                    state = PROCESS_STDIN_ESC_BEGIN;
                else // not ESC key
                    state = PROCESS_STDIN_UTF32;
                break;

            case PROCESS_STDIN_ESC_BEGIN:
                poll_status = poll(poll_fds + STDIN_POLL_FD, 1, 0);
                if(poll_status < 0) return NT_EVENT_EMPTY;

                if(poll_status == 0) // just ESC
                {
                    struct nt_key_event key = {
                        .type = NT_KEY_EVENT_UTF32,
                        .utf32 = { .cp = 27, .alt = false }
                    };
                    SET_OUT(out_status, NT_SUCCESS);
                    return nt_event_new(NT_EVENT_KEY, &key, sizeof(key));
                }

                read_status = read(STDIN_FILENO, buff + 1, 1);
                if(read_status < 0) return NT_EVENT_EMPTY;

                state = PROCESS_STDIN_ESC_SEQ_OR_ALT;
                break;

            case PROCESS_STDIN_ESC_SEQ_OR_ALT:
                if((buff[1] == '[') || (buff[1] == 'O')) // TODO
                {
                    poll_status = poll(poll_fds + STDIN_POLL_FD, 1, 0);
                    if(poll_status < 0) return NT_EVENT_EMPTY;

                    if(poll_status == 0) // ALT + BUFF[read_count]
                    {
                        struct nt_key_event key = {
                            .type = NT_KEY_EVENT_UTF32,
                            .utf32 = { .cp = buff[1], .alt = true }
                        };
                        SET_OUT(out_status, NT_SUCCESS);
                        return nt_event_new(NT_EVENT_KEY, &key, sizeof(key));
                    }

                    state = PROCESS_STDIN_ESC_SEQ_READ;
                    esc_seq_read_count = 2;
                }
                else
                {
                    alt = true;
                    state = PROCESS_STDIN_UTF32;
                }
                break;

            case PROCESS_STDIN_UTF32:
                return process_stdin_utf32(buff + alt, alt, out_status, out_ignore);
                break;

            case PROCESS_STDIN_ESC_SEQ_READ:
                read_status = read(STDIN_FILENO, buff + esc_seq_read_count, 1);
                if(read_status < 0) return NT_EVENT_EMPTY;

                esc_seq_read_count++;

                if((buff[esc_seq_read_count - 1] >= 0x40) && (buff[esc_seq_read_count - 1] <= 0x7E))
                {
                    state = PROCESS_STDIN_ESC_SEQ_PROCESS;
                    break;
                }

                poll_status = poll(poll_fds + STDIN_POLL_FD, 1, 0);
                if(poll_status <= 0) return NT_EVENT_EMPTY;

                break;

            case PROCESS_STDIN_ESC_SEQ_PROCESS:
                return process_stdin_esc(buff, esc_seq_read_count, out_status, out_ignore);
                break;
        }
    }

    return NT_EVENT_EMPTY;
}

static struct nt_event process_stdin_utf32(uint8_t* utf8_sbyte,
        bool alt, nt_status* out_status, bool* out_ignore)
{
    SET_OUT(out_status, NT_ERR_UNEXPECTED);
    SET_OUT(out_ignore, false);

    size_t utf32_len;
    utf32_len = uc_utf8_unit_len(utf8_sbyte[0]);
    if(utf32_len == SIZE_MAX) return NT_EVENT_EMPTY;

    // read()
    int read_status = read(STDIN_FILENO, utf8_sbyte + 1, utf32_len - 1);
    if(read_status < 0) return NT_EVENT_EMPTY;
    
    if(read_status != (utf32_len - 1)) return NT_EVENT_EMPTY;

    uint32_t utf32;
    size_t utf32_width;
    int _status;
    uc_utf8_to_utf32(utf8_sbyte, utf32_len, &utf32, 1, 0, &utf32_width, &_status);
    if(_status != UC_SUCCESS) return NT_EVENT_EMPTY;

    struct nt_key_event key_event = {
        .type = NT_KEY_EVENT_UTF32,
        .utf32 = { .cp = utf32, .alt = alt }
    };

    SET_OUT(out_status, NT_SUCCESS);
    return nt_event_new(NT_EVENT_KEY, &key_event, sizeof(key_event));
}

enum process_mouse_result
{
    MOUSE_EVENT_SUPPORTED,
    MOUSE_EVENT_UNSUPPORTED,
    NOT_MOUSE_EVENT
};

/* Detects if an escape sequence is a mouse sequence.
 * If yes, return whether the sequence is supported. If supported, initialize
 * `out_event`.
 * If not, return signals that the sequence is not a mouse event */
static enum process_mouse_result process_stdin_esc_mouse(uint8_t* buff,
        size_t read_count, struct nt_mouse_event* out_event);

static struct nt_event process_stdin_esc(uint8_t* buff,
        size_t read_count, nt_status* out_status, bool* out_ignore)
{
    SET_OUT(out_status, NT_ERR_UNEXPECTED);
    SET_OUT(out_ignore, false);

    buff[read_count] = 0;

    struct nt_mouse_event _mouse_event;
    enum process_mouse_result mouse_rv = process_stdin_esc_mouse(buff,
            read_count, &_mouse_event);
    if(mouse_rv == MOUSE_EVENT_SUPPORTED)
    {
        SET_OUT(out_status, NT_SUCCESS);
        return nt_event_new(NT_EVENT_MOUSE, &_mouse_event, sizeof(_mouse_event));
    }
    else if(mouse_rv == MOUSE_EVENT_UNSUPPORTED)
    {
        SET_OUT(out_ignore, true);
        SET_OUT(out_status, NT_SUCCESS);
        return NT_EVENT_EMPTY;
    }

    int i;
    struct nt_key_event key;
    struct nt_term_info term = _nt_term_get_used();
    for(i = 0; i < NT_ESC_KEY_OTHER; i++)
    {
        if(strcmp((char*)buff, term.esc_key_seqs[i]) == 0)
        {
            key = (struct nt_key_event) {
                .type = NT_KEY_EVENT_ESC,
                .esc = { .val = i }
            };

            SET_OUT(out_status, NT_SUCCESS);
            return nt_event_new(NT_EVENT_KEY, &key, sizeof(key));
        }
    }

    key = (struct nt_key_event) {
        .type = NT_KEY_EVENT_ESC,
        .esc = { .val = NT_ESC_KEY_OTHER }
    };

    SET_OUT(out_status, NT_SUCCESS);
    return nt_event_new(NT_EVENT_KEY, &key, sizeof(key));
}

// ESC [ < Cb ; Cx ; Cy M
// Refactor sometimes...
static enum process_mouse_result process_stdin_esc_mouse(uint8_t* buff,
        size_t read_count, struct nt_mouse_event* out_event)
{
    struct nt_mouse_event event = {0};

    SET_OUT(out_event, event);
    if(read_count < 9) return NOT_MOUSE_EVENT;

    if((buff[0] != 0x1b) || (buff[1] != '[') || (buff[2] != '<'))
        return NOT_MOUSE_EVENT;

    size_t semicol_idx[2];
    void* memchr_rv;
    memchr_rv = memchr(buff + 3, ';', read_count - 3);
    if(memchr_rv == NULL) return NOT_MOUSE_EVENT;
    semicol_idx[0] = (char*)(memchr_rv) - (char*)buff;

    if(!(semicol_idx[0] > 3)) return NOT_MOUSE_EVENT;

    memchr_rv = memchr(buff + semicol_idx[0] + 1, ';', read_count - semicol_idx[0] - 1);
    if(memchr_rv == NULL) return NOT_MOUSE_EVENT;
    semicol_idx[1] = (char*)(memchr_rv) - (char*)buff;

    if(!(semicol_idx[1] > (semicol_idx[0] + 1))) return NOT_MOUSE_EVENT;

    if((buff[read_count - 1] != 'M') && (buff[read_count - 1] != 'm'))
        return NOT_MOUSE_EVENT;
    if(buff[read_count - 1] == 'm') return MOUSE_EVENT_UNSUPPORTED;

    if(!((read_count - 1) > (semicol_idx[1] + 1))) return NOT_MOUSE_EVENT;

    size_t i;
    for(i = 3; i < semicol_idx[0]; i++)
    {
        if((buff[i] < '0') || (buff[i] > '9')) return NOT_MOUSE_EVENT;
    }
    for(i = (semicol_idx[0] + 1); i < semicol_idx[1]; i++)
    {
        if((buff[i] < '0') || (buff[i] > '9')) return NOT_MOUSE_EVENT;
    }
    for(i = (semicol_idx[1] + 1); i < (read_count - 1); i++)
    {
        if((buff[i] < '0') || (buff[i] > '9')) return NOT_MOUSE_EVENT;
    }

    int cb, cx, cy;
    char num_buff[64];

    memcpy(num_buff, buff + 3, semicol_idx[0] - 3);
    num_buff[semicol_idx[0] - 3] = 0;
    cb = atoi(num_buff);

    memcpy(num_buff, buff + semicol_idx[0] + 1, semicol_idx[1] - semicol_idx[0] - 1);
    num_buff[semicol_idx[1] - semicol_idx[0] - 1] = 0;
    cx = atoi(num_buff);

    memcpy(num_buff, buff + semicol_idx[1] + 1, read_count - semicol_idx[1] - 2);
    num_buff[read_count - semicol_idx[1] - 2] = 0;
    cy = atoi(num_buff);

    event.x = cx;
    event.y = cy;
    if(cb == 64)
        event.type = NT_MOUSE_EVENT_SCROLL_UP;
    else if(cb == 65)
        event.type = NT_MOUSE_EVENT_SCROLL_DOWN;
    else if((cb & 0b11) == 0)
        event.type = NT_MOUSE_EVENT_CLICK_LEFT;
    else if((cb & 0b11) == 1)
        event.type = NT_MOUSE_EVENT_CLICK_RIGHT;
    else if((cb & 0b11) == 2)
        event.type = NT_MOUSE_EVENT_CLICK_MIDDLE;
    else return MOUSE_EVENT_UNSUPPORTED;
    
    SET_OUT(out_event, event);
    return MOUSE_EVENT_SUPPORTED;
}

