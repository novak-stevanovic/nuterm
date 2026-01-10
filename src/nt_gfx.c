#include "nt_gfx.h"
#include <string.h>
#include <sys/types.h>

struct point3d
{
    ssize_t x, y, z;
};

static inline ssize_t square_ssize(ssize_t val)
{
    return val * val;
}

static inline size_t distance_point3d(struct point3d p1, struct point3d p2)
{
    return (square_ssize(p2.x - p1.x) +
            square_ssize(p2.y - p1.y) +
            square_ssize(p2.z - p1.z));
}

/* Order is defined by ANSI esc sequence standards. */
const static struct point3d colors[] = {
    { .x = 0, .y = 0, .z = 0 }, // Black
    { .x = 255, .y = 0, .z = 0 }, // Red
    { .x = 0, .y = 255, .z = 0 }, // Green
    { .x = 255, .y = 255, .z = 0 }, // Yellow
    { .x = 0, .y = 0, .z = 255 }, // Blue
    { .x = 255, .y = 0, .z = 255 }, // Magenta
    { .x = 0, .y = 255, .z = 255 }, // Cyan
    { .x = 255, .y = 255, .z = 255 }, // White
};

static inline int nt_clamp_int(int min, int mid, int max)
{
    if(mid < min)
        mid = min;
    else if(mid > max)
        mid = max;

    return mid;
}

/* -------------------------------------------------------------------------- */
/* COLOR */
/* -------------------------------------------------------------------------- */

struct nt_rgb nt_rgb_new(int r, int g, int b)
{
    int r_clamped = nt_clamp_int(0, r, 255);
    int g_clamped = nt_clamp_int(0, g, 255);
    int b_clamped = nt_clamp_int(0, b, 255);

    return (struct nt_rgb) {
        .r = r_clamped,
        .g = g_clamped,
        .b = b_clamped
    };
}

uint8_t nt_rgb_to_c8(struct nt_rgb rgb)
{
    uint8_t r = rgb.r;
    uint8_t g = rgb.g;
    uint8_t b = rgb.b;

    const struct point3d color = { .x = r, .y = g, .z = b };

    size_t count = sizeof(colors) / sizeof(struct point3d);
    size_t i;

    ssize_t min_distance = -1;
    ssize_t min_idx = -1;
    size_t it_distance;
    for(i = 0; i < count; i++)
    {
        it_distance = distance_point3d(colors[i], color);
        if((it_distance < min_distance) || (min_distance == -1))
        {
            min_distance = it_distance;
            min_idx = i;
        }
    }

    return min_idx;
}

uint8_t nt_rgb_to_c256(struct nt_rgb rgb)
{
    uint8_t r = rgb.r;
    uint8_t g = rgb.g;
    uint8_t b = rgb.b;

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

/* -------------------------------------------------------------------------- */

const nt_color NT_COLOR_DEFAULT = {
    ._code8 = 255,
    ._code256 = 255,
    ._rgb = { 0, 0, 0 }
};

nt_color nt_color_new(uint8_t code8, uint8_t code256, struct nt_rgb rgb)
{
    if(code8 > 7)
    {
        return NT_COLOR_DEFAULT;
    }
    else
    {
        return (nt_color) {
            ._code8 = code8,
                ._code256 = code256,
                ._rgb = rgb
        };
    }
}

nt_color nt_color_new_auto(struct nt_rgb rgb)
{
    return (nt_color) {
        ._code8 = nt_rgb_to_c8(rgb),
        ._code256 = nt_rgb_to_c256(rgb),
        ._rgb = rgb
    };
}

/* -------------------------------------------------------------------------- */
/* STYLE */
/* -------------------------------------------------------------------------- */

const nt_style NT_STYLE_DEFAULT = {0};

nt_style nt_style_new(uint8_t value8, uint8_t value256, uint8_t value_rgb)
{
    return (nt_style) {
        ._value_c8 = value8,
        ._value_c256 = value256,
        ._value_rgb = value_rgb
    };
}

nt_style nt_style_new_uniform(uint8_t value)
{
    return nt_style_new(value, value, value);
}

/* -------------------------------------------------------------------------- */
/* GFX */
/* -------------------------------------------------------------------------- */

const struct nt_gfx NT_GFX_DEFAULT = {
    .fg = NT_COLOR_DEFAULT,
    .bg = NT_COLOR_DEFAULT,
    .style = NT_STYLE_DEFAULT
};
