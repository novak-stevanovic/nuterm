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

struct nt_rgb
{
    uint8_t r, g, b;
};

bool nt_rgb_are_equal(struct nt_rgb rgb1, struct nt_rgb rgb2);

/* ------------------------------------------------------ */

typedef struct nt_color_8
{
    uint8_t _code; // read-only
    bool __default; // private
} nt_color_8;

/* Checks if the code is correct. */
nt_color_8 nt_color_8_new(uint8_t code);

bool nt_color_8_cmp(nt_color_8 c1, nt_color_8 c2);

extern const nt_color_8 NT_COLOR_8_DEFAULT;
extern const nt_color_8 NT_COLOR_8_BLACK;
extern const nt_color_8 NT_COLOR_8_RED;
extern const nt_color_8 NT_COLOR_8_GREEN;
extern const nt_color_8 NT_COLOR_8_YELLOW;
extern const nt_color_8 NT_COLOR_8_BLUE;
extern const nt_color_8 NT_COLOR_8_MAGENTA;
extern const nt_color_8 NT_COLOR_8_CYAN;
extern const nt_color_8 NT_COLOR_8_WHITE;

#define NT_COLOR_8_BLACK_CODE      0
#define NT_COLOR_8_RED_CODE        1
#define NT_COLOR_8_GREEN_CODE      2
#define NT_COLOR_8_YELLOW_CODE     3
#define NT_COLOR_8_BLUE_CODE       4
#define NT_COLOR_8_MAGENTA_CODE    5
#define NT_COLOR_8_CYAN_CODE       6
#define NT_COLOR_8_WHITE_CODE      7

/* ------------------------------------------------------ */

typedef struct nt_color_256
{
    uint8_t _code; // read-only
    bool __default; // private
} nt_color_256;

/* Checks if the code is correct. */
nt_color_256 nt_color_256_new(uint8_t code);

bool nt_color_256_cmp(nt_color_256 c1, nt_color_256 c2);

extern const nt_color_256 NT_COLOR_256_DEFAULT;

/* ------------------------------------------------------ */

typedef struct nt_color_rgb
{
    struct nt_rgb _rgb; // read-only
    bool __default; // private
} nt_color_rgb;

nt_color_rgb nt_color_rgb_new(uint8_t r, uint8_t g, uint8_t b);
nt_color_rgb nt_color_rgb_new_(struct nt_rgb rgb);

bool nt_color_rgb_cmp(nt_color_rgb c1, nt_color_rgb c2);

extern const nt_color_rgb NT_COLOR_RGB_DEFAULT;

/* ------------------------------------------------------ */

nt_color_8 nt_color_8_from_rgb(nt_color_rgb color_rgb);
nt_color_256 nt_color_256_from_rgb(nt_color_rgb color_rgb);

/* -------------------------------------------------------------------------- */
/* STYLE */
/* -------------------------------------------------------------------------- */

typedef uint8_t nt_style;

#define NT_STYLE_DEFAULT        0
#define NT_STYLE_BOLD           (1 << 0)  // 00000001
#define NT_STYLE_FAINT          (1 << 1)  // 00000010
#define NT_STYLE_ITALIC         (1 << 2)  // 00000100
#define NT_STYLE_UNDERLINE      (1 << 3)  // 00001000
#define NT_STYLE_BLINK          (1 << 4)  // 00010000
#define NT_STYLE_REVERSE        (1 << 5)  // 00100000
#define NT_STYLE_HIDDEN         (1 << 6)  // 01000000
#define NT_STYLE_STRIKETHROUGH  (1 << 7)  // 10000000

/* -------------------------------------------------------------------------- */
/* GFX */
/* -------------------------------------------------------------------------- */

struct nt_gfx_8
{
    nt_color_8 bg, fg;
    nt_style style;
};

extern const struct nt_gfx_8 NT_GFX_8_DEFAULT;

bool nt_gfx_8_cmp(struct nt_gfx_8 g1, struct nt_gfx_8 g2);

/* ------------------------------------------------------ */

struct nt_gfx_256
{
    nt_color_256 bg, fg;
    nt_style style;
};

bool nt_gfx_256_cmp(struct nt_gfx_256 g1, struct nt_gfx_256 g2);

extern const struct nt_gfx_256 NT_GFX_256_DEFAULT;

/* ------------------------------------------------------ */

struct nt_gfx_rgb
{
    nt_color_rgb bg, fg;
    nt_style style;
};

extern const struct nt_gfx_rgb NT_GFX_RGB_DEFAULT;

bool nt_gfx_rgb_cmp(struct nt_gfx_rgb g1, struct nt_gfx_rgb g2);

/* ------------------------------------------------------ */

struct nt_gfx
{
    struct nt_gfx_8 gfx_8;
    struct nt_gfx_256 gfx_256;
    struct nt_gfx_rgb gfx_rgb;
};

bool nt_gfx_cmp(struct nt_gfx g1, struct nt_gfx g2);

extern const struct nt_gfx NT_GFX_DEFAULT;

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif // _NT_GFX_H_
