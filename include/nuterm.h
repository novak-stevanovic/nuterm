/* TODO:
 * 1. Implement conversion functions between colors
 * 2. Handle default color
 * 3. Add other terminal support(esc keys, funcs, detection...)
 * 4. Improve documentation
 * 5. keysets? */
#ifndef _NUTERM_H_
#define _NUTERM_H_

#include "nt_esc.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "nt_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* GENERAL */
/* ------------------------------------------------------------------------- */

/* Initializes the nuterm library.
 *
 * STATUS CODES:
 * 1) NT_SUCCESS,
 * 2) NT_ERR_ALLOC_FAIL - failure to allocate memory needed for internal
 * resources,
 * 3) NT_ERR_INIT_PIPE - pipe() failed and errno was set to ENFILE or EMFILE,
 * 4) NT_ERR_INIT_TERM_ENV - failure to detect terminal due to $TERM not being
 * set,
 * 5) NT_ERR_TERM_NOT_SUPPORTED - terminal emulator not supported,
 * 6) NT_ERR_UNEXPECTED. */
void nt_init(nt_status_t* out_status);

/* Destroys the library and reverts terminal settings to old values.
 * Frees dynamically allocated memory. */
void nt_destroy();

/* ------------------------------------------------------------------------- */
/* COLOR & STYLE */
/* ------------------------------------------------------------------------- */

struct nt_rgb { uint8_t r, g, b; };

typedef struct nt_color
{
    uint8_t _code8;
    uint8_t _code256;
    struct nt_rgb _rgb;
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

bool nt_style_cmp(nt_style_t s1, nt_style_t s2);

/* ------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* ------------------------------------------------------------------------- */

/* Enables buffering. All terminal functions and text(appended to the buffer
 * by calls to nt_write_char()/nt_write_str()) will be buffered until a call to
 * nt_flush(). */
void nt_buffer_enable();

void nt_buffer_flush();

typedef enum nt_buffact { NT_BUFF_FLUSH, NT_BUFF_DISCARD } nt_buffact_t;

/* Disables buffering. Parameter `action` dictates if the currently buffered
 * content will be printed to the screen or discarded. */
void nt_buffer_disable(nt_buffact_t action);

/* ----------------------------------------------------- */

/* The functions below share the same STATUS CODES:
 *
 * 1) NT_SUCCESS,
 * 2) NT_ERR_ALLOC_FAIL - buffering is enabled and allocation to expand the
 * buffer failed,
 * 3) NT_ERR_TERM_UNKNOWN - variable indicating which terminal emulator is
 * being used is not set(nt_init() failed),
 * 4) NT_ERR_FUNC_NOT_SUPPORTED - terminal emulator doesn't support this
 * function,
 * 5) NT_ERR_UNEXPECTED. */

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

/* Converts UTF-32 `codepoint` to UTF-8 and then invokes nt_write_str().
 *
 * The status codes returned match those specified for nt_write_str().
 * Additionally, the following status codes can be returned:
 * 1) NT_ERR_INVALID_UTF32 - if `codepoint` is invalid or has a surrogate
 * value. */
void nt_write_char(uint32_t codepoint, struct nt_gfx gfx, size_t x, size_t y,
        nt_style_t* out_styles, nt_status_t* out_status);

/* Prints null-terminated `str` to screen. The text printed will have
 * graphical attributes described by struct `gfx` and the text will be printed
 * at the provided coordinates.
 * 
 * If buffering is enabled, the printing will occur only when nt_flush()
 * is called. 
 *
 * If a style is specified in `gfx` but the terminal doesn't support the style,
 * the status variable will not indicate this. Instead, `out_styles` will be set
 * to indicate successfully set styles.
 *
 * STATUS CODES:
 * 1) NT_SUCCESS, 
 * 2) NT_ERR_FUNC_NOT_SUPPORTED - one of the functions invoked is not supported
 * by the terminal - moving the cursor, resetting gfx, setting color.
 * 3) NT_ERR_ALLOC_FAIL - buffering is enabled and allocation to expand the
 * buffer failed,
 * 4) NT_ERR_UNEXPECTED. */
void nt_write_str(const char* str, struct nt_gfx gfx, size_t x, size_t y,
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

#define NT_EVENT_WAIT_FOREVER -1

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
/* END */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif // _NUTERM_H_
