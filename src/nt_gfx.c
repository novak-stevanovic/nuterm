#include "nt_gfx.h"
#include <string.h>
#include <sys/types.h>

static uint8_t _rgb_to_c8(uint8_t r, uint8_t g, uint8_t b);
static uint8_t _rgb_to_c256(uint8_t r, uint8_t g, uint8_t b);

static inline uint8_t _nt_clamp_uint8(uint8_t min, uint8_t mid, uint8_t max)
{
    if(mid < min)
        mid = min;
    else if(mid > max)
        mid = max;

    return mid;
}

bool nt_rgb_are_equal(struct nt_rgb rgb1, struct nt_rgb rgb2)
{
    return (memcmp(&rgb1, &rgb2, sizeof(struct nt_rgb)) == 0);
}

/* ------------------------------------------------------ */

nt_color_8 nt_color_8_new(uint8_t code)
{
    return (nt_color_8) {
        ._code = _nt_clamp_uint8(0, code, 7),
        .__default = false
    };
}

bool nt_color_8_cmp(nt_color_8 c1, nt_color_8 c2)
{
    return (memcmp(&c1, &c2, sizeof(nt_color_8)) == 0);
}

const nt_color_8 NT_COLOR_8_DEFAULT = {
    ._code = 0,
    .__default = true
};

const nt_color_8 NT_COLOR_8_BLACK = {
    ._code = NT_COLOR_8_BLACK_CODE,
    .__default = false
};

const nt_color_8 NT_COLOR_8_RED = {
    ._code = NT_COLOR_8_RED_CODE,
    .__default = false
};

const nt_color_8 NT_COLOR_8_GREEN = {
    ._code = NT_COLOR_8_GREEN_CODE,
    .__default = false
};

const nt_color_8 NT_COLOR_8_YELLOW = {
    ._code = NT_COLOR_8_YELLOW_CODE,
    .__default = false
};

const nt_color_8 NT_COLOR_8_BLUE = {
    ._code = NT_COLOR_8_BLUE_CODE,
    .__default = false
};

const nt_color_8 NT_COLOR_8_MAGENTA = {
    ._code = NT_COLOR_8_MAGENTA_CODE,
    .__default = false
};

const nt_color_8 NT_COLOR_8_CYAN = {
    ._code = NT_COLOR_8_CYAN_CODE,
    .__default = false
};

const nt_color_8 NT_COLOR_8_WHITE = {
    ._code = NT_COLOR_8_WHITE_CODE,
    .__default = false
};

/* ------------------------------------------------------ */

nt_color_256 nt_color_256_new(uint8_t code)
{
    return (nt_color_256) {
        ._code = code,
        .__default = false
    };
}

bool nt_color_256_cmp(nt_color_256 c1, nt_color_256 c2)
{
    return (memcmp(&c1, &c2, sizeof(nt_color_256)) == 0);
}

const nt_color_256 NT_COLOR_256_DEFAULT = {
    ._code = 0,
    .__default = true
};

/* ------------------------------------------------------ */

nt_color_rgb nt_color_rgb_new_(struct nt_rgb rgb)
{
    return (nt_color_rgb) {
        ._rgb = rgb,
        .__default = false
    };
}

nt_color_rgb nt_color_rgb_new(uint8_t r, uint8_t g, uint8_t b)
{
    return (nt_color_rgb) {
        ._rgb = { .r = r, .g = g, .b = b },
        .__default = false
    };
}

bool nt_color_rgb_cmp(nt_color_rgb c1, nt_color_rgb c2)
{
    return (memcmp(&c1, &c2, sizeof(nt_color_rgb)) == 0);
}

const nt_color_rgb NT_COLOR_RGB_DEFAULT = {
    ._rgb = (struct nt_rgb) {0},
    .__default = true
};

/* ------------------------------------------------------ */

nt_color_8 nt_color_8_from_rgb(nt_color_rgb color_rgb)
{
    if(color_rgb.__default)
    {
        return NT_COLOR_8_DEFAULT;
    }
    else
    {
        return nt_color_8_new(_rgb_to_c8(
                    color_rgb._rgb.r,
                    color_rgb._rgb.g,
                    color_rgb._rgb.b));
    }
}

nt_color_256 nt_color_256_from_rgb(nt_color_rgb color_rgb)
{
    if(color_rgb.__default)
    {
        return NT_COLOR_256_DEFAULT;
    }
    else
    {
        return nt_color_256_new(_rgb_to_c256(
                    color_rgb._rgb.r,
                    color_rgb._rgb.g,
                    color_rgb._rgb.b));
    }
}

/* -------------------------------------------------------------------------- */
/* GFX */
/* -------------------------------------------------------------------------- */

const struct nt_gfx_8 NT_GFX_8_DEFAULT = {
    .bg = NT_COLOR_8_DEFAULT,
    .fg = NT_COLOR_8_DEFAULT,
    .style = NT_STYLE_DEFAULT
};

bool nt_gfx_8_cmp(struct nt_gfx_8 g1, struct nt_gfx_8 g2)
{
    return (memcmp(&g1, &g2, sizeof(struct nt_gfx_8)) == 0);
}

/* ------------------------------------------------------ */

const struct nt_gfx_256 NT_GFX_256_DEFAULT = {
    .bg = NT_COLOR_256_DEFAULT,
    .fg = NT_COLOR_256_DEFAULT,
    .style = NT_STYLE_DEFAULT
};

bool nt_gfx_256_cmp(struct nt_gfx_256 g1, struct nt_gfx_256 g2)
{
    return (memcmp(&g1, &g2, sizeof(struct nt_gfx_256)) == 0);
}

/* ------------------------------------------------------ */

const struct nt_gfx_rgb NT_GFX_RGB_DEFAULT = {
    .bg = NT_COLOR_RGB_DEFAULT,
    .fg = NT_COLOR_RGB_DEFAULT,
    .style = NT_STYLE_DEFAULT
};

bool nt_gfx_rgb_cmp(struct nt_gfx_rgb g1, struct nt_gfx_rgb g2)
{
    return (memcmp(&g1, &g2, sizeof(struct nt_gfx_rgb)) == 0);
}

/* ------------------------------------------------------ */

bool nt_gfx_cmp(struct nt_gfx g1, struct nt_gfx g2)
{
    return (memcmp(&g1, &g2, sizeof(struct nt_gfx)) == 0);
}

const struct nt_gfx NT_GFX_DEFAULT = {
    .gfx_8 = NT_GFX_8_DEFAULT,
    .gfx_256 = NT_GFX_256_DEFAULT,
    .gfx_rgb = NT_GFX_RGB_DEFAULT
};

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
