/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#include "nt.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
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

#define UCONV_IMPLEMENTATION
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
static volatile bool sigthread_stop;

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

static void nt__clear_pending_sigpipe(void)
{
    sigset_t pending;
    if(sigpending(&pending) != 0)
        return;
    if(!sigismember(&pending, SIGPIPE))
        return;

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);

    int signal;
    sigwait(&set, &signal);
}

static int nt__write_all(int fd, const void* data, size_t size)
{
    const char* it = data;

    while(size > 0)
    {
        ssize_t written = write(fd, it, size);
        if(written < 0)
        {
            if(errno == EINTR)
                continue;
            if(errno == EPIPE)
                nt__clear_pending_sigpipe();

            return NT_ERR_UNEXPECTED;
        }
        if(written == 0)
            return NT_ERR_UNEXPECTED;

        it += written;
        size -= (size_t)written;
    }

    return 0;
}

/* Pipe event writes must stay a single atomic write. */
static int nt__write_pipe_event(int fd, const void* data, size_t size)
{
    ssize_t written;
    do
    {
        written = write(fd, data, size);
    }
    while((written < 0) && (errno == EINTR));

    if(written < 0)
    {
        if(errno == EPIPE)
            nt__clear_pending_sigpipe();
        return NT_ERR_UNEXPECTED;
    }
    if((size_t)written != size)
        return NT_ERR_UNEXPECTED;

    return 0;
}

static int nt__read_exact(int fd, void* data, size_t size)
{
    uint8_t* it = data;

    while(size > 0)
    {
        ssize_t read_count = read(fd, it, size);
        if(read_count < 0)
        {
            if(errno == EINTR)
                continue;

            return NT_ERR_UNEXPECTED;
        }
        if(read_count == 0)
            return NT_ERR_UNEXPECTED;

        it += read_count;
        size -= (size_t)read_count;
    }

    return 0;
}

static int nt__poll_retry(struct pollfd* fds, nfds_t count, int timeout)
{
    int status;
    do
    {
        status = poll(fds, count, timeout);
    }
    while((status < 0) && (errno == EINTR));

    return status;
}

static inline int nt__write_to_stdout(const char* str, size_t str_len)
{
    if(str_len == 0)
        return 0;

    if(stdout_buff == NULL)
        return nt__write_all(STDOUT_FILENO, str, str_len);

    if(stdout_buff_pos + str_len <= stdout_buff_cap)
    {
        memcpy(stdout_buff + stdout_buff_pos, str, str_len);
        stdout_buff_pos += str_len;
        return 0;
    }

    int status = nt__write_all(STDOUT_FILENO, stdout_buff, stdout_buff_pos);
    stdout_buff_pos = 0;
    if(status)
        return status;

    if(str_len <= stdout_buff_cap)
    {
        memcpy(stdout_buff, str, str_len);
        stdout_buff_pos = str_len;
        return 0;
    }

    return nt__write_all(STDOUT_FILENO, str, str_len);
}

static void* nt__sigthread_fn(void* data)
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

        if(sigwait(&set, &signal) != 0)
        {
            close(resize_pipe[1]);
            resize_pipe[1] = -1;
            close(signal_pipe[1]);
            signal_pipe[1] = -1;
            break;
        }

        usignal = (unsigned int)signal;
        if(signal == SIGWINCH)
        {
            if(nt__write_pipe_event(
                    resize_pipe[1],
                    &usignal,
                    sizeof(unsigned int)))
            {
                close(resize_pipe[1]);
                resize_pipe[1] = -1;
                close(signal_pipe[1]);
                signal_pipe[1] = -1;
                break;
            }
        }

        if(nt__write_pipe_event(
                signal_pipe[1],
                &usignal,
                sizeof(unsigned int)))
        {
            close(resize_pipe[1]);
            resize_pipe[1] = -1;
            close(signal_pipe[1]);
            signal_pipe[1] = -1;
            break;
        }
    }

    return NULL;
}

static inline void nt__term_opts_raw(struct termios* term_opts)
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

static void nt__init_default_values(void)
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

int nt_init(void)
{
    nt__init_default_values();

    int status;

    status = tcgetattr(STDIN_FILENO, &init_term_opts);
    if(status == -1)
    {
        nt_deinit();
        return NT_ERR_UNEXPECTED;
    }
    init_get_term_opts = true;

    struct termios raw_opts = init_term_opts;
    nt__term_opts_raw(&raw_opts);
    status = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_opts);
    if(status == -1)
    {
        nt_deinit();
        return NT_ERR_UNEXPECTED;
    }
    init_set_term_opts = true;

    if(pipe(signal_pipe) != 0)
    {
        nt_deinit();
        return NT_ERR_INIT_PIPE;
    }

    if(pipe(custom_event_pipe) != 0)
    {
        nt_deinit();
        return NT_ERR_INIT_PIPE;
    }

    if(pipe(resize_pipe) != 0)
    {
        nt_deinit();
        return NT_ERR_INIT_PIPE;
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
    if(pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
    {
        nt_deinit();
        return NT_ERR_UNEXPECTED;
    }
    init_sigmask_set = true;

    if(pthread_mutex_init(&sigthread_lock, NULL) != 0)
    {
        nt_deinit();
        return NT_ERR_UNEXPECTED;
    }
    init_sigthread_lock = true;

    if(pthread_create(&sigthread, NULL, nt__sigthread_fn, NULL) != 0)
    {
        nt_deinit();
        return NT_ERR_UNEXPECTED;
    }
    init_sigthread_create = true;

    status = nt__term_init();
    switch(status)
    {
        case 0:
            init_term = true;
            return 0;
        case NT_ERR_TERM_NOT_SUPP:
            init_term = true;
            return NT_ERR_TERM_NOT_SUPP;
        case NT_ERR_INIT_TERM_ENV:
            nt_deinit();
            return NT_ERR_INIT_TERM_ENV;
        default:
            nt_deinit();
            return NT_ERR_UNEXPECTED;
    }
}

static void nt__close_pipe(int p[2])
{
    if(p[0] >= 0) { close(p[0]); p[0] = -1; }
    if(p[1] >= 0) { close(p[1]); p[1] = -1; }
}

void nt_deinit(void)
{
    if(init_sigthread_create)
    {
        pthread_mutex_lock(&sigthread_lock);
        sigthread_stop = true;
        pthread_mutex_unlock(&sigthread_lock);
        pthread_kill(sigthread, SIGINT);

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
        nt_write_str("", 0, NT_GFX_DEFAULT);

        nt__term_deinit();
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

    nt__close_pipe(signal_pipe);
    nt__close_pipe(custom_event_pipe);
    nt__close_pipe(resize_pipe);

    nt__init_default_values();
}

/* -------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* -------------------------------------------------------------------------- */

static int nt__execute_used_term_func(
        enum nt_esc_func func,
        int use_va,
        ...)
{
    int status;

    struct nt_term_info used_term = nt__term_get_used();

    const char* esc_func = used_term.esc_func_seqs[func];
    if(esc_func == NULL)
        return NT_ERR_FUNC_NOT_SUPP;

    const char* term_func;
    char buff[100];
    if(use_va)
    {
        va_list list;
        va_start(list, use_va);

        status = vsnprintf(buff, sizeof(buff), esc_func, list);
        va_end(list);
        if((status < 0) || ((size_t)status >= sizeof(buff)))
            return NT_ERR_UNEXPECTED;

        term_func = buff;
    }
    else
    {
        term_func = esc_func;
    }

    return nt__write_to_stdout(term_func, strlen(term_func));
}

/* -------------------------------------------------------------------------- */

int nt_buffer_enable(char* buff, size_t cap)
{
    if((buff == NULL) || (cap == 0))
        return NT_ERR_INVALID_ARG;

    if(stdout_buff != NULL)
        return NT_ERR_ALR_BUFF;

    stdout_buff = buff;
    stdout_buff_cap = cap;
    stdout_buff_pos = 0;

    return 0;
}

int nt_buffer_disable(enum nt_buffact buffact, char** out_buff)
{
    char* old = stdout_buff;
    int status = 0;

    if(stdout_buff != NULL)
    {
        if((buffact == NT_BUFF_FLUSH) && (stdout_buff_pos > 0))
            status = nt__write_all(STDOUT_FILENO, stdout_buff, stdout_buff_pos);

        /* Disable buffering regardless of the flush result. */
        stdout_buff = NULL;
        stdout_buff_pos = 0;
        stdout_buff_cap = 0;
    }

    if(out_buff != NULL)
        *out_buff = old;

    return status;
}

int nt_buffer_flush(void)
{
    int status = 0;

    if((stdout_buff != NULL) && (stdout_buff_pos > 0))
    {
        status = nt__write_all(STDOUT_FILENO, stdout_buff, stdout_buff_pos);

        /* A failed write may be partial, so the attempted contents cannot
         * be safely retried as a whole. */
        stdout_buff_pos = 0;
    }

    return status;
}

/* ----------------------------------------------------- */

int nt_cursor_hide(void)
{
    return nt__execute_used_term_func(NT_ESC_FUNC_CURSOR_HIDE, false);
}

int nt_cursor_show(void)
{
    return nt__execute_used_term_func(NT_ESC_FUNC_CURSOR_SHOW, false);
}

int nt_cursor_move(size_t x, size_t y)
{
    return nt__execute_used_term_func(
            NT_ESC_FUNC_CURSOR_MOVE, true, y + 1, x + 1);
}

int nt_erase_screen(void)
{
    int status = nt__execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false);
    if(status != 0)
        return status;

    return nt__execute_used_term_func(NT_ESC_FUNC_ERASE_SCREEN, false);
}

int nt_erase_line(void)
{
    int status = nt__execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false);
    if(status != 0)
        return status;

    return nt__execute_used_term_func(NT_ESC_FUNC_ERASE_LINE, false);
}

int nt_erase_scrollback(void)
{
    int status = nt__execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false);
    if(status != 0)
        return status;

    return nt__execute_used_term_func(NT_ESC_FUNC_ERASE_SCROLLBACK, false);
}

int nt_alt_screen_enable(void)
{
    return nt__execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_ENTER, false);
}

int nt_alt_screen_disable(void)
{
    return nt__execute_used_term_func(NT_ESC_FUNC_ALT_BUFF_EXIT, false);
}

int nt_mouse_mode_enable(void)
{
    return nt__execute_used_term_func(NT_ESC_FUNC_MOUSE_ENABLE, false);
}

int nt_mouse_mode_disable(void)
{
    return nt__execute_used_term_func(NT_ESC_FUNC_MOUSE_DISABLE, false);
}

void nt_get_term_size(size_t* out_width, size_t* out_height)
{
    struct winsize size;
    int status = ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
    size_t ret_width, ret_height;
    if(status == -1)
    {
        ret_width = 0;
        ret_height = 0;
    }
    else
    {
        ret_width = size.ws_col;
        ret_height = size.ws_row;
    }

    if(out_width != NULL) *out_width = ret_width;
    if(out_height != NULL) *out_height = ret_height;
}

/* ------------------------------------------------------------------------- */
/* WRITE TO TERMINAL */
/* ------------------------------------------------------------------------- */

/* This function assumes:
 * 1) The terminal has the capability to set default fg and bg colors.
 * 2) If the terminal supports RGB, then the library holds the terminal's
 * esc sequence to set the RGB color for bg/fg. Same with 256 colors and
 * 8 colors */
static int nt__set_gfx(struct nt_gfx gfx)
{
    int status;
    nt_term_color_count colors = nt__term_get_color_count();

    /* Set foreground --------------------------------------------------- */

    if(nt_color_are_eql(NT_COLOR_DEFAULT, gfx.fg))
    {
        status = nt__execute_used_term_func(NT_ESC_FUNC_FG_SET_DEFAULT, false);
    }
    else
    {
        gfx.fg = (gfx.fg.code8 <= NT_COLOR_C8_WHITE) ? gfx.fg : NT_COLOR_DEFAULT;

        if(colors == NT_TERM_COLOR_TC)
        {
            status = nt__execute_used_term_func(
                    NT_ESC_FUNC_FG_SET_RGB,
                    true,
                    gfx.fg.rgb.r,
                    gfx.fg.rgb.g,
                    gfx.fg.rgb.b);
        }
        else if(colors == NT_TERM_COLOR_C256)
        {
            status = nt__execute_used_term_func(
                    NT_ESC_FUNC_FG_SET_C256,
                    true,
                    gfx.fg.code256);
        }
        else if(colors == NT_TERM_COLOR_C8)
        {
            status = nt__execute_used_term_func(
                    NT_ESC_FUNC_FG_SET_C8,
                    true,
                    gfx.fg.code8);
        }
        else
        {
            return NT_ERR_UNEXPECTED;
        }
    }

    if(status != 0)
        return NT_ERR_UNEXPECTED;

    /* Set background --------------------------------------------------- */

    if(nt_color_are_eql(NT_COLOR_DEFAULT, gfx.bg))
    {
        status = nt__execute_used_term_func(NT_ESC_FUNC_BG_SET_DEFAULT, false);
    }
    else
    {
        gfx.bg = (gfx.bg.code8 <= NT_COLOR_C8_WHITE) ? gfx.bg : NT_COLOR_DEFAULT;

        if(colors == NT_TERM_COLOR_TC)
        {
            status = nt__execute_used_term_func(
                    NT_ESC_FUNC_BG_SET_RGB,
                    true,
                    gfx.bg.rgb.r,
                    gfx.bg.rgb.g,
                    gfx.bg.rgb.b);
        }
        else if(colors == NT_TERM_COLOR_C256)
        {
            status = nt__execute_used_term_func(
                    NT_ESC_FUNC_BG_SET_C256,
                    true,
                    gfx.bg.code256);
        }
        else if(colors == NT_TERM_COLOR_C8)
        {
            status = nt__execute_used_term_func(
                    NT_ESC_FUNC_BG_SET_C8,
                    true,
                    gfx.bg.code8);
        }
        else
        {
            return NT_ERR_UNEXPECTED;
        }
    }

    if(status != 0)
        return NT_ERR_UNEXPECTED;

    /* Set style -------------------------------------------------------- */

    uint8_t style;

    switch(colors)
    {
        case NT_TERM_COLOR_TC:
            style = gfx.style.value_rgb;
            break;
        case NT_TERM_COLOR_C256:
            style = gfx.style.value_c256;
            break;
        case NT_TERM_COLOR_C8:
            style = gfx.style.value_c8;
            break;
        default:
            return NT_ERR_UNEXPECTED;
    }

    size_t i;
    size_t count = 8;
    for(i = 0; i < count; i++)
    {
        if(style & (NT_STYLE_BOLD << i))
        {
            status = nt__execute_used_term_func(
                    NT_ESC_FUNC_STYLE_SET_BOLD + i,
                    true);

            if((status != 0) && (status != NT_ERR_FUNC_NOT_SUPP))
                return status;
        }
    }

    return 0;
}

int nt_write_str(const char* str, size_t len, struct nt_gfx gfx)
{
    int status;

    status = nt__execute_used_term_func(NT_ESC_FUNC_GFX_RESET, false);
    if(status != 0)
        return status;

    status = nt__set_gfx(gfx);
    if(status != 0)
        return status;

    /* In some terminals, a newline will fill the next row with currently set bg.
     * To avoid this, any time we run into a newline, we will reset the gfx,
     * print it in default GFX, and then resume printing */
    size_t rem;
    if(len > 0)
    {
        const char *it_begin = str, *it_end;

        while(true)
        {
            rem = (str + len) - it_begin;
            it_end = memchr(it_begin, '\n', rem);

            if(it_end != NULL)
            {
                status = nt__write_to_stdout(it_begin, it_end - it_begin);
                if(status != 0)
                    return status;

                status = nt__execute_used_term_func(NT_ESC_FUNC_GFX_RESET, false);
                if(status != 0)
                    return status;

                status = nt__write_to_stdout("\n", 1);
                if(status != 0)
                    return status;

                status = nt__set_gfx(gfx);
                if(status != 0)
                    return status;

                if(it_end < (str + len - 1))
                    it_begin = it_end + 1;
                else
                    break;
            }
            else
            {
                status = nt__write_to_stdout(it_begin, (str + len) - it_begin);
                if(status != 0)
                    return status;
                break;
            }
        }
    }

    return 0;
}

int nt_write_str_unsafe(const char* str, struct nt_gfx gfx)
{
    size_t len = str ? strlen(str) : 0;
    return nt_write_str(str, len, gfx);
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
static int nt__process_stdin(struct nt_event* out_event, bool* out_ignore);
static int nt__process_resize(struct nt_event* out_event, bool* out_ignore);
static int nt__process_signal(struct nt_event* out_event, bool* out_ignore);
static int nt__process_custom(struct nt_event* out_event, bool* out_ignore);

/* -------------------------------------------------------------------------- */

static const struct nt_event NT_EVENT_EMPTY = {0};

static int nt__event_new(
        uint32_t type,
        void* data,
        uint8_t data_size,
        struct nt_event* out_event)
{
    int status = nt_event_new_custom(type, data, data_size, out_event);
    return (status == 0) ? 0 : NT_ERR_UNEXPECTED;
}

int nt_event_wait(
        struct nt_event* out_event,
        unsigned int timeout,
        unsigned int* out_elapsed)
{
    struct timespec time1, time2;
    int poll_status;
    unsigned int elapsed;
    int status;

    struct nt_event event = NT_EVENT_EMPTY;
    struct nt_event timeout_event;
    if(nt_event_new_custom(NT_EVENT_TIMEOUT, NULL, 0, &timeout_event) != 0)
        return NT_ERR_UNEXPECTED;

    bool ignore = false;

    if(out_event != NULL)
        *out_event = NT_EVENT_EMPTY;
    if(out_elapsed != NULL)
        *out_elapsed = 0;

    while(true)
    {
        clock_gettime(CLOCK_REALTIME, &time1);
        poll_status = nt__poll_retry(poll_fds, POLL_FD_COUNT, (int)timeout);
        clock_gettime(CLOCK_REALTIME, &time2);
        elapsed = ((time2.tv_sec - time1.tv_sec) * 1e3) +
                  ((time2.tv_nsec - time1.tv_nsec) / 1e6);
        elapsed = (elapsed <= timeout) ? elapsed : timeout;

        if(out_elapsed != NULL)
            *out_elapsed = elapsed;

        if(poll_status == -1)
            return NT_ERR_UNEXPECTED;

        if(poll_status == 0)
        {
            if(out_event != NULL)
                *out_event = timeout_event;
            return 0;
        }

        timeout -= elapsed; // for the next poll(), if ignore == true

        if(poll_fds[STDIN_POLL_FD].revents & POLLIN)
        {
            status = nt__process_stdin(&event, &ignore);
            poll_fds[STDIN_POLL_FD].revents = 0;
        }
        else if(poll_fds[RESIZE_POLL_FD].revents & POLLIN)
        {
            status = nt__process_resize(&event, &ignore);
            poll_fds[RESIZE_POLL_FD].revents = 0;
        }
        else if(poll_fds[SIGNAL_POLL_FD].revents & POLLIN)
        {
            status = nt__process_signal(&event, &ignore);
            poll_fds[SIGNAL_POLL_FD].revents = 0;
        }
        else if(poll_fds[CUSTOM_POLL_FD].revents & POLLIN)
        {
            status = nt__process_custom(&event, &ignore);
            poll_fds[CUSTOM_POLL_FD].revents = 0;
        }
        else
        {
            return NT_ERR_UNEXPECTED;
        }

        if(status != 0)
            return status;

        if(ignore)
            continue;

        break;
    }

    if(out_event != NULL)
        *out_event = event;

    return 0;
}

int nt_event_queue_drain(void)
{
    struct nt_event event = {0};
    int status;

    while(true)
    {
        status = nt_event_wait(&event, 0, NULL);
        if(status != 0)
            return status;

        if(event.type == NT_EVENT_TIMEOUT)
            return 0;
    }
}

int nt_event_push(const struct nt_event* event)
{
    if(!event || !nt_event_is_valid(event))
        return NT_ERR_INVALID_ARG;

    size_t i;
    uint8_t type;
    for(i = 0; i < sizeof(uint32_t) * 8; i++)
    {
        if(event->type & (1u << i))
            break;
    }
    type = i;

    // Prepare buffer for writing
    uint8_t buff[sizeof(struct nt_event_header) + NT_EVENT_DATA_MAX_SIZE] = {0};
    buff[0] = type;
    buff[1] = event->data_size;

    // If there's data, write it to buffer
    if(event->data_size > 0)
        memcpy(buff + 2, event->u.data, event->data_size);

    // Write to the pipe. The whole event must stay one atomic write.
    size_t write_size = sizeof(struct nt_event_header) + event->data_size;
    return nt__write_pipe_event(custom_event_pipe[1], buff, write_size);
}

/* ------------------------------------------------------ */

static int nt__process_resize(struct nt_event* out_event, bool* out_ignore)
{
    if(out_ignore != NULL)
        *out_ignore = false;

    int poll_status;
    char c;
    while(true)
    {
        // Already polled, so read first.
        if(nt__read_exact(resize_pipe[0], &c, 1) != 0)
            return NT_ERR_UNEXPECTED;

        poll_status = nt__poll_retry(poll_fds + RESIZE_POLL_FD, 1, 0);
        if(poll_status < 0)
            return NT_ERR_UNEXPECTED;
        if(poll_status == 0)
            break;
        if(!(poll_fds[RESIZE_POLL_FD].revents & POLLIN))
            return NT_ERR_UNEXPECTED;
    }

    struct nt_resize rsz;
    memset(&rsz, 0, sizeof(rsz));
    nt_get_term_size(&rsz.new_x, &rsz.new_y);
    return nt__event_new(NT_EVENT_RESIZE, &rsz, sizeof(rsz), out_event);
}

/* ------------------------------------------------------ */

static int nt__process_signal(struct nt_event* out_event, bool* out_ignore)
{
    if(out_ignore != NULL)
        *out_ignore = false;

    unsigned int signum = 0;
    if(nt__read_exact(signal_pipe[0], &signum, sizeof(signum)) != 0)
        return NT_ERR_UNEXPECTED;

    return nt__event_new(NT_EVENT_SIGNAL, &signum, sizeof(signum), out_event);
}

/* ------------------------------------------------------ */

static int nt__process_custom(struct nt_event* out_event, bool* out_ignore)
{
    if(out_ignore != NULL)
        *out_ignore = false;

    // Read header to determine type and data_size.
    struct nt_event_header header = {0};
    if(nt__read_exact(custom_event_pipe[0], &header, sizeof(header)) != 0)
        return NT_ERR_UNEXPECTED;

    if((header.type >= 32) || (header.data_size > NT_EVENT_DATA_MAX_SIZE))
        return NT_ERR_UNEXPECTED;

    uint8_t buff[NT_EVENT_DATA_MAX_SIZE] = {0};
    if((header.data_size > 0) &&
       (nt__read_exact(custom_event_pipe[0], buff, header.data_size) != 0))
    {
        return NT_ERR_UNEXPECTED;
    }

    uint32_t type = (1u << header.type);
    return nt__event_new(type, buff, header.data_size, out_event);
}

/* ------------------------------------------------------ */

static int nt__process_stdin_utf32(
        uint8_t* utf8_sbyte,
        bool alt,
        struct nt_event* out_event,
        bool* out_ignore);

static int nt__process_stdin_esc(
        uint8_t* buff,
        size_t read_count,
        struct nt_event* out_event,
        bool* out_ignore);

enum nt__process_stdin_state
{
    PROCESS_STDIN_BEGIN, // nothing had been pressed
    PROCESS_STDIN_ESC_BEGIN, // first key is ESC key
    PROCESS_STDIN_UTF32, // UTF32 with or without alt(ESC prefix)
    PROCESS_STDIN_ESC_SEQ_OR_ALT, // second key is a sequence opener
    PROCESS_STDIN_ESC_SEQ_READ, // read escape sequence
    PROCESS_STDIN_ESC_SEQ_PROCESS // process esc sequence
};

static int nt__process_stdin(struct nt_event* out_event, bool* out_ignore)
{
    uint8_t buff[64];
    int poll_status;

    if(out_ignore != NULL)
        *out_ignore = false;

    enum nt__process_stdin_state state = PROCESS_STDIN_BEGIN;

    size_t esc_seq_read_count = 0;
    bool alt = false;
    while(true)
    {
        switch(state)
        {
            case PROCESS_STDIN_BEGIN:
                if(nt__read_exact(STDIN_FILENO, buff, 1) != 0)
                    return NT_ERR_UNEXPECTED;

                if(buff[0] == 0x1b)
                    state = PROCESS_STDIN_ESC_BEGIN;
                else // not ESC key
                    state = PROCESS_STDIN_UTF32;
                break;

            case PROCESS_STDIN_ESC_BEGIN:
                poll_status = nt__poll_retry(poll_fds + STDIN_POLL_FD, 1, 0);
                if(poll_status < 0)
                    return NT_ERR_UNEXPECTED;

                if(poll_status == 0) // just ESC
                {
                    struct nt_key key = nt_key_utf32_new(27, false);
                    return nt__event_new(NT_EVENT_KEY, &key, sizeof(key), out_event);
                }

                if(!(poll_fds[STDIN_POLL_FD].revents & POLLIN))
                    return NT_ERR_UNEXPECTED;

                if(nt__read_exact(STDIN_FILENO, buff + 1, 1) != 0)
                    return NT_ERR_UNEXPECTED;

                state = PROCESS_STDIN_ESC_SEQ_OR_ALT;
                break;

            case PROCESS_STDIN_ESC_SEQ_OR_ALT:
                /* Key escape sequences used by the supported terminals are
                 * CSI (ESC [) or SS3 (ESC O). */
                if((buff[1] == '[') || (buff[1] == 'O'))
                {
                    poll_status = nt__poll_retry(poll_fds + STDIN_POLL_FD, 1, 0);
                    if(poll_status < 0)
                        return NT_ERR_UNEXPECTED;

                    if(poll_status == 0) // ALT + BUFF[read_count]
                    {
                        struct nt_key key = nt_key_utf32_new(buff[1], true);
                        return nt__event_new(NT_EVENT_KEY, &key, sizeof(key), out_event);
                    }
                    if(!(poll_fds[STDIN_POLL_FD].revents & POLLIN))
                        return NT_ERR_UNEXPECTED;

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
                return nt__process_stdin_utf32(buff + alt, alt, out_event, out_ignore);

            case PROCESS_STDIN_ESC_SEQ_READ:
                if(esc_seq_read_count >= (sizeof(buff) - 1))
                    return NT_ERR_UNEXPECTED;

                if(nt__read_exact(STDIN_FILENO, buff + esc_seq_read_count, 1) != 0)
                {
                    return NT_ERR_UNEXPECTED;
                }

                esc_seq_read_count++;

                if((buff[esc_seq_read_count - 1] >= 0x40) &&
                   (buff[esc_seq_read_count - 1] <= 0x7E))
                {
                    state = PROCESS_STDIN_ESC_SEQ_PROCESS;
                    break;
                }

                poll_status = nt__poll_retry(poll_fds + STDIN_POLL_FD, 1, 0);
                if((poll_status <= 0) || !(poll_fds[STDIN_POLL_FD].revents & POLLIN))
                {
                    return NT_ERR_UNEXPECTED;
                }

                break;

            case PROCESS_STDIN_ESC_SEQ_PROCESS:
                return nt__process_stdin_esc(buff, esc_seq_read_count, out_event, out_ignore);
        }
    }

    return NT_ERR_UNEXPECTED;
}

static int nt__process_stdin_utf32(
        uint8_t* utf8_sbyte,
        bool alt,
        struct nt_event* out_event,
        bool* out_ignore)
{
    if(out_ignore != NULL)
        *out_ignore = false;

    size_t utf32_len = uc_utf8_unit_len(utf8_sbyte[0]);
    if(utf32_len == SIZE_MAX)
        return NT_ERR_UNEXPECTED;

    if((utf32_len > 1) &&
    (nt__read_exact(STDIN_FILENO, utf8_sbyte + 1, utf32_len - 1) != 0))
    {
        return NT_ERR_UNEXPECTED;
    }

    uint32_t utf32;
    size_t utf32_width;
    int status = uc_utf8_to_utf32(utf8_sbyte, utf32_len, &utf32, 1, 0, &utf32_width);
    if(status != 0)
        return NT_ERR_UNEXPECTED;

    struct nt_key key_event = nt_key_utf32_new(utf32, alt);
    return nt__event_new(NT_EVENT_KEY, &key_event, sizeof(key_event), out_event);
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

static enum process_mouse_result 
nt__process_stdin_esc_mouse(
        uint8_t* buff,
        size_t read_count,
        struct nt_mouse* out_event);

static int nt__process_stdin_esc(
        uint8_t* buff,
        size_t read_count,
        struct nt_event* out_event,
        bool* out_ignore)
{
    if(out_ignore != NULL)
        *out_ignore = false;

    buff[read_count] = 0;

    struct nt_mouse mouse_event;
    enum process_mouse_result mouse_rv = nt__process_stdin_esc_mouse(
            buff, read_count, &mouse_event);
    if(mouse_rv == MOUSE_EVENT_SUPPORTED)
    {
        return nt__event_new(
                NT_EVENT_MOUSE, &mouse_event, sizeof(mouse_event), out_event);
    }
    else if(mouse_rv == MOUSE_EVENT_UNSUPPORTED)
    {
        if(out_ignore != NULL)
            *out_ignore = true;
        return 0;
    }

    int i;
    struct nt_key key;
    struct nt_term_info term = nt__term_get_used();
    for(i = 0; i < NT_ESC_KEY_OTHER; i++)
    {
        if(strcmp((char*)buff, term.esc_key_seqs[i]) == 0)
        {
            key = nt_key_esc_new(i);
            return nt__event_new(
                    NT_EVENT_KEY, &key, sizeof(key), out_event);
        }
    }

    key = nt_key_esc_new(NT_ESC_KEY_OTHER);
    return nt__event_new(NT_EVENT_KEY, &key, sizeof(key), out_event);
}

// ESC [ < Cb ; Cx ; Cy M
// Refactor sometimes...
static enum process_mouse_result
nt__process_stdin_esc_mouse(
        uint8_t* buff,
        size_t read_count,
        struct nt_mouse* out_event)
{
    if(!out_event) return NOT_MOUSE_EVENT;
    memset(out_event, 0, sizeof(*out_event));

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

    out_event->x = (cx > 0) ? (cx - 1) : 0;
    out_event->y = (cy > 0) ? (cy - 1) : 0;
    if(cb == 64)
        out_event->type = NT_MOUSE_SCROLL_UP;
    else
    {
        if(cb == 65)
            out_event->type = NT_MOUSE_SCROLL_DOWN;
        else
        {
            int click_res = cb & 0x03; 
            switch(click_res)
            {
                case 0:
                    out_event->type = NT_MOUSE_CLICK_LEFT;
                    break;
                case 1:
                    out_event->type = NT_MOUSE_CLICK_MIDDLE;
                    break;
                case 2:
                    out_event->type = NT_MOUSE_CLICK_RIGHT;
                    break;
                default:
                    return MOUSE_EVENT_UNSUPPORTED;
            }
        }
    }

    return MOUSE_EVENT_SUPPORTED;
}
