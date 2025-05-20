/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "_nt_shared.h"
#include "nt_shared.h"

struct nt_xy nt_get_term_size(nt_status_t* out_status)
{
    struct winsize size;
    int status = ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
    if(status == -1)
    {
        _return((struct nt_xy) {0}, out_status, NT_ERR_FUNC_NOT_SUPPORTED);
    }

    struct nt_xy ret;
    ret.x = size.ws_col;
    ret.y = size.ws_row;

    _return(ret, out_status, NT_SUCCESS);
}

int nt_aread(int fd, void* dest, size_t count)
{
    int status;
    do
    {
        status = read(fd, dest, count);
    } while((status == -1) && (errno == EINTR));

    return status;
}

int nt_apoll(struct pollfd pollfds[], size_t count, size_t timeout)
{
    int status;
    do
    {
        status = poll(pollfds, count, timeout);
    } while((status == -1) && (errno == EINTR));

    return status;
}

int nt_awrite(int fd, const void* src, size_t count)
{
    int status;
    do
    {
        status = write(fd, src, count);
    } while((status == -1) && (errno == EINTR));

    return status;
}

/* -------------------------------------------------------------------------- */

struct point3d
{
    ssize_t x, y, z;
};

static inline ssize_t _square_ssize(ssize_t val)
{
    return val * val;
}

static inline size_t _distance_point3d(struct point3d p1, struct point3d p2)
{
    return (_square_ssize(p2.x - p1.x) +
            _square_ssize(p2.y - p1.y) +
            _square_ssize(p2.z - p1.z));
}

/* Order is defined by ANSI esc sequence standards. */
const struct point3d colors[] = {
    { .x = 0, .y = 0, .z = 0 }, // Black
    { .x = 255, .y = 0, .z = 0 }, // Red
    { .x = 0, .y = 255, .z = 0 }, // Green
    { .x = 255, .y = 255, .z = 0 }, // Yellow
    { .x = 0, .y = 0, .z = 255 }, // Blue
    { .x = 255, .y = 0, .z = 255 }, // Magenta
    { .x = 0, .y = 255, .z = 255 }, // Cyan
    { .x = 255, .y = 255, .z = 255 }, // White
};

uint8_t nt_rgb_to_c8(uint8_t r, uint8_t g, uint8_t b)
{
    const struct point3d color = { .x = r, .y = g, .z = b };

    size_t count = sizeof(colors) / sizeof(struct point3d);
    size_t i;

    ssize_t min_distance = -1;
    ssize_t min_idx = -1;
    size_t it_distance;
    for(i = 0; i < count; i++)
    {
        it_distance = _distance_point3d(colors[i], color);
        if((it_distance < min_distance) || (min_distance == -1))
        {
            min_distance = it_distance;
            min_idx = i;
        }
    }

    return min_idx;
}

uint8_t nt_rgb_to_c256(uint8_t r, uint8_t g, uint8_t b)
{
    if((r == g) && (g == b)) // gray
    {
        // 26 is derived from 24 gray colors + black + white
        uint8_t val_adj = (uint8_t)(r / (256.0 / 26));

        if(val_adj == 0) return 16; // black
        if(val_adj == 25) return 15; // white

        else return (232 + val_adj);
    }
    else // not gray
    {
        uint8_t r_adj = (uint8_t)(r / (256.0 / 6));
        uint8_t g_adj = (uint8_t)(g / (256.0 / 6));
        uint8_t b_adj = (uint8_t)(b / (256.0 / 6));

        return (16 + (36 * r_adj) + (6 * g_adj) + (1 * b_adj));
    }

    return 0;
}
