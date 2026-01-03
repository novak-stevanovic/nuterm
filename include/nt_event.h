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
#define NT_EVENT_SIGNAL 2
#define NT_EVENT_TIMEOUT 3

// Range [0, 99] is reserved for library events.
#define NT_EVENT_CUSTOM_BASE 100

/* Event payload(data field) for built-in events:
 * 1) NT_EVENT_KEY:     struct nt_key
 * 2) NT_EVENT_SIGNAL:  unsigned int (signum)
 * 3) NT_EVENT_TIMEOUT: no payload */

/* -------------------------------------------------------------------------- */

#define NT_EVENT_DATA_MAX_SIZE 96

struct nt_event
{
    char data[NT_EVENT_DATA_MAX_SIZE];
    uint8_t type;
    uint8_t data_size;
    bool custom; // True if event was pushed with `nt_event_push()`
    uint8_t __padding;
};

#ifdef __cplusplus
}
#endif

#endif // _NT_EVENT_H_
