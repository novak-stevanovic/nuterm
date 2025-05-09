#ifndef _NUTERM_H_
#define _NUTERM_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int nt_status_t;

#define NT_SUCCESS 0
#define NT_ERR_PIPE 1
#define NT_ERR_UNEXPECTED 2

/* ------------------------------------------------------------------------- */
/* START */
/* ------------------------------------------------------------------------- */

void nt_init(nt_status_t* out_status);
void nt_destroy();

struct nt_xy { size_t x, y; };
struct nt_dxy { ssize_t x, y; };

/* ------------------------------------------------------------------------- */
/* GFX */
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

/* ------------------------------------------------------------------------- */

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
/* ENV */
/* ------------------------------------------------------------------------- */

void nt_cursor_set_pos(struct nt_xy pos);
void nt_cursor_hide();
void nt_cursor_show();

void nt_erase_screen(nt_color_t color);
void nt_erase_line(nt_color_t color);

struct nt_xy nt_display_get_size();

void nt_alt_screen_enable();
void nt_alt_screen_disable();

struct nt_cell
{
    uint32_t codepoint;
    nt_color_t foreground, background;
    nt_style_t style;
};

void nt_write_cell(struct nt_cell cell);

void nt_flush_ansi();

/* ------------------------------------------------------------------------- */
/* EVENT */
/* ------------------------------------------------------------------------- */

/* Key Event & Resize Event */

typedef enum nt_key_event_type
{ 
    NT_KEY_EVENT_UTF32,
    NT_KEY_EVENT_ESC_KEY
} nt_key_event_type_t;

struct nt_key_event
{
    nt_key_event_type_t type;
    size_t codepoint;
    bool alt; /* Relevant when type == NT_KEY_EVENT_UTF32, otherwise false */
};

struct nt_resize_event
{
    struct nt_xy old_size;
    struct nt_xy new_size;
};

/* Event */

typedef enum nt_event_type
{
    NT_EVENT_TYPE_KEY,
    NT_EVENT_TYPE_RESIZE
} nt_event_type_t;

struct nt_event
{
    nt_event_type_t type;
    union
    {
        struct nt_key_event key_data;
        struct nt_resize_event resize_data;
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
