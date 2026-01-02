/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#ifndef __NT_SHARED_H__
#define __NT_SHARED_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/poll.h>

/* Internally used. */
enum nt_esc_fn
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
    NT_ESC_FUNC_COUNT // Must be last because internally used as count
};

#define _nt_return(ret_val, out_status_param, out_status)                      \
    do                                                                         \
    {                                                                          \
        if((out_status_param) != NULL)                                         \
            (*out_status_param) = (out_status);                                \
        return (ret_val);                                                      \
    } while(0);                                                                \

#define _nt_vreturn(out_status_param, out_status)                              \
    do                                                                         \
    {                                                                          \
        if((out_status_param) != NULL)                                         \
            (*out_status_param) = (out_status);                                \
        return;                                                                \
    } while(0);                                                                \

#endif // __NT_SHARED_H__
