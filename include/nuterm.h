/* TODO:
 * 1. Implement detection of terminal & colors | (done for xterm)
 * 2. Implement conversion functions between colors
 * 3. Implement other functions inside the API
 * 4. Implement keybind sets that depend on key events
 * 5. Re-think error handling inside nt_wait_for_event()
 * 6. Add other terminal support */
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

void nt_init(nt_status_t* out_status);
void nt_destroy();

struct nt_xy { size_t x, y; };
struct nt_dxy { ssize_t x, y; };

/* ------------------------------------------------------------------------- */
/* COLOR & STYLE */
/* ------------------------------------------------------------------------- */

struct nt_rgb { uint8_t r, g, b; };

typedef struct nt_color
{
    uint8_t _color_c8;
    uint8_t _color_c256;
    struct nt_rgb _color_rgb;
    bool __default;
} nt_color_t;

extern const nt_color_t NT_COLOR_DEFAULT;

nt_color_t nt_color_create(uint8_t r, uint8_t g, int8_t b);
bool nt_color_are_equal(nt_color_t c1, nt_color_t c2);

/* ----------------------------------------------------- */

typedef uint8_t nt_style_t;

extern const nt_style_t NT_STYLE_DEFAULT;

#define NT_GFX_STYLE_DEFAULT         0
#define NT_GFX_STYLE_BOLD            (1 << 0)  // 00000001
#define NT_GFX_STYLE_FAINT           (1 << 1)  // 00000010
#define NT_GFX_STYLE_ITALIC          (1 << 2)  // 00000100
#define NT_GFX_STYLE_UNDERLINE       (1 << 3)  // 00001000
#define NT_GFX_STYLE_BLINK           (1 << 4)  // 00010000
#define NT_GFX_STYLE_REVERSE         (1 << 5)  // 00100000
#define NT_GFX_STYLE_HIDDEN          (1 << 6)  // 01000000
#define NT_GFX_STYLE_STRIKETHROUGH   (1 << 7)  // 10000000

bool nt_style_are_equal(nt_style_t s1, nt_style_t s2);

/* ------------------------------------------------------------------------- */
/* TERMINAL FUNCTIONS */
/* ------------------------------------------------------------------------- */

void nt_buffer_enable();
void nt_buffer_flush();

typedef enum nt_buffact { NT_BUFF_FLUSH, NT_BUFF_DISCARD } nt_buffact_t;

void nt_buffer_disable(nt_buffact_t action);

/* ----------------------------------------------------- */

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
    nt_color_t foreground, background;
    nt_style_t style;
};

// UTF-32
void nt_write_char(uint32_t codepoint, struct nt_gfx gfx, size_t x, size_t y);

// UTF-8
void nt_write_str(const char* str, struct nt_gfx gfx, size_t x, size_t y);

/* ------------------------------------------------------------------------- */
/* EVENT */
/* ------------------------------------------------------------------------- */

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

struct nt_event nt_wait_for_event(int timeout, nt_status_t* out_status);

/* ------------------------------------------------------------------------- */
/* END */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif // _NUTERM_H_
