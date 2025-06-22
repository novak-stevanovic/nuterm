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
    nt_status _status;
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

    // while(1)
    // {
        struct nt_gfx gfx1 = {
            .fg = nt_color_new(0, 0, 255),
            .bg = nt_color_new(255, 0, 0),
            .style = NT_STYLE_BOLD
        };
        // char c = getchar();
        // if(c == 'q') break;

        for(i = 0; i < 8; i++)
        {
            nt_status status;
            nt_style style;
            nt_write_str_at("Test1", gfx1, 0, 10, &style, &status);
            // assert(style == gfx1.style);
            assert(status == NT_SUCCESS);
            sleep(1);
            gfx1.style <<= 1;
        }
    // }
}

void write_test()
{
    nt_status _status;
    struct nt_gfx gfx1 = {
        .bg = nt_color_new(255, 0, 0),
        .fg = nt_color_new(0, 0, 255),
        .style = NT_STYLE_DEFAULT
    };

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
    nt_status _status;
    __nt_init__(&_status);
    assert(_status == NT_SUCCESS);

    struct nt_gfx complete = {
        .bg = nt_color_new(0, 255, 0),
        .fg = nt_color_new(0, 255, 0),
        .style = 0
    };

    struct nt_gfx uncomplete = {
        .bg = nt_color_new(255, 0, 0),
        .fg = nt_color_new(255, 0, 0),
        .style = 0
    };

    size_t _width, _height;
    nt_get_term_size(&_width, &_height);
    
    nt_buffer_enable(nt_charbuff_new(100000));

    size_t i, j;
    size_t total;
    for(total = 0; total < 10; total++)
    {
        for(i = 0; i < _width; i++)
        {
            nt_write_str_at("", NT_GFX_DEFAULT, 0, _height - 1, NULL, NULL);
            for(j = 0; j <= i; j++)
            {
                nt_write_str(" ", complete, NULL, NULL);
            }

            for(j = i + 1; j < _width; j++)
            {
                nt_write_str(" ", uncomplete, NULL, NULL);
            }

            nt_buffer_flush();
        }
    }

    getchar();

    nt_buffer_disable(NT_BUFF_FLUSH);

    __nt_deinit__();

    return 0;
}
