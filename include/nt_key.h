#ifndef _NT_KEY_H_
#define _NT_KEY_H_

#include <stdbool.h>
#include <stdint.h>

enum nt_esc_key
{
    NT_ESC_KEY_F1,
    NT_ESC_KEY_F2,
    NT_ESC_KEY_F3,
    NT_ESC_KEY_F4,
    NT_ESC_KEY_F5,
    NT_ESC_KEY_F6,
    NT_ESC_KEY_F7,
    NT_ESC_KEY_F8,
    NT_ESC_KEY_F9,
    NT_ESC_KEY_F10,
    NT_ESC_KEY_F11,
    NT_ESC_KEY_F12,
    NT_ESC_KEY_ARR_UP,
    NT_ESC_KEY_ARR_RIGHT,
    NT_ESC_KEY_ARR_DOWN,
    NT_ESC_KEY_ARR_LEFT,
    NT_ESC_KEY_INSERT,
    NT_ESC_KEY_DEL,
    NT_ESC_KEY_HOME,
    NT_ESC_KEY_END,
    NT_ESC_KEY_PG_UP,
    NT_ESC_KEY_PG_DOWN,
    NT_ESC_KEY_STAB,
    NT_ESC_KEY_COUNT // Must be last because internally used as count
};

typedef enum nt_key_type
{ 
    NT_KEY_UTF32,
    NT_KEY_ESC
} nt_key_type;

struct nt_key
{
    nt_key_type type;
    union
    {
        struct
        {
            uint32_t cp; // codepoint
            bool alt;
        } utf32;

        struct
        {
            enum nt_esc_key val;
        } esc;
    };
};

/* Providing invalid `codepoint` is UB */
struct nt_key nt_key_event_utf32_new(uint32_t codepoint, bool alt);
/* Checks if provided `key` matches description */
bool nt_key_event_utf32_check(struct nt_key key, uint32_t codepoint, bool alt);
/* Alt insensitive */
bool nt_key_event_utf32_check_(struct nt_key key, uint32_t codepoint);

/* Providing invalid `esc_key` is UB */
struct nt_key nt_key_event_esc_key_new(enum nt_esc_key esc_key);
/* Checks if provided `key` matches description */
bool nt_key_event_esc_key_check(struct nt_key key, enum nt_esc_key esc_key);

#endif // _NT_KEY_H_
