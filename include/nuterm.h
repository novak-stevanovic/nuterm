/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */

// TODO: add support for other terminals */
#ifndef _NUTERM_H_
#define _NUTERM_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "nt_esc.h"
#include "nt_shared.h"

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
void nt_init(nt_status_t* out_status);

/* Destroys the library and reverts terminal settings to old values.
 * Frees dynamically allocated memory. */
void nt_destroy();

/* ------------------------------------------------------------------------- */
/* COLOR & STYLE */
/* ------------------------------------------------------------------------- */

/* Colors are meant to be created by invoking nt_color_new(). Provided RGB
 * values are translated into color codes(0-7 & 0-255). When writing text to
 * terminal, terminal color will be set based on supported color palette. */
typedef struct nt_color
{
    uint8_t _code8;
    uint8_t _code256;
    struct 
    {
        uint8_t r, g, b;
    } _rgb;
    bool __default;
} nt_color_t;

extern const nt_color_t NT_COLOR_DEFAULT;

nt_color_t nt_color_new(uint8_t r, uint8_t g, uint8_t b);
bool nt_color_cmp(nt_color_t c1, nt_color_t c2);

/* ----------------------------------------------------- */

typedef uint8_t nt_style_t;

extern const nt_style_t NT_STYLE_DEFAULT;

#define NT_STYLE_DEFAULT         0
#define NT_STYLE_BOLD            (1 << 0)  // 00000001
#define NT_STYLE_FAINT           (1 << 1)  // 00000010
#define NT_STYLE_ITALIC          (1 << 2)  // 00000100
#define NT_STYLE_UNDERLINE       (1 << 3)  // 00001000
#define NT_STYLE_BLINK           (1 << 4)  // 00010000
#define NT_STYLE_REVERSE         (1 << 5)  // 00100000
#define NT_STYLE_HIDDEN          (1 << 6)  // 01000000
#define NT_STYLE_STRIKETHROUGH   (1 << 7)  // 10000000

/* ------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* ------------------------------------------------------------------------- */

/* It is possible to enable buffering to avoid excessive writing to terminal
 * (this includes terminal function codes and text). */

/* ------------------------------------------------------ */

/* Sets capacity of internal output buffer. If `buff_cap` == 0, buffering is
 * disabled. If the buffer contains content, and the content's length is 
 * greater than `buff_cap`, a flush will occur.

 * STATUS CODES:
 * 1) NT_SUCCESS,
 * 2) NT_ERR_ALLOC_FAIL - if `buff_cap` is greater than the current buffer
 * capacity, a realloc() call may fail. */
void nt_buffer_set_cap(size_t buff_cap, nt_status_t* out_status);

/* Capacity of internal output buffer */
#define NT_BUFFER_CAP_DEFAULT 10000

/* Convenience macros. */
#define nt_buffer_enable() nt_buffer_set_cap(NT_BUFFER_CAP_DEFAULT, NULL)
#define nt_buffer_disable() nt_buffer_set_cap(0, NULL)

/* Flushes the content of the internal output buffer onto the screen. */
void nt_buffer_flush();

/* ----------------------------------------------------- */

/* The functions below share the same STATUS CODES:
 *
 * 1) NT_SUCCESS,
 * 2) NT_ERR_ALLOC_FAIL - buffering is enabled and allocation to expand the
 * buffer failed,
 * 3) NT_ERR_FUNC_NOT_SUPPORTED - terminal emulator doesn't support this
 * function,
 * 4) NT_ERR_UNEXPECTED. */

void nt_cursor_hide(nt_status_t* out_status);
void nt_cursor_show(nt_status_t* out_status);

void nt_erase_screen(nt_status_t* out_status);
void nt_erase_line(nt_status_t* out_status);
void nt_erase_scrollback(nt_status_t* out_status);

void nt_alt_screen_enable(nt_status_t* out_status);
void nt_alt_screen_disable(nt_status_t* out_status);

/* ------------------------------------------------------------------------- */
/* WRITE TO TERMINAL */
/* ------------------------------------------------------------------------- */

struct nt_gfx
{
    nt_color_t fg, bg;
    nt_style_t style;
};

extern const struct nt_gfx NT_GFX_DEFAULT;

#define NT_WRITE_INPLACE -1

/* Converts UTF-32 `codepoint` to UTF-8 and then invokes nt_write_str().
 *
 * The status codes returned match those specified for nt_write_str().
 * Additionally, the following status codes can be returned:
 * 1) NT_ERR_INVALID_UTF32 - if `codepoint` is invalid or has a surrogate
 * value. */
void nt_write_char(uint32_t codepoint, struct nt_gfx gfx, ssize_t x, ssize_t y,
        nt_style_t* out_styles, nt_status_t* out_status);

/* Prints null-terminated `str` to screen. The text printed will have
 * graphical attributes described by struct `gfx` and the text will be printed
 * at the provided coordinates.
 *
 * If x == NT_WRITE_INPLACE && y == NT_WRITE_INPLACE, the cursor will not move
 * before printing to screen.
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
 * by the terminal - moving the cursor, resetting gfx, setting color,
 * 3) NT_ERR_ALLOC_FAIL - buffering is enabled and allocation to expand the
 * buffer failed,
 * 4) NT_ERR_UNEXPECTED. */
void nt_write_str(const char* str, struct nt_gfx gfx, ssize_t x, ssize_t y,
        nt_style_t* out_styles, nt_status_t* out_status);

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

typedef enum nt_key_event_type
{ 
    NT_KEY_EVENT_UTF32,
    NT_KEY_EVENT_ESC_KEY
} nt_key_event_type_t;

struct nt_key_event
{
    nt_key_event_type_t type;
    union
    {
        struct
        {
            size_t codepoint;
            bool alt;
        } utf32_data;

        struct
        {
            nt_esc_key_t esc_key;
        } esc_key_data;
    };
};

struct nt_resize_event
{
    struct nt_xy new_size;
};

struct nt_timeout_event
{
    uint64_t elapsed;
};

/* Event */

typedef enum nt_event_type
{
    NT_EVENT_TYPE_KEY,
    NT_EVENT_TYPE_RESIZE,
    NT_EVENT_TYPE_TIMEOUT
} nt_event_type_t;

struct nt_event
{
    nt_event_type_t type;
    union
    {
        struct nt_key_event key_data;
        struct nt_resize_event resize_data;
        struct nt_timeout_event timeout_data;
    };
};

#define NT_WAIT_FOREVER -1

/* Waits for an event(key event, resize event or "timeout event"). 
 * The thread is blocked until the event occurs.
 * A "timeout event" occurs when the `timeout` milliseconds had passed. If
 * `timeout` is set to NT_EVENT_WAIT_FOREVER, the blocking will last until
 * a key/resize eent occurs.
 *
 * STATUS CODES:
 * 1) NT_SUCCESS,
 * 2) NT_ERR_UNEXPECTED - this can occur, for example, if read(), write()
 * or poll() fails and the failure is not internally handled. */
struct nt_event nt_wait_for_event(int timeout, nt_status_t* out_status);

/* ------------------------------------------------------------------------- */

/* Nu-term allows you to bind key event handlers to key evnets. This may be
 * useful when creating a TGUI application, for example. A single key event
 * can have only one entry in the keymap. */

typedef struct nt_keymap* nt_keymap;

/* Creates the nt_keymap, initializes internal hashmap.
 *
 * STATUS CODES:
 * 1. NT_SUCCESS,
 * 2. NT_ERR_ALLOC_FAIL - dynamic allocation for the internal struct nt_keymap
 * failed. */
nt_keymap nt_keymap_new(nt_status_t* out_status);

/* Destroys the nt_keymap. Frees the dynamially allocated memory. */
void nt_keymap_destroy(nt_keymap map);

typedef void (*nt_key_handler_t)(struct nt_key_event key_event);

/* Binds a specific `event_handler` to given `key_event`. This will create an
 * entry inside the internal hashmap. Function assumes `key_event` is valid.
 *
 * STATUS CODES:
 * 1. NT_SUCCESS, 
 * 2. NT_ERR_INVALID_ARG - `map` or `event_handler` is NULL,
 * 3. NT_ERR_BIND_ALREADY_EXISTS - `key_event` is already a key inside the
 * hashmap,
 * 4. NT_ERR_ALLOC_FAIL - dynamic allocation failed. */
void nt_keymap_bind(nt_keymap map, struct nt_key_event key_event,
        nt_key_handler_t event_handler, nt_status_t* out_status);

/* Unbinds a specific `event_handler` from the given `key_event`. This will
 * remove the entry inside the internal hashmap. If `key_event` is not a key
 * inside the hashmap, NT_SUCCESS is returned.
 *
 * STATUS CODES:
 * 1. NT_SUCCESS, 
 * 2. NT_ERR_INVALID_ARG - `map` is NULL. */
void nt_keymap_unbind(nt_keymap map, struct nt_key_event key_event,
        nt_status_t* out_status);

/* Retrieves nt_key_handler_t tied to given `key_event`. If `key_event` is not
 * an entry inside the `map`, NULL is returned.
 *
 * 1. NT_SUCCESS, 
 * 2. NT_ERR_INVALID_ARG - `map` is NULL. */
nt_key_handler_t nt_keymap_get(nt_keymap map, struct nt_key_event key_event,
        nt_status_t* out_status);

/* ------------------------------------------------------------------------- */
/* END */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif // _NUTERM_H_
