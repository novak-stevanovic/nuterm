/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef _NT_CHARBUFF_H_
#define _NT_CHARBUFF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define NT_CHARBUFF_CAP_DEFAULT 10000

typedef struct nt_charbuff nt_charbuff_t;

/* Functions below perform no validation of provided arguments. */

// Returns 0 on success, 1 on alloc fail.
nt_charbuff_t* nt_charbuff_new(size_t cap);

void nt_charbuff_destroy(nt_charbuff_t* buff);

/* If current length + len(`str`) > capacity, a flush will occur and
 * `str` will be stored at the beginning. If there is no capacity
 * for the provided `str`, no buffering will occur - the `str` will
 * be written right away. */
void nt_charbuff_append(nt_charbuff_t* buff, const char* str);

/* Flushes the current buffer. Length is set to 0. */
void nt_charbuff_flush(nt_charbuff_t* buff);

/* Returns 0 on success, 1 on alloc fail.
 * If current length is greater than provided `cap`, a flush will occur. */
int nt_charbuff_set_cap(nt_charbuff_t* buff, size_t cap);

#ifdef __cplusplus
}
#endif

#endif // _NT_CHARBUFF_H_
