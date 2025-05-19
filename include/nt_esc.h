/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef _NT_ESC_H_
#define _NT_ESC_H_

typedef enum nt_esc_key
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
    NT_ESC_KEY_OTHER // Must be last because internally used as count
} nt_esc_key_t;

/* Internally used. */
typedef enum nt_esc_func
{
    NT_ESC_FUNC_CURSOR_SHOW,
    NT_ESC_FUNC_CURSOR_HIDE,
    NT_ESC_FUNC_CURSOR_MOVE, // 2 params
    NT_ESC_FUNC_FG_SET_C8, // 1 param
    NT_ESC_FUNC_FG_SET_C256, // 1 param
    NT_ESC_FUNC_FG_SET_RGB, // 3 params
    NT_ESC_FUNC_FG_SET_DEFAULT,
    NT_ESC_FUNC_BG_SET_C8, // 1 param
    NT_ESC_FUNC_BG_SET_C256, // 1 param
    NT_ESC_FUNC_BG_SET_RGB, // 3 params
    NT_ESC_FUNC_BG_SET_DEFAULT,
    NT_ESC_FUNC_STYLE_SET_BOLD,
    NT_ESC_FUNC_STYLE_SET_FAINT,
    NT_ESC_FUNC_STYLE_SET_ITALIC,
    NT_ESC_FUNC_STYLE_SET_UNDERLINE,
    NT_ESC_FUNC_STYLE_SET_BLINK,
    NT_ESC_FUNC_STYLE_SET_REVERSE,
    NT_ESC_FUNC_STYLE_SET_HIDDEN,
    NT_ESC_FUNC_STYLE_SET_STRIKETHROUGH,
    NT_ESC_FUNC_GFX_RESET, // (!)
    NT_ESC_FUNC_ERASE_SCREEN,
    NT_ESC_FUNC_ERASE_SCROLLBACK, // CHECK IF STANDARD
    NT_ESC_FUNC_ERASE_LINE,
    NT_ESC_FUNC_ALT_BUFF_ENTER,
    NT_ESC_FUNC_ALT_BUFF_EXIT,
    NT_ESC_FUNC_OTHER // Must be last because internally used as count
} nt_esc_func_t;

#endif // _NT_ESC_H_
