#include "nt_gfx.h"
#include "_nt_shared.h"
#include <string.h>

const nt_color_t NT_COLOR_DEFAULT = {
    ._code8 = 255,
    ._code256 = 255,
    ._rgb = { 0, 0, 0 },
};

const nt_style_t NT_STYLE_DEFAULT = {0};

const struct nt_gfx NT_GFX_DEFAULT = {
    .bg = NT_COLOR_DEFAULT,
    .fg = NT_COLOR_DEFAULT,
    .style = NT_STYLE_DEFAULT
};

nt_color_t nt_color_new(uint8_t r, uint8_t g, uint8_t b)
{
    return (nt_color_t) {
        ._rgb =  { .r = r, .g = g, .b = b },
        ._code256 = nt_rgb_to_c256(r, g, b),
        ._code8 = nt_rgb_to_c8(r, g, b)
    };
}

bool nt_color_cmp(nt_color_t c1, nt_color_t c2)
{
    return (memcmp(&c1, &c2, sizeof(nt_color_t)) == 0);
}
