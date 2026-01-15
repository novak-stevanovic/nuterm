/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef _NT_GFX_H_
#define _NT_GFX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "nt_shared.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* COLOR */
/* -------------------------------------------------------------------------- */

struct nt_rgb
{
    uint8_t r, g, b;
};

/* Clamps the values into range [0, 255] */
NT_API struct nt_rgb nt_rgb_new(int r, int g, int b);

static inline bool nt_rgb_are_equal(struct nt_rgb rgb1, struct nt_rgb rgb2)
{
    return ((rgb1.r == rgb2.r) && (rgb1.g == rgb2.g) && (rgb1.b == rgb2.b));
}

NT_API uint8_t nt_rgb_to_c8(struct nt_rgb rgb);
NT_API uint8_t nt_rgb_to_c256(struct nt_rgb rgb);

/* ------------------------------------------------------ */

#define NT_COLOR_C8_BLACK      0
#define NT_COLOR_C8_RED        1
#define NT_COLOR_C8_GREEN      2
#define NT_COLOR_C8_YELLOW     3
#define NT_COLOR_C8_BLUE       4
#define NT_COLOR_C8_MAGENTA    5
#define NT_COLOR_C8_CYAN       6
#define NT_COLOR_C8_WHITE      7

typedef struct nt_color
{
    /* read-only fields */
    uint8_t _code8;
    uint8_t _code256;
    struct nt_rgb _rgb;
} nt_color;

NT_API extern const nt_color NT_COLOR_DEFAULT;

/* If provided an invalid `code8`, NT_COLOR_DEFAULT will be returned */
NT_API nt_color nt_color_new(uint8_t code8, uint8_t code256, struct nt_rgb rgb);

/* Auto-converts to other colors */
NT_API nt_color nt_color_new_auto(struct nt_rgb rgb);

static inline bool nt_color_are_equal(nt_color color1, nt_color color2)
{
    return ((color1._code8 == color2._code8) &&
        (color1._code256 == color2._code256) &&
        nt_rgb_are_equal(color1._rgb, color2._rgb));
}

/* -------------------------------------------------------------------------- */
/* STYLE */
/* -------------------------------------------------------------------------- */

typedef struct nt_style
{
    /* read-only fields */
    uint8_t _value_c8, _value_c256, _value_rgb;
} nt_style;

NT_API extern const nt_style NT_STYLE_DEFAULT;

NT_API nt_style nt_style_new(uint8_t value8, uint8_t value256, uint8_t value_rgb);
NT_API nt_style nt_style_new_uniform(uint8_t value);

static inline bool nt_style_are_equal(nt_style style1, nt_style style2)
{
    return ((style1._value_c8 == style2._value_c8) &&
            (style1._value_c256 == style2._value_c256) &&
            (style1._value_rgb == style2._value_rgb));
}

#define NT_STYLE_VAL_DEFAULT        0
#define NT_STYLE_VAL_BOLD           (1 << 0)  // 00000001
#define NT_STYLE_VAL_FAINT          (1 << 1)  // 00000010
#define NT_STYLE_VAL_ITALIC         (1 << 2)  // 00000100
#define NT_STYLE_VAL_UNDERLINE      (1 << 3)  // 00001000
#define NT_STYLE_VAL_BLINK          (1 << 4)  // 00010000
#define NT_STYLE_VAL_REVERSE        (1 << 5)  // 00100000
#define NT_STYLE_VAL_HIDDEN         (1 << 6)  // 01000000
#define NT_STYLE_VAL_STRIKETHROUGH  (1 << 7)  // 10000000

/* -------------------------------------------------------------------------- */
/* GFX */
/* -------------------------------------------------------------------------- */

struct nt_gfx
{
    nt_color bg, fg;
    nt_style style;
};

NT_API extern const struct nt_gfx NT_GFX_DEFAULT;

static inline bool nt_gfx_are_equal(struct nt_gfx gfx1, struct nt_gfx gfx2)
{
    return (nt_color_are_equal(gfx1.fg, gfx2.fg) &&
        nt_color_are_equal(gfx1.bg, gfx2.bg) &&
        nt_style_are_equal(gfx1.style, gfx2.style));
}

#ifdef __cplusplus
}
#endif

#endif // _NT_GFX_H_
