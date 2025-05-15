#ifndef __NT_SHARED_H__
#define __NT_SHARED_H__

#include "nt_shared.h"
#include <stdint.h>
#include <sys/poll.h>

struct nt_xy nt_get_term_size(nt_status_t* out_status);

int nt_aread(int fd, void* dest, size_t count);
int nt_apoll(struct pollfd pollfds[], size_t count, size_t timeout);
int nt_awrite(int fd, const void* src, size_t count);

uint8_t nt_rgb_to_c8(uint8_t r, uint8_t g, uint8_t b);
uint8_t nt_rgb_to_c256(uint8_t r, uint8_t g, uint8_t b);

#define _RETURN(ret_val, out_status_param, out_status)                         \
    if((out_status_param) != NULL)                                             \
        (*out_status_param) = (out_status);                                    \
    return (ret_val)                                                           \

#define _VRETURN(out_status_param, out_status)                                 \
    if((out_status_param) != NULL)                                             \
        (*out_status_param) = out_status;                                      \
    return                                                                     \

#endif // __NT_SHARED_H__
