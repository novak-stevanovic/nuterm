#include "nt_key.h"
#include "uconv.h"

struct nt_key nt_key_utf32_new(uint32_t codepoint, bool alt)
{
    if(codepoint > UC_UNICODE_MAX) codepoint = 0;

    return (struct nt_key) {
        .type = NT_KEY_UTF32,
        .utf32 = { .cp = codepoint, .alt = alt }
    };
}

bool nt_key_utf32_check(struct nt_key key, uint32_t codepoint, bool alt)
{
    return ((key.type == NT_KEY_UTF32) && (key.utf32.cp == codepoint) &&
            (key.utf32.alt == alt));
}

bool nt_key_utf32_check_(struct nt_key key, uint32_t codepoint)
{
    return ((key.type == NT_KEY_UTF32) && (key.utf32.cp == codepoint));
}

struct nt_key nt_key_esc_new(enum nt_esc_key esc_key)
{
    if(esc_key >= NT_ESC_KEY_COUNT)
        esc_key = NT_ESC_KEY_F1;

    return (struct nt_key) {
        .type = NT_KEY_ESC,
        .esc = { .val = esc_key }
    };
}

bool nt_key_esc_check(struct nt_key key, enum nt_esc_key esc_key)
{
    return ((key.type == NT_KEY_ESC) && (key.esc.val == esc_key));
}
