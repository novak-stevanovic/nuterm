/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef NT_GFX_H
#define NT_GFX_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#error "C99 or newer is required"
#endif /* C99 check */

#include "nt_shared.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* TYPES */
/* -------------------------------------------------------------------------- */

enum nt_color_c8
{
    NT_COLOR_C8_BLACK = 0,
    NT_COLOR_C8_RED,
    NT_COLOR_C8_GREEN,
    NT_COLOR_C8_YELLOW,
    NT_COLOR_C8_BLUE,
    NT_COLOR_C8_MAGENTA,
    NT_COLOR_C8_CYAN,
    NT_COLOR_C8_WHITE
};

struct nt_rgb
{
    uint8_t r, g, b;
};

struct nt_color
{
    enum nt_color_c8 code8;
    uint8_t code256;
    struct nt_rgb rgb;
};

struct nt_style
{
    uint8_t value_c8, value_c256, value_rgb;
};

struct nt_gfx
{
    struct nt_color bg, fg;
    struct nt_style style;
};

/* -------------------------------------------------------------------------- */
/* COLOR */
/* -------------------------------------------------------------------------- */

/* Clamps the values into range [0, 255] */
NT_API struct nt_rgb nt_rgb_clamp(int r, int g, int b);

NT_API enum nt_color_c8 nt_rgb_to_c8(uint8_t r, uint8_t g, uint8_t b);
NT_API uint8_t nt_rgb_to_c256(uint8_t r, uint8_t g, uint8_t b);

NT_API enum nt_color_c8 nt_rgb_to_c8_rgb(struct nt_rgb rgb);
NT_API uint8_t nt_rgb_to_c256_rgb(struct nt_rgb rgb);

static inline bool nt_rgb_are_eql(struct nt_rgb rgb1, struct nt_rgb rgb2)
{
    return ((rgb1.r == rgb2.r) && (rgb1.g == rgb2.g) && (rgb1.b == rgb2.b));
}

/* ------------------------------------------------------ */

NT_API extern const struct nt_color NT_COLOR_DEFAULT;

/* Auto-converts to other colors */
NT_API struct nt_color nt_color_new_auto(uint8_t r, uint8_t g, uint8_t b);
NT_API struct nt_color nt_color_new_auto_rgb(struct nt_rgb rgb);

static inline bool 
nt_color_are_eql(struct nt_color color1, struct nt_color color2)
{
    return ((color1.code8 == color2.code8) &&
            (color1.code256 == color2.code256) &&
            nt_rgb_are_eql(color1.rgb, color2.rgb));
}

/* -------------------------------------------------------------------------- */
/* STYLE */
/* -------------------------------------------------------------------------- */

NT_API extern const struct nt_style NT_STYLE_DEFAULT;

NT_API struct nt_style nt_style_new_uniform(uint8_t value);

static inline bool 
nt_style_are_eql(struct nt_style style1, struct nt_style style2)
{
    return ((style1.value_c8 == style2.value_c8) &&
            (style1.value_c256 == style2.value_c256) &&
            (style1.value_rgb == style2.value_rgb));
}

#define NT_STYLE_BOLD           (1u << 0)  // 00000001
#define NT_STYLE_FAINT          (1u << 1)  // 00000010
#define NT_STYLE_ITALIC         (1u << 2)  // 00000100
#define NT_STYLE_UNDERLINE      (1u << 3)  // 00001000
#define NT_STYLE_BLINK          (1u << 4)  // 00010000
#define NT_STYLE_REVERSE        (1u << 5)  // 00100000
#define NT_STYLE_HIDDEN         (1u << 6)  // 01000000
#define NT_STYLE_STRIKETHROUGH  (1u << 7)  // 10000000

/* -------------------------------------------------------------------------- */
/* GFX */
/* -------------------------------------------------------------------------- */

NT_API extern const struct nt_gfx NT_GFX_DEFAULT;

static inline bool 
nt_gfx_are_eql(struct nt_gfx gfx1, struct nt_gfx gfx2)
{
    return (nt_color_are_eql(gfx1.fg, gfx2.fg) &&
        nt_color_are_eql(gfx1.bg, gfx2.bg) &&
        nt_style_are_eql(gfx1.style, gfx2.style));
}

static inline struct nt_gfx
nt_gfx_style_reverse(struct nt_gfx gfx)
{
    gfx.style.value_c8 |= NT_STYLE_REVERSE;
    gfx.style.value_c256 |= NT_STYLE_REVERSE;
    gfx.style.value_rgb |= NT_STYLE_REVERSE;

    return gfx;
}

#endif // NT_GFX_H
