/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef NT_EVENT_H
#define NT_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nt_shared.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* NT_EVENT */
/* -------------------------------------------------------------------------- */

#define NT_EVENT_INVALID 0
#define NT_EVENT_KEY (1u << 0)
#define NT_EVENT_MOUSE (1u << 1)
#define NT_EVENT_SIGNAL (1u << 2)
#define NT_EVENT_RESIZE (1u << 3)
#define NT_EVENT_TIMEOUT (1u << 4)

// Range [0, 16) is reserved for library events. Range [16, 32) is for
// user-defined events.
#define NT_EVENT_CUSTOM_BASE (1u << 15)

/* Event payload(data field) for built-in events:
 * 1) NT_EVENT_KEY:     struct nt_key_event
 * 2) NT_EVENT_MOUSE:   struct nt_mouse_event
 * 3) NT_EVENT_SIGNAL:  unsigned int (signum)
 * 4) NT_EVENT_RESIZE:  struct nt_resize_event
 * 5) NT_EVENT_TIMEOUT: no payload */

/* -------------------------------------------------------------------------- */

#define NT_EVENT_DATA_MAX_SIZE 64

struct nt_event
{
    union
    {
        char data[NT_EVENT_DATA_MAX_SIZE];
        max_align_t _align;
    };
    uint32_t type; // only 1 bit set
    uint8_t data_size;
};

#define NT_EVENT_FILL_DATA(event, out_ptr) \
    memcpy((out_ptr), event.data, sizeof(*(out_ptr)))

// Returns 0 on success, error code on failure
NT_API int nt_event_new_custom(
        uint32_t type,
        void* data,
        uint8_t data_size,
        struct nt_event* out_event);

NT_API bool nt_event_is_valid(const struct nt_event* event);

/* -------------------------------------------------------------------------- */
/* NT_KEY_EVENT */
/* -------------------------------------------------------------------------- */

enum nt_esc_key
{
    NT_ESC_KEY_F1 = 0,
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

enum nt_key_type
{ 
    NT_KEY_UTF32,
    NT_KEY_ESC
};

struct nt_key_event
{
    enum nt_key_type type;
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

NT_API bool nt_key_event_are_eql(struct nt_key_event key1, struct nt_key_event key2);

// Helper functions
NT_API struct nt_key_event nt_key_event_utf32_new(uint32_t codepoint, bool alt);
NT_API struct nt_key_event nt_key_event_esc_new(enum nt_esc_key esc_key);

NT_API bool nt_key_event_utf32_check_alt(struct nt_key_event key, uint32_t codepoint, bool alt);
NT_API bool nt_key_event_utf32_check(struct nt_key_event key, uint32_t codepoint);

NT_API bool nt_key_event_esc_check(struct nt_key_event key, enum nt_esc_key esc_key);

/* -------------------------------------------------------------------------- */
/* NT_MOUSE_EVENT */
/* -------------------------------------------------------------------------- */

enum nt_mouse_type
{
    NT_MOUSE_CLICK_LEFT = 0,
    NT_MOUSE_CLICK_RIGHT,
    NT_MOUSE_CLICK_MIDDLE,
    NT_MOUSE_SCROLL_UP,
    NT_MOUSE_SCROLL_DOWN
};

struct nt_mouse_event
{
    enum nt_mouse_type type;
    size_t x, y; // Indexing starts at 0
};

/* -------------------------------------------------------------------------- */
/* NT_RESIZE_EVENT */
/* -------------------------------------------------------------------------- */

struct nt_resize_event
{
    size_t new_x, new_y;
};

#ifdef __cplusplus
}
#endif

#endif // NT_EVENT_H
