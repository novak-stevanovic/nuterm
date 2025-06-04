/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef __NT_SHARED_H__
#define __NT_SHARED_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/poll.h>

int nt_aread(int fd, void* dest, size_t count);
int nt_apoll(struct pollfd pollfds[], size_t count, size_t timeout);
int nt_awrite(int fd, const void* src, size_t count);

#define _return(ret_val, out_status_param, out_status)                         \
    do                                                                         \
    {                                                                          \
        if((out_status_param) != NULL)                                         \
            (*out_status_param) = (out_status);                                \
        return (ret_val);                                                      \
    } while(0);                                                                \

#define _vreturn(out_status_param, out_status)                                 \
    do                                                                         \
    {                                                                          \
        if((out_status_param) != NULL)                                         \
            (*out_status_param) = (out_status);                                \
        return;                                                                \
    } while(0);                                                                \

#endif // __NT_SHARED_H__
