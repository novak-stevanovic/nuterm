/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef NT_H
#define NT_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "nt_shared.h"
#include "nt_event.h"
#include "nt_gfx.h"
#include "nt_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* GENERAL */
/* -------------------------------------------------------------------------- */

/* Initializes the nuterm library. Calling any of the functions from the library
 * without initializing first is undefined behavior.
 *
 * ERROR CODES:
 * 1) NT_ERR_ALLOC_FAIL - failure to allocate memory needed for internal
 * resources,
 * 2) NT_ERR_INIT_PIPE - pipe() failed and errno was set to ENFILE or EMFILE,
 * 3) NT_ERR_INIT_TERM_ENV - failure to detect terminal due to $TERM not being
 * set,
 * 4) NT_ERR_TERM_NOT_SUPP - terminal emulator not supported - library
 * will assume that the emulator is compatible with xterm,
 * 5) NT_ERR_UNEXPECTED. */
NT_API int nt_init(void);

/* Destroys the library and reverts terminal settings to old values.
 * Frees resources used by the library. Output will NOT be flushed(if
 * buffering is on). */
NT_API void nt_deinit(void);

/* -------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* -------------------------------------------------------------------------- */

/* It is possible to enable buffering to avoid excessive writing to terminal
 * (this includes terminal function codes and text). */

/* ------------------------------------------------------ */
/* BUFFERING */
/* ------------------------------------------------------ */

enum nt_buffact
{ 
    NT_BUFF_DISCARD,
    NT_BUFF_FLUSH // flush the contents to stdout
};

/* Enables buffering. When `buff` reaches its capacity, its contents will be
 * flushed to stdout. If buffering is already enabled, this function will return
 * with an error code.
 *
 * ERROR CODES:
 * 1) NT_ERR_INVALID_ARG - `buff` is NULL, `cap` is 0,
 * 2) NT_ERR_ALR_BUFF - buffering is already enabled. */

NT_API int nt_buffer_enable(char* buff, size_t cap);

/* Disables buffering. `buffact` dictates what happens to the contents of the
 * buffer. If buffering is already disabled, this function has no effect.
 * Buffering is disabled even if flushing fails. `out_buff` receives the used
 * buffer when non-NULL.
 *
 * ERROR CODES:
 * 1) NT_ERR_UNEXPECTED - flushing to stdout failed. Subsequent output
 * operations may still be attempted. */

NT_API int nt_buffer_disable(enum nt_buffact buffact, char** out_buff);

/* Flushes the buffer to stdout if buffering is currently enabled. Buffered
 * contents are discarded after the flush attempt, even if writing fails.
 *
 * ERROR CODES:
 * 1) NT_ERR_UNEXPECTED - writing to stdout failed. Subsequent output
 * operations may still be attempted. */

NT_API int nt_buffer_flush(void);

/* ------------------------------------------------------ */
/* CORE */
/* ------------------------------------------------------ */

/* Prints `str` of size `len` to screen. The text printed will have
 * graphical attributes described by struct `gfx` and the text will be printed
 * at current cursor position.
 *
 * If buffering is enabled, the printing will occur only when nt_flush()
 * is called. 
 *
 * If a style is specified in `gfx` but the terminal doesn't support the style,
 * the return value will not indicate this.
 *
 * ERROR CODES:
 * 1) NT_ERR_FUNC_NOT_SUPP - one of the functions invoked is not supported
 * by the terminal - resetting gfx, setting color.
 * 2) NT_ERR_UNEXPECTED - output could not be completed. Subsequent output
 * operations may still be attempted. */

NT_API int 
nt_write_str(const char* str, size_t len, struct nt_gfx gfx);

NT_API int
nt_write_str_unsafe(const char* str, struct nt_gfx gfx);

/* Functions below may be used for moving the cursor, or setting the gfx
 * of a program without actually writing anything to the screen. */

/* The functions below share the same status codes:
 *
 * 1) 0 - success,
 * 2) NT_ERR_FUNC_NOT_SUPP - terminal emulator doesn't support this
 * function(not very reliable),
 * 3) NT_ERR_UNEXPECTED - output could not be completed. Subsequent output
 * operations may still be attempted.
 *
 * With buffering enabled, output is buffered. */

NT_API int nt_cursor_hide(void);
NT_API int nt_cursor_show(void);
/* Zero-based indexing. When an attempt is made to move the cursor out of bounds,
 * the position is silently capped. */
NT_API int nt_cursor_move(size_t x, size_t y);

NT_API int nt_erase_screen(void);
NT_API int nt_erase_line(void);
NT_API int nt_erase_scrollback(void);

NT_API int nt_alt_screen_enable(void);
NT_API int nt_alt_screen_disable(void);

// Returns 0 on success.
NT_API int nt_mouse_mode_enable(void);
NT_API int nt_mouse_mode_disable(void);

NT_API void nt_get_term_size(size_t* out_width, size_t* out_height);

/* -------------------------------------------------------------------------- */
/* EVENT */
/* -------------------------------------------------------------------------- */

#define NT_EVENT_WAIT_FOREVER ((unsigned int)-1)

/* Waits for an event. The thread is blocked until the event occurs. Meant to be
 * used for main loop in TGUI applications. Should be called from the main thread.
 *
 * A resize triggers both a SIGWINCH signal event and a resize event. If several
 * resize events are queued, only the last one is delivered.
 *
 * `out_elapsed` receives elapsed time in milliseconds. If an error occurs, the
 * type of `out_event` will be NT_EVENT_INVALID.
 *
 * ERROR CODES:
 * 1) 0 - success,
 * 2) NT_ERR_UNEXPECTED - a hard event I/O failure occurred. After this error,
 * the event subsystem should not be assumed to be usable. */

NT_API int 
nt_event_wait(struct nt_event* out_event, unsigned int timeout,
              unsigned int* out_elapsed);

/* Removes queued events until the queue is empty. Stops and reports an error if
 * nt_event_wait() encounters a hard event I/O failure. */
NT_API int nt_event_queue_drain(void);

/* Pushes event to queue. This will wake the thread which is blocked on
 * `nt_event_wait()`. If the calling thread is the main thread, next call
 * to `nt_event_wait()` will return with the pushed event right away.
 *
 * It is possible to push built-in library events(NT_EVENT_KEY, for example).
 * Make sure to provide the correct payload and handle such situations properly.
 *
 * Thread-safe.
 
 * ERROR CODES:
 * 1) NT_ERR_INVALID_ARG - `event` did not pass nt_event_is_valid() check,
 * 2) NT_ERR_UNEXPECTED. */

NT_API int nt_event_push(const struct nt_event* event);

#ifdef __cplusplus
}
#endif

#endif // NT_H
