/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef _NUTERM_H_
#define _NUTERM_H_

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "nt_charbuff.h"
#include "nt_shared.h"
#include "nt_event.h"
#include "nt_gfx.h"
#include "nt_esc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* GENERAL */
/* ------------------------------------------------------------------------- */

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
 * 5) NT_ERR_TERM_NOT_SUPPORTED - terminal emulator not supported - library
 * will assume that the emulator is compatible with xterm,
 * 6) NT_ERR_UNEXPECTED. */
void __nt_init__(nt_status* out_status);

/* Destroys the library and reverts terminal settings to old values.
 * Frees dynamically allocated memory. */
void __nt_deinit__();

/* ------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* ------------------------------------------------------------------------- */

/* It is possible to enable buffering to avoid excessive writing to terminal
 * (this includes terminal function codes and text). */

/* ------------------------------------------------------ */

/* Enables buffering. When `buff` reaches its capacity, its contents will be
 * flushed to stdout. If buffering is already enabled, this function has no
 * effect. */
void nt_buffer_enable(nt_charbuff* buff);

typedef enum nt_buffact
{ 
    NT_BUFF_KEEP, // keep the contents inside the nt_charbuff
    NT_BUFF_DISCARD, // rewind the str pointer of the nt_charbuff
    NT_BUFF_FLUSH // flush the contents of the nt_charbuff to stdout
} nt_buffact;

/* Disables buffering. `buffact` dictates what happens to the contents of the
 * buffer. If buffering is already disabled, this function has no effect. */
void nt_buffer_disable(nt_buffact buffact);

/* Returns the current buffer. Can be used to manually free the nt_charbuff. */
nt_charbuff* nt_buffer_get();

/* Flushes the buffer to stdout if buffering is currently enabled. */
void nt_buffer_flush();

/* ----------------------------------------------------- */

void nt_get_term_size(size_t* out_width, size_t* out_height);

/* The functions below share the same STATUS CODES:
 *
 * 1) NT_SUCCESS,
 * 2) NT_ERR_ALLOC_FAIL - buffering is enabled and allocation to expand the
 * buffer failed,
 * 3) NT_ERR_FUNC_NOT_SUPPORTED - terminal emulator doesn't support this
 * function,
 * 4) NT_ERR_UNEXPECTED. */

void nt_cursor_hide(nt_status* out_status);
void nt_cursor_show(nt_status* out_status);

void nt_erase_screen(nt_status* out_status);
void nt_erase_line(nt_status* out_status);
void nt_erase_scrollback(nt_status* out_status);

void nt_alt_screen_enable(nt_status* out_status);
void nt_alt_screen_disable(nt_status* out_status);

/* ------------------------------------------------------------------------- */
/* WRITE TO TERMINAL */
/* ------------------------------------------------------------------------- */

/* Converts UTF-32 `codepoint` to UTF-8 and then invokes nt_write_str().
 *
 * The status codes returned match those specified for nt_write_str().
 * Additionally, the following status codes can be returned:
 * 1) NT_ERR_INVALID_UTF32 - if `codepoint` is invalid or has a surrogate
 * value. */
void nt_write_char(uint32_t codepoint, struct nt_gfx gfx, nt_style* out_styles,
        nt_status* out_status);

/* Prints null-terminated `str` to screen. The text printed will have
 * graphical attributes described by struct `gfx` and the text will be printed
 * at current cursor position.
 *
 * If buffering is enabled, the printing will occur only when nt_flush()
 * is called. 
 *
 * If a style is specified in `gfx` but the terminal doesn't support the style,
 * the status variable will not indicate this. Instead, `out_styles` will be set
 * to indicate successfully set styles. Keep in mind that some terminal emulators
 * view some styles as mutually exclusive(for example, bold and italic may not
 * be set at the same time). `out_styles` will not be able to indicate this.
 *
 * STATUS CODES:
 * 1) NT_SUCCESS, 
 * 2) NT_ERR_FUNC_NOT_SUPPORTED - one of the functions invoked is not supported
 * by the terminal - resetting gfx, setting color.
 * 3) NT_ERR_ALLOC_FAIL - buffering is enabled and allocation to expand the
 * buffer failed,
 * 4) NT_ERR_UNEXPECTED. */
void nt_write_str(const char* str, struct nt_gfx gfx, nt_style* out_styles,
        nt_status* out_status);

/* Moves cursor to specified `row` and `col` and then invokes nt_write_char(). 
 *
 * The status codes returned match those specified for nt_write_char().
 * Additionally, the following status codes can be returned:
 * 1) NT_ERR_FUNC_NOT_SUPPORTED - terminal doesn't support moving the cursor or
 * nt_write_char() failed with this code,
 * 2) NT_ERR_OUT_OF_BOUNDS. */
void nt_write_char_at(uint32_t codepoint, struct nt_gfx gfx, size_t x, size_t y,
        nt_style* out_styles, nt_status* out_status);

/* Moves cursor to specified `row` and `col` and then invokes nt_write_str(). 
 *
 * The status codes returned match those specified for nt_write_str().
 * Additionally, the following status codes can be returned:
 * 1) NT_ERR_FUNC_NOT_SUPPORTED - terminal doesn't support moving the cursor or
 * nt_write_str() failed with this code,
 * 2) NT_ERR_OUT_OF_BOUNDS. */
void nt_write_str_at(const char* str, struct nt_gfx gfx, size_t x, size_t y,
        nt_style* out_styles, nt_status* out_status);

/* ------------------------------------------------------------------------- */
/* EVENT */
/* ------------------------------------------------------------------------- */

/* Key events occur on (most) key-presses. They are divided into 2 categories:
 *
 * 1) UTF-32 key events occur when the terminal emulator delivers a UTF-32
 * codepoint. This can occur, for example, if 'K' or cyrillic 'S' is pressed
 * on the keyboard. The key-press may be accompanied by an ALT key-hold.
 * If this is the case, modifier `alt` inside `ut32_data` is set.
 *
 * 2) Escape key-presses - Some keys on a keyboard emit special escape sequences
 * (they are specific to the terminal emulator used).
 * Some of these keys include: F1, F12, Insert, Delete...
 * When the function detects an escape sequence, it attempts to figure out which
 * escape key had generated it. If it can't pinpoint the exact escape key pressed,
 * `esc_key` inside struct `esc_key_data` will be set to NT_ESC_KEY_OTHER. */

/* ------------------------------------------------------------------------- */

#define NT_WAIT_FOREVER -1

/* Waits for an event(key event, resize event or "timeout event"). 
 * The thread is blocked until the event occurs.
 * A "timeout event" occurs when the `timeout` milliseconds had passed. If
 * `timeout` is set to NT_EVENT_WAIT_FOREVER, the blocking will last until
 * a key/resize event occurs.
 *
 * STATUS CODES:
 * 1) NT_SUCCESS,
 * 2) NT_ERR_UNEXPECTED - this can occur, for example, if read(), write()
 * or poll() fails and the failure is not internally handled. */
struct nt_event nt_wait_for_event(int timeout, nt_status* out_status);

#ifdef __cplusplus
}
#endif

#endif // _NUTERM_H_
