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

#define NT_EVENT_INVALID 0

#define NT_EVENT_KEY 1
// For this event type, `data` buffer will contain: struct nt_key key

#define NT_EVENT_SIGNAL 2
// For this event type, `data` buffer will contain: uint8_t signum

#define NT_EVENT_TIMEOUT 3
// For this event type, `data` will be empty

// Range [0, 99] is reserved.
#define NT_EVENT_CUSTOM_BASE 100

/* -------------------------------------------------------------------------- */

#define NT_EVENT_DATA_SIZE 63

struct nt_event
{
    char data[NT_EVENT_DATA_SIZE];
    uint8_t type;
};

#ifdef __cplusplus
}
#endif

#endif // _NT_EVENT_H_
