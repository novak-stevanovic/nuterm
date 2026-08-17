#include "nt_event.h"
#include "uconv.h"
#include "nt_error.h"
#include <assert.h>
#include <string.h>

bool nt_key_event_are_eql(struct nt_key_event key1, struct nt_key_event key2)
{
    if((key1.type == NT_KEY_UTF32) && (key2.type == NT_KEY_UTF32))
        return ((key1.data.utf32.cp == key2.data.utf32.cp) && (key1.data.utf32.alt == key2.data.utf32.alt));
    else if((key1.type == NT_KEY_ESC) && (key2.type == NT_KEY_ESC))
        return (key1.data.esc.val == key2.data.esc.val);
    else
        return false;
}

struct nt_key_event nt_key_event_utf32_new(uint32_t codepoint, bool alt)
{
    struct nt_key_event event;
    memset(&event, 0, sizeof(event));

    event.type = NT_KEY_UTF32;
    event.data.utf32.cp = codepoint;
    event.data.utf32.alt = alt;

    return event;
}

struct nt_key_event nt_key_event_esc_new(enum nt_esc_key esc_key)
{
    struct nt_key_event event;
    memset(&event, 0, sizeof(event));

    event.type = NT_KEY_ESC;
    event.data.esc.val = esc_key;

    return event;
}

bool nt_key_event_utf32_check_alt(struct nt_key_event key, uint32_t codepoint, bool alt)
{
    return ((key.type == NT_KEY_UTF32) && (key.data.utf32.cp == codepoint) &&
            (key.data.utf32.alt == alt));
}

bool nt_key_event_utf32_check(struct nt_key_event key, uint32_t codepoint)
{
    return ((key.type == NT_KEY_UTF32) && (key.data.utf32.cp == codepoint));
}

bool nt_key_event_esc_check(struct nt_key_event key, enum nt_esc_key esc_key)
{
    return ((key.type == NT_KEY_ESC) && (key.data.esc.val == esc_key));
}

NT_API int nt_event_new_custom(
        uint32_t type,
        void* data,
        uint8_t data_size,
        struct nt_event* out_event)
{
    if(!out_event)
        return NT_ERR_INVALID_ARG;

    memset(out_event, 0, sizeof(*out_event));

    if(((data_size > 0) && !data) || (data_size > NT_EVENT_DATA_MAX_SIZE))
        return NT_ERR_INVALID_ARG;
    if((type == NT_EVENT_INVALID) || ((type & (type - 1)) != 0))
        return NT_ERR_INVALID_ARG;

    if(data_size > 0)
        memcpy(out_event->u.data, data, data_size);
    out_event->data_size = data_size;
    out_event->type = type;

    return 0;
}

bool nt_event_is_valid(const struct nt_event* event)
{
    if(!event) return false;

    uint32_t type = event->type;
    uint8_t data_size = event->data_size;
    return ((type != NT_EVENT_INVALID) &&
            ((type & (type - 1)) == 0) &&
            (data_size <= NT_EVENT_DATA_MAX_SIZE));
}
