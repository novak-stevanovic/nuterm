/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef NT_H
#define NT_H

#include "nt_shared.h"
#include "nt_event.h"
#include "nt_gfx.h"
#include "nt_error.h"

/* ========================================================================== */
/* INIT/DEINIT */
/* ========================================================================== */

/* Initializes the nuterm library. The library must be initialized before use.
 *
 * ERROR CODES:
 * 1) NT_ERR_INIT_PIPE - Failed to create an internal pipe.
 * 2) NT_ERR_INIT_TERM_ENV - The TERM environment variable is not set.
 * 3) NT_ERR_TERM_NOT_SUPP - The terminal was not recognized. The library
 * remains initialized and assumes xterm compatibility.
 * 4) NT_ERR_UNEXPECTED - Terminal or signal setup failed. */

NT_API int nt_init(void);

/* ------------------------------------------------------ */

/* Deinitializes the library and restores terminal state. Buffered output is
 * discarded without being flushed. */

NT_API void nt_deinit(void);

/* ========================================================================== */
/* TERMINAL FUNCTIONS */
/* ========================================================================== */

/* ------------------------------------------------------ */
/* BUFFERING */
/* ------------------------------------------------------ */

enum nt_buffact
{
    NT_BUFF_DISCARD,
    NT_BUFF_FLUSH // flush the contents to stdout
};

/* ------------------------------------------------------ */

/* Enables output buffering using `buff` with capacity `cap`. The buffer is
 * flushed automatically when full.
 *
 * ERROR CODES:
 * 1) NT_ERR_INVALID_ARG - `buff` is NULL or `cap` is 0.
 * 2) NT_ERR_ALR_BUFF - Buffering is already enabled. */

NT_API int nt_buffer_enable(char* buff, size_t cap);

/* ------------------------------------------------------ */

/* Disables buffering. `buffact` selects whether pending output is discarded or
 * flushed. If provided, `out_buff` receives the previously used buffer.
 *
 * ERROR CODES:
 * 1) NT_ERR_UNEXPECTED - Flushing buffered output failed. */

NT_API int nt_buffer_disable(enum nt_buffact buffact, char** out_buff);

/* ------------------------------------------------------ */

/* Flushes pending buffered output. Attempted contents are discarded even if
 * writing fails.
 *
 * ERROR CODES:
 * 1) NT_ERR_UNEXPECTED - Writing to stdout failed. */

NT_API int nt_buffer_flush(void);

/* ------------------------------------------------------ */
/* WRITE */
/* ------------------------------------------------------ */

/* Writes `len` bytes from `str` using `gfx` at the current cursor position.
 * Output is buffered when buffering is enabled.
 *
 * ERROR CODES:
 * 1) NT_ERR_FUNC_NOT_SUPP - A required terminal function is unsupported.
 * 2) NT_ERR_UNEXPECTED - Output could not be completed. */

NT_API int
nt_write_str(const char* str, size_t len, struct nt_gfx gfx);

/* ------------------------------------------------------ */

/* Writes a NUL-terminated string using `gfx`.
 *
 * ERROR CODES:
 * 1) NT_ERR_FUNC_NOT_SUPP - A required terminal function is unsupported.
 * 2) NT_ERR_UNEXPECTED - Output could not be completed. */

NT_API int
nt_write_str_unsafe(const char* str, struct nt_gfx gfx);

/* ------------------------------------------------------ */

/* The terminal-control functions below buffer output when buffering is enabled.
 *
 * ERROR CODES:
 * 1) NT_ERR_FUNC_NOT_SUPP - The requested terminal function is unsupported.
 * 2) NT_ERR_UNEXPECTED - Output could not be completed. */

/* ------------------------------------------------------ */
/* CURSOR */
/* ------------------------------------------------------ */

NT_API int nt_cursor_hide(void);
NT_API int nt_cursor_show(void);

/* Moves the cursor to zero-based position (`x`, `y`). */
NT_API int nt_cursor_move(size_t x, size_t y);

/* ------------------------------------------------------ */
/* ERASE */
/* ------------------------------------------------------ */

NT_API int nt_erase_screen(void);
NT_API int nt_erase_line(void);
NT_API int nt_erase_scrollback(void);

/* ------------------------------------------------------ */
/* ALTERNATE SCREEN */
/* ------------------------------------------------------ */

NT_API int nt_alt_screen_enable(void);
NT_API int nt_alt_screen_disable(void);

/* ------------------------------------------------------ */
/* MOUSE */
/* ------------------------------------------------------ */

NT_API int nt_mouse_mode_enable(void);
NT_API int nt_mouse_mode_disable(void);

/* ------------------------------------------------------ */
/* MISC */
/* ------------------------------------------------------ */

/* Stores the terminal size in `out_width` and `out_height` when provided.
 * Stores 0 for both values if the terminal size cannot be read. */

NT_API void nt_get_term_size(size_t* out_width, size_t* out_height);

/* ========================================================================== */
/* LOOP */
/* ========================================================================== */

#define NT_EVENT_WAIT_FOREVER ((unsigned int)-1)

/* Waits up to `timeout` milliseconds for an event. `NT_EVENT_WAIT_FOREVER`
 * waits indefinitely. `out_elapsed` receives the total elapsed time when
 * provided.
 *
 * If an error occurs, `out_event` is set to NT_EVENT_INVALID when provided.
 * Queued resize events are coalesced and only the latest resize is delivered.
 *
 * ERROR CODES:
 * 1) NT_ERR_UNEXPECTED - A hard event I/O failure occurred. */

NT_API int
nt_event_wait(struct nt_event* out_event, unsigned int timeout,
              unsigned int* out_elapsed);

/* ------------------------------------------------------ */

/* Removes all queued events.
 *
 * ERROR CODES:
 * 1) NT_ERR_UNEXPECTED - A hard event I/O failure occurred. */

NT_API int nt_event_queue_drain(void);

/* ------------------------------------------------------ */

/* Pushes `event` to the event queue and wakes a waiting thread. Thread-safe.
 *
 * ERROR CODES:
 * 1) NT_ERR_INVALID_ARG - `event` is NULL or invalid.
 * 2) NT_ERR_UNEXPECTED - Writing the event to the queue failed. */

NT_API int nt_event_push(const struct nt_event* event);

/* ========================================================================== */

#endif // NT_H
