#include "nt_event.h"
#include "_uconv.h"

struct nt_key_event nt_key_event_utf32_new(uint32_t codepoint, bool alt)
{
    bool in_range = uc_utf32_is_in_range(codepoint, 0);

    if(in_range)
    {
        return (struct nt_key_event) {
            .type = NT_KEY_EVENT_UTF32,
            .utf32_data = { .codepoint = codepoint, .alt = alt }
        };
    }
    else
    {
        return (struct nt_key_event) {
            .type = NT_KEY_EVENT_UTF32,
            .utf32_data = { .codepoint = UC_UNICODE_MAX, .alt = false }
        };
    }
}

bool nt_key_event_utf32_check(struct nt_key_event key, uint32_t codepoint, bool alt)
{
    return ((key.type == NT_KEY_EVENT_UTF32) &&
            (key.utf32_data.codepoint == codepoint) &&
            (key.utf32_data.alt == alt));
}

bool nt_key_event_utf32_check_(struct nt_key_event key, uint32_t codepoint)
{
    return ((key.type == NT_KEY_EVENT_UTF32) &&
            (key.utf32_data.codepoint == codepoint));
}

struct nt_key_event nt_key_event_esc_key_new(enum nt_esc_key esc_key)
{
    bool in_range = (esc_key < NT_ESC_KEY_OTHER);

    if(in_range)
    {
        return (struct nt_key_event) {
            .type = NT_KEY_EVENT_ESC_KEY,
            .esc_key_data = { .esc_key = esc_key }
        };
    }
    else
    {
        return (struct nt_key_event) {
            .type = NT_KEY_EVENT_ESC_KEY,
            .esc_key_data = { .esc_key = NT_ESC_KEY_OTHER }
        };
    }
}

bool nt_key_event_esc_key_check(struct nt_key_event key, enum nt_esc_key esc_key)
{
    return (key.type == NT_KEY_EVENT_ESC_KEY) &&
        (key.esc_key_data.esc_key == esc_key);
}
