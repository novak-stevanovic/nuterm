/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef _NT_EVENT_H_
#define _NT_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "nt_esc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------ */
/* KEY EVENT */
/* ------------------------------------------------------ */

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
            enum nt_esc_key esc_key;
        } esc_key_data;
    };
};

/* ------------------------------------------------------ */
/* RESIZE EVENT */
/* ------------------------------------------------------ */

/* New terminal width and height */
struct nt_resize_event
{
    size_t width, height;
};

/* ------------------------------------------------------ */
/* TIMEOUT EVENT */
/* ------------------------------------------------------ */

struct nt_timeout_event
{
    uint64_t elapsed;
};

/* -------------------------------------------------------------------------- */
/* EVENT */
/* -------------------------------------------------------------------------- */

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

#ifdef __cplusplus
}
#endif

#endif // _NT_EVENT_H_
