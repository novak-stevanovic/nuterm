#include "nt_event.h"
#include "uconv.h"

struct nt_key nt_key_event_utf32_new(uint32_t codepoint, bool alt)
{
    bool in_range = uc_utf32_is_in_range(codepoint, 0);

    if(in_range)
    {
        return (struct nt_key) {
            .type = NT_KEY_UTF32,
            .utf32 = { .cp = codepoint, .alt = alt }
        };
    }
    else
    {
        return (struct nt_key) {
            .type = NT_KEY_UTF32,
            .utf32 = { .cp = UC_UNICODE_MAX, .alt = false }
        };
    }
}

bool nt_key_event_utf32_check(struct nt_key key, uint32_t codepoint, bool alt)
{
    return ((key.type == NT_KEY_UTF32) &&
            (key.utf32.cp == codepoint) &&
            (key.utf32.alt == alt));
}

bool nt_key_event_utf32_check_(struct nt_key key, uint32_t codepoint)
{
    return ((key.type == NT_KEY_UTF32) &&
            (key.utf32.cp == codepoint));
}

struct nt_key nt_key_event_esc_key_new(enum nt_esc_key esc_key)
{
    bool in_range = (esc_key < NT_ESC_KEY_COUNT);

    if(in_range)
    {
        return (struct nt_key) {
            .type = NT_KEY_ESC,
            .esc = { .val = esc_key }
        };
    }
    else
    {
        return (struct nt_key) {
            .type = NT_KEY_ESC,
            .esc = { .val = NT_ESC_KEY_COUNT }
        };
    }
}

bool nt_key_event_esc_key_check(struct nt_key key, enum nt_esc_key esc_key)
{
    return (key.type == NT_KEY_ESC) &&
        (key.esc.val == esc_key);
}
