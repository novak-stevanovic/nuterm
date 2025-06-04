/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef _NT_GFX_H_
#define _NT_GFX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* COLOR */
/* -------------------------------------------------------------------------- */

/* Colors are meant to be created by invoking nt_color_new(). Provided RGB
 * values are translated into color codes(0-7 & 0-255). When writing text to
 * terminal, terminal color will be set based on supported color palette. */

typedef struct nt_color
{
    uint8_t _code8;
    uint8_t _code256;
    struct 
    {
        uint8_t r, g, b;
    } _rgb;
} nt_color_t;

extern const nt_color_t NT_COLOR_DEFAULT;

nt_color_t nt_color_new(uint8_t r, uint8_t g, uint8_t b);
bool nt_color_cmp(nt_color_t c1, nt_color_t c2);

/* -------------------------------------------------------------------------- */
/* STYLE */
/* -------------------------------------------------------------------------- */

/* Bitmask type where each bit represents a style. */

typedef uint8_t nt_style_t;

extern const nt_style_t NT_STYLE_DEFAULT;

#define NT_STYLE_NONE         0
#define NT_STYLE_BOLD            (1 << 0)  // 00000001
#define NT_STYLE_FAINT           (1 << 1)  // 00000010
#define NT_STYLE_ITALIC          (1 << 2)  // 00000100
#define NT_STYLE_UNDERLINE       (1 << 3)  // 00001000
#define NT_STYLE_BLINK           (1 << 4)  // 00010000
#define NT_STYLE_REVERSE         (1 << 5)  // 00100000
#define NT_STYLE_HIDDEN          (1 << 6)  // 01000000
#define NT_STYLE_STRIKETHROUGH   (1 << 7)  // 10000000

/* -------------------------------------------------------------------------- */
/* GFX */
/* -------------------------------------------------------------------------- */

struct nt_gfx
{
    nt_color_t fg, bg;
    nt_style_t style;
};

extern const struct nt_gfx NT_GFX_DEFAULT;

#ifdef __cplusplus
}
#endif

#endif // _NT_GFX_H_
