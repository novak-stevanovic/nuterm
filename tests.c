#include "nuterm.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

void loop_basic()
{
    unsigned char c;
    while(true)
    {
        c = getchar();
        if(c == 'q') break;
        printf("%x ", c);
        fflush(stdout);
    }
}

void loop_lib()
{
    nt_status_t _status;
    bool loop = true;
    while(loop)
    {
        struct nt_event event = nt_wait_for_event(-1, &_status);
        assert(_status == NT_SUCCESS);
        if(event.type == NT_EVENT_TYPE_KEY)
        {
            printf("K(");

            if(event.key_data.type == NT_KEY_EVENT_UTF32)
            {
                if(event.key_data.utf32_data.alt == true)
                    printf("a+");

                printf("%ld", event.key_data.utf32_data.codepoint);
                if(event.key_data.utf32_data.codepoint == 'q')
                    loop = false;

            }
            else
            {
                printf("e%d", event.key_data.esc_key_data.esc_key);
            }

            printf(") | ");
            fflush(stdout);
        }
        else if(event.type == NT_EVENT_TYPE_RESIZE)
        {
            printf("R(%ld,%ld)", 
                    event.resize_data.new_size.x, event.resize_data.new_size.y);

            printf(" | ");

            fflush(stdout);
        }
        else
        {
            printf("T | ");
            fflush(stdout);
        }
    }

    printf("Done\n");
}

void test_styles()
{
    size_t i;

    while(1)
    {
        struct nt_gfx gfx1 = {
            .fg = nt_color_new(0, 0, 255),
            .bg = nt_color_new(255, 0, 0),
            .style = NT_STYLE_BOLD
        };
        char c = getchar();
        if(c == 'q') break;

        for(i = 0; i < 8; i++)
        {
            nt_status_t status;
            nt_style_t style;
            nt_write_str("Test1", gfx1, 0, 0, &style, &status);
            // assert(style == gfx1.style);
            assert(status == NT_SUCCESS);
            sleep(2);
            gfx1.style <<= 1;
        }
    }
}

void write_test()
{
    nt_status_t _status;
    struct nt_gfx gfx1 = {
        .bg = nt_color_new(255, 0, 0),
        .fg = nt_color_new(0, 0, 255),
        .style = NT_STYLE_DEFAULT
            // NT_STYLE_FAINT
            // NT_STYLE_ITALIC
            // NT_STYLE_UNDERLINE
            // NT_STYLE_REVERSE | 
            // NT_STYLE_HIDDEN | 
            // NT_STYLE_STRIKETHROUGH
    };

    nt_style_t styles;
    nt_write_str("SAUSAHS", gfx1, NT_WRITE_INPLACE, NT_WRITE_INPLACE, &styles, &_status);

    gfx1.style |= NT_STYLE_FAINT;
    nt_write_str("SAUSAHS", gfx1, NT_WRITE_INPLACE, NT_WRITE_INPLACE, &styles, &_status);

    assert(styles == gfx1.style);
    assert(_status == NT_SUCCESS);

    // nt_write_char(1055, gfx1, 50, 10, &styles, &_status);
    // assert(_status == NT_SUCCESS);

}

void handler1(struct nt_key_event key_event)
{
    printf("EVENT1\n");
}

void handler2(struct nt_key_event key_event)
{
    printf("EVENT2\n");
}

void test_hm()
{
    nt_status_t _status;
    nt_keymap keymap = nt_keymap_new(&_status);

    struct nt_key_event kv1 = {
        .type = NT_KEY_EVENT_UTF32,
        .utf32_data = {
            .codepoint = 'k',
            .alt = false
        }
    };

    nt_keymap_bind(keymap, kv1, handler1, &_status);

    nt_key_handler_t handler1 = nt_keymap_get(keymap, kv1, &_status);
    if(handler1 != NULL)
        handler1(kv1);

    nt_keymap_destroy(keymap);
}

int main(int argc, char *argv[])
{
    nt_status_t _status;
    nt_init(&_status);
    assert(_status == NT_SUCCESS);

    // nt_alt_screen_enable(NULL);

    struct nt_gfx gfx1 = {
        .bg = nt_color_new(255, 0, 128),
        .fg = nt_color_new(255, 255, 255),
        .style = NT_STYLE_BOLD | NT_STYLE_ITALIC
    };

    test_hm();

    // nt_alt_screen_disable(NULL);
    nt_destroy();

    return 0;
}
