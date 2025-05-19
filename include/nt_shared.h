/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef _NT_SHARED_H_
#define _NT_SHARED_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

struct nt_xy { size_t x, y; };
struct nt_dxy { ssize_t x, y; };

typedef int nt_status_t;

#define NT_SUCCESS 0
#define NT_ERR_INIT_PIPE 1
#define NT_ERR_UNEXPECTED 2
#define NT_ERR_FUNC_NOT_SUPPORTED 3
#define NT_ERR_INVALID_ARG 4
#define NT_ERR_ALLOC_FAIL 5
#define NT_ERR_INIT_TERM_ENV 6
#define NT_ERR_TERM_NOT_SUPPORTED 7
#define NT_ERR_INVALID_UTF32 8
#define NT_ERR_BIND_ALREADY_EXISTS 9

#endif // _NT_SHARED_H_
