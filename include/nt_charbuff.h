/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef _NT_CHARBUFF_H_
#define _NT_CHARBUFF_H_

#include <stddef.h>
#include "nt_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NT_CHARBUFF_CAP_DEFAULT 10000

typedef struct nt_charbuff nt_charbuff_t;

/* If allocation fails or `cap` == 0, returns NULL. */
nt_charbuff_t* nt_charbuff_new(size_t cap);

void nt_charbuff_destroy(nt_charbuff_t* buff);

/* STATUS CODES:
 * 1. NT_SUCCESS,
 * 2. NT_ERR_INVALID_ARG - `buff` is NULL,
 * 3. NT_ERR_OUT_OF_BOUNDS - not enough capacity. */
void nt_charbuff_append(nt_charbuff_t* buff, const char* str, size_t len,
        nt_status_t* out_status);

void nt_charbuff_rewind(nt_charbuff_t* buff, const char** out_str,
        size_t* out_len);

#ifdef __cplusplus
}
#endif

#endif // _NT_CHARBUFF_H_
