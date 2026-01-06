/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef _NT_EVENT_H_
#define _NT_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* NT_EVENT */
/* -------------------------------------------------------------------------- */

#define NT_EVENT_INVALID 0
#define NT_EVENT_KEY (1 << 0)
#define NT_EVENT_MOUSE (1 << 1)
#define NT_EVENT_SIGNAL (1 << 2)
#define NT_EVENT_TIMEOUT (1 << 3)

// Range [0, 16) is reserved for library events. Range [16, 32) is for
// user-defined events.
#define NT_EVENT_CUSTOM_BASE (1 << 15)

/* Event payload(data field) for built-in events:
 * 1) NT_EVENT_KEY:     struct nt_key_event
 * 2) NT_EVENT_MOUSE:   struct nt_mouse_event
 * 3) NT_EVENT_SIGNAL:  unsigned int (signum)
 * 4) NT_EVENT_RESIZE:  struct nt_resize_event
 * 5) NT_EVENT_TIMEOUT: no payload */

/* -------------------------------------------------------------------------- */

#define NT_EVENT_DATA_MAX_SIZE 96

struct nt_event
{
    char data[NT_EVENT_DATA_MAX_SIZE];
    uint32_t type; // only 1 bit set
    uint8_t data_size;
    bool custom; // True if event was pushed via `nt_event_push()`
};

/* -------------------------------------------------------------------------- */
/* NT_KEY_EVENT */
/* -------------------------------------------------------------------------- */

enum nt_esc_key
{
    NT_ESC_KEY_F1,
    NT_ESC_KEY_F2,
    NT_ESC_KEY_F3,
    NT_ESC_KEY_F4,
    NT_ESC_KEY_F5,
    NT_ESC_KEY_F6,
    NT_ESC_KEY_F7,
    NT_ESC_KEY_F8,
    NT_ESC_KEY_F9,
    NT_ESC_KEY_F10,
    NT_ESC_KEY_F11,
    NT_ESC_KEY_F12,
    NT_ESC_KEY_ARR_UP,
    NT_ESC_KEY_ARR_RIGHT,
    NT_ESC_KEY_ARR_DOWN,
    NT_ESC_KEY_ARR_LEFT,
    NT_ESC_KEY_INSERT,
    NT_ESC_KEY_DEL,
    NT_ESC_KEY_HOME,
    NT_ESC_KEY_END,
    NT_ESC_KEY_PG_UP,
    NT_ESC_KEY_PG_DOWN,
    NT_ESC_KEY_STAB,
    NT_ESC_KEY_OTHER // unknown
};

enum nt_key_event_type
{ 
    NT_KEY_EVENT_UTF32,
    NT_KEY_EVENT_ESC
};

struct nt_key_event
{
    enum nt_key_event_type type;
    union
    {
        struct
        {
            uint32_t cp; // codepoint
            bool alt;
        } utf32;

        struct
        {
            enum nt_esc_key val;
        } esc;
    };
};

/* Providing invalid `codepoint` is UB */
struct nt_key_event nt_key_event_utf32_new(uint32_t codepoint, bool alt);
/* Checks if provided `key` matches description */
bool nt_key_event_utf32_check(struct nt_key_event key, uint32_t codepoint, bool alt);
/* Alt insensitive */
bool nt_key_event_utf32_check_(struct nt_key_event key, uint32_t codepoint);

/* Providing invalid `esc_key` is UB */
struct nt_key_event nt_key_event_esc_new(enum nt_esc_key esc_key);
/* Checks if provided `key` matches description */
bool nt_key_event_esc_check(struct nt_key_event key, enum nt_esc_key esc_key);

/* -------------------------------------------------------------------------- */
/* NT_MOUSE_EVENT */
/* -------------------------------------------------------------------------- */

enum nt_mouse_event_type
{
    NT_MOUSE_EVENT_CLICK_LEFT,
    NT_MOUSE_EVENT_CLICK_RIGHT,
    NT_MOUSE_EVENT_CLICK_MIDDLE,
    NT_MOUSE_EVENT_SCROLL_UP,
    NT_MOUSE_EVENT_SCROLL_DOWN
};

struct nt_mouse_event
{
    enum nt_mouse_event_type type;
    size_t x, y;
};

/* -------------------------------------------------------------------------- */
/* NT_RESIZE_EVENT */
/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif // _NT_EVENT_H_
