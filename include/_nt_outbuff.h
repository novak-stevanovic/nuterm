/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef _NT_CHARBUFF_H_
#define _NT_CHARBUFF_H_

#include <stddef.h>

#include "nt_shared.h"

#define NT_OUTBUFF_CAP_DEFAULT 10000

typedef struct nt_outbuff
{
    char* _data;
    size_t _len;
    size_t _cap;
} nt_outbuff_t;

/* Functions below perform no validation of provided arguments. */

// Returns 0 on success, 1 on alloc fail.
int nt_outbuff_init(nt_outbuff_t* buff, size_t cap);

void nt_outbuff_destroy(nt_outbuff_t* buff);

/* If current length + len(`str`) > capacity, a flush will occur and
 * `str` will be stored at the beginning. If there is no capacity
 * for the provided `str`, no buffering will occur - the `str` will
 * be written right away. */
void nt_outbuff_append(nt_outbuff_t* buff, const char* str);

/* Flushes the current buffer. Length is set to 0. */
void nt_outbuff_flush(nt_outbuff_t* buff);

/* Returns 0 on success, 1 on alloc fail.
 * If current length is greater than provided `cap`, a flush will occur. */
int nt_outbuff_set_cap(nt_outbuff_t* buff, size_t cap);

#endif // _NT_CHARBUFF_H_
