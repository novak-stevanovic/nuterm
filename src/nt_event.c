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
