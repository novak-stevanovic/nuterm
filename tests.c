#include "nt.h"
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

#define LOOP_TIMEOUT 5 * 1000

void loop_lib()
{
    nt_status_t _status;
    bool loop = true;
    while(loop)
    {
        struct nt_event event = nt_wait_for_event(LOOP_TIMEOUT, &_status);
        printf("(e:%d)", event.elapsed);

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
                    event.resize_data.width, event.resize_data.height);

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
            nt_write_str_at("Test1", gfx1, 0, 10, &style, &status);
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

    size_t _width = 5, _height = 5;
    // nt_get_term_size(&_width, &_height);
    size_t i, j;

    for(i = 0; i < _height; i++)
    {
        for(j = 0; j < _width; j++)
        {
            nt_write_char_at(70, gfx1, j, i, NULL, &_status);
            assert(_status == 0);
        }
    }
}

void handler1(struct nt_key_event key_event, void* data)
{
    printf("EVENT1\n");
}

void handler2(struct nt_key_event key_event, void* data)
{
    printf("EVENT2\n");
}

int main(int argc, char *argv[])
{
    nt_status_t _status;
    __nt_init__(&_status);
    assert(_status == NT_SUCCESS);

    nt_alt_screen_enable(NULL);

    // struct nt_gfx gfx1 = {
    //     .bg = nt_color_new(255, 0, 128),
    //     .fg = nt_color_new(255, 255, 255),
    //     .style = NT_STYLE_BOLD | NT_STYLE_ITALIC
    // };

    // write_test();
    //
    loop_lib();

    nt_alt_screen_disable(NULL);
    __nt_deinit__();

    return 0;
}
