#include "nt_gfx.h"
#include "_nt_shared.h"
#include <string.h>
#include <sys/types.h>

static uint8_t _rgb_to_c8(uint8_t r, uint8_t g, uint8_t b);

static uint8_t _rgb_to_c256(uint8_t r, uint8_t g, uint8_t b);

/* -------------------------------------------------------------------------- */

const nt_color NT_COLOR_DEFAULT = {
    ._code8 = 255,
    ._code256 = 255,
    ._rgb = { 0, 0, 0 },
};

const nt_style NT_STYLE_DEFAULT = {0};

const struct nt_gfx NT_GFX_DEFAULT = {
    .bg = NT_COLOR_DEFAULT,
    .fg = NT_COLOR_DEFAULT,
    .style = NT_STYLE_DEFAULT
};

nt_color nt_color_new(uint8_t r, uint8_t g, uint8_t b)
{
    return (nt_color) {
        ._rgb =  { .r = r, .g = g, .b = b },
        ._code256 = _rgb_to_c256(r, g, b),
        ._code8 = _rgb_to_c8(r, g, b)
    };
}

bool nt_color_cmp(nt_color c1, nt_color c2)
{
    return (memcmp(&c1, &c2, sizeof(nt_color)) == 0);
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
const static struct point3d _colors[] = {
    { .x = 0, .y = 0, .z = 0 }, // Black
    { .x = 255, .y = 0, .z = 0 }, // Red
    { .x = 0, .y = 255, .z = 0 }, // Green
    { .x = 255, .y = 255, .z = 0 }, // Yellow
    { .x = 0, .y = 0, .z = 255 }, // Blue
    { .x = 255, .y = 0, .z = 255 }, // Magenta
    { .x = 0, .y = 255, .z = 255 }, // Cyan
    { .x = 255, .y = 255, .z = 255 }, // White
};

static uint8_t _rgb_to_c8(uint8_t r, uint8_t g, uint8_t b)
{
    const struct point3d color = { .x = r, .y = g, .z = b };

    size_t count = sizeof(_colors) / sizeof(struct point3d);
    size_t i;

    ssize_t min_distance = -1;
    ssize_t min_idx = -1;
    size_t it_distance;
    for(i = 0; i < count; i++)
    {
        it_distance = _distance_point3d(_colors[i], color);
        if((it_distance < min_distance) || (min_distance == -1))
        {
            min_distance = it_distance;
            min_idx = i;
        }
    }

    return min_idx;
}

static uint8_t _rgb_to_c256(uint8_t r, uint8_t g, uint8_t b)
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
