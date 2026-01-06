/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
// TODO:
// 1) implement mouse event detection
// 5) cleanup if init fails
#ifndef _NUTERM_H_
#define _NUTERM_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "nt_event.h"
#include "nt_gfx.h"
#include "nt_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* GENERAL */
/* -------------------------------------------------------------------------- */

/* Initializes the nuterm library. Calling any of the functions from the library
 * without initializing first is undefined behavior.
 *
 * STATUS CODES:
 * 1) NT_SUCCESS,
 * 2) NT_ERR_ALLOC_FAIL - failure to allocate memory needed for internal
 * resources,
 * 3) NT_ERR_INIT_PIPE - pipe() failed and errno was set to ENFILE or EMFILE,
 * 4) NT_ERR_INIT_TERM_ENV - failure to detect terminal due to $TERM not being
 * set,
 * 5) NT_ERR_TERM_NOT_SUPP - terminal emulator not supported - library
 * will assume that the emulator is compatible with xterm,
 * 6) NT_ERR_UNEXPECTED. */
void nt_init(nt_status* out_status);

/* Destroys the library and reverts terminal settings to old values.
 * Frees resources used by the library. Output will NOT be flushed(if
 * buffering is on). */
void nt_deinit();

/* -------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* -------------------------------------------------------------------------- */

/* It is possible to enable buffering to avoid excessive writing to terminal
 * (this includes terminal function codes and text). */

/* ------------------------------------------------------ */

typedef enum nt_buffact
{ 
    NT_BUFF_DISCARD,
    NT_BUFF_FLUSH // flush the contents to stdout
} nt_buffact;

/* Enables buffering. When `buff` reaches its capacity, its contents will be
 * flushed to stdout. If buffering is already enabled, this function will return
 * with an error code.
 *
 * STATUS CODES:
 * 1) NT_SUCCESS,
 * 2) NT_ERR_INVALID_ARG - `buff` is NULL, `cap` is 0,
 * 3) NT_ERR_ALR_BUFF - buffering is already enabled. */

void nt_buffer_enable(char* buff, size_t cap, nt_status* out_status);

/* Disables buffering. `buffact` dictates what happens to the contents of the
 * buffer. If buffering is already disabled, this function has no effect.
 * Returns used buffer. */

char* nt_buffer_disable(nt_buffact buffact);

/* Flushes the buffer to stdout if buffering is currently enabled. */

void nt_buffer_flush();

/* ----------------------------------------------------- */

void nt_get_term_size(size_t* out_width, size_t* out_height);

/* The functions below share the same STATUS CODES:
 *
 * 1) NT_SUCCESS,
 * 2) NT_ERR_FUNC_NOT_SUPP - terminal emulator doesn't support this
 * function(not very reliable),
 * 3) NT_ERR_UNEXPECTED. */

void nt_cursor_hide(nt_status* out_status);
void nt_cursor_show(nt_status* out_status);

void nt_erase_screen(nt_status* out_status);
void nt_erase_line(nt_status* out_status);
void nt_erase_scrollback(nt_status* out_status);

void nt_alt_screen_enable(nt_status* out_status);
void nt_alt_screen_disable(nt_status* out_status);

// Status is always NT_SUCCESS
void nt_mouse_mode_enable(nt_status* out_status);
void nt_mouse_mode_disable(nt_status* out_status);

/* -------------------------------------------------------------------------- */
/* WRITE TO TERMINAL */
/* -------------------------------------------------------------------------- */

/* Converts UTF-32 `codepoint` to UTF-8 and then invokes nt_write_str().
 *
 * The status codes returned match those specified for nt_write_str().
 * Additionally, the following status codes can be returned:
 * 1) NT_ERR_INVALID_UTF32 - if `codepoint` is invalid or has a surrogate
 * value. */

void nt_write_char(uint32_t codepoint, struct nt_gfx gfx, nt_status* out_status);

/* Prints `str` of size `len` to screen. The text printed will have
 * graphical attributes described by struct `gfx` and the text will be printed
 * at current cursor position.
 *
 * If buffering is enabled, the printing will occur only when nt_flush()
 * is called. 
 *
 * If a style is specified in `gfx` but the terminal doesn't support the style,
 * the status variable will not indicate this.
 *
 * STATUS CODES:
 * 1) NT_SUCCESS, 
 * 2) NT_ERR_FUNC_NOT_SUPP - one of the functions invoked is not supported
 * by the terminal - resetting gfx, setting color.
 * 3) NT_ERR_UNEXPECTED. */

void nt_write_str(const char* str, size_t len, struct nt_gfx gfx, nt_status* out_status);

/* Moves cursor to specified `row` and `col` and then invokes nt_write_char(). 
 *
 * The status codes returned match those specified for nt_write_char().
 * Additionally, the following status codes can be returned:
 * 1) NT_ERR_FUNC_NOT_SUPP - terminal doesn't support moving the cursor or
 * nt_write_char() failed with this code,
 * 2) NT_ERR_OUT_OF_BOUNDS. */

void nt_write_char_at(uint32_t codepoint, struct nt_gfx gfx, size_t x, size_t y,
        nt_status* out_status);

/* Moves cursor to specified `row` and `col` and then invokes nt_write_str(). 
 *
 * The status codes returned match those specified for nt_write_str().
 * Additionally, the following status codes can be returned:
 * 1) NT_ERR_FUNC_NOT_SUPP - terminal doesn't support moving the cursor or
 * nt_write_str() failed with this code,
 * 2) NT_ERR_OUT_OF_BOUNDS. */

void nt_write_str_at(const char* str, size_t len, struct nt_gfx gfx,
        size_t x, size_t y, nt_status* out_status);

/* -------------------------------------------------------------------------- */
/* EVENT */
/* -------------------------------------------------------------------------- */

#define NT_EVENT_WAIT_FOREVER -1

/* Waits for an event. The thread is blocked until the event occurs. Meant to be
 * used for main loop in TGUI applications. Should be called from the main thread.
 *
 * A resize triggers both a SIGWINCH signal event and a resize event. If several
 * resize events are queued, only the last one is delivered.
 *
 * Returns elapsed time(in milliseconds). If an error occurs, the type of`out_event`
 * will be NT_EVENT_INVALID.
 *
 * STATUS CODES:
 * 1) NT_SUCCESS,
 * 2) NT_ERR_UNEXPECTED. */

unsigned int nt_event_wait(struct nt_event* out_event, unsigned int timeout,
        nt_status* out_status);

/* Pushes event to queue. This will wake the thread which is blocked on
 * `nt_event_wait()`. If the calling thread is the main thread, next call
 * to `nt_event_wait()` will return with the pushed event right away.
 *
 * It is possible to push built-in library events(NT_EVENT_KEY, for example).
 * Make sure to provide the correct payload and handle such situations properly.
 *
 * Thread-safe.
 
 * STATUS CODES:
 * 1) NT_SUCCESS,
 * 2) NT_ERR_INVALID_ARG - `event` did not pass nt_event_is_valid() check,
 * 3) NT_ERR_UNEXPECTED. */

void nt_event_push(struct nt_event event, nt_status* out_status);

#ifdef __cplusplus
}
#endif

#endif // _NUTERM_H_
