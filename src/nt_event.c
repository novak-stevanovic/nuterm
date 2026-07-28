#include "nt_event.h"
#include "uconv.h"
#include <assert.h>
#include <string.h>

bool nt_key_event_are_eql(struct nt_key_event key1, struct nt_key_event key2)
{
    if((key1.type == NT_KEY_EVENT_UTF32) && (key2.type == NT_KEY_EVENT_UTF32))
        return ((key1.utf32.cp == key2.utf32.cp) && (key1.utf32.alt == key2.utf32.alt));
    else if((key1.type == NT_KEY_EVENT_ESC) && (key2.type == NT_KEY_EVENT_ESC))
        return (key1.esc.val == key2.esc.val);
    else
        return false;
}

bool nt_key_event_utf32_check_alt(struct nt_key_event key, uint32_t codepoint, bool alt)
{
    return ((key.type == NT_KEY_EVENT_UTF32) && (key.utf32.cp == codepoint) &&
            (key.utf32.alt == alt));
}

bool nt_key_event_utf32_check(struct nt_key_event key, uint32_t codepoint)
{
    return ((key.type == NT_KEY_EVENT_UTF32) && (key.utf32.cp == codepoint));
}

bool nt_key_event_esc_check(struct nt_key_event key, enum nt_esc_key esc_key)
{
    return ((key.type == NT_KEY_EVENT_ESC) && (key.esc.val == esc_key));
}

struct nt_event nt_event_new(uint32_t type, void* data, uint8_t data_size)
{
    // Have to check for data size here as well in order not to overflow the data buff
    if(((data_size > 0) && (data == NULL)) || (data_size > NT_EVENT_DATA_MAX_SIZE))
        return (struct nt_event) {0};

    struct nt_event event = {0};
    if(data_size > 0)
        memcpy(event.data, data, data_size);
    event.data_size = data_size;
    event.type = type;

    if(nt_event_is_valid(event)) return event;
    else
        return (struct nt_event) {0};
}

bool nt_event_is_valid(struct nt_event event)
{
    uint32_t type = event.type;
    uint8_t data_size = event.data_size;
    return ((type != NT_EVENT_INVALID) &&
            ((type & (type - 1)) == 0) &&
            (data_size <= NT_EVENT_DATA_MAX_SIZE));
}
