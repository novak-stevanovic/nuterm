#include "nt.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
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
        if(event.type == NT_EVENT_KEY)
        {
            printf("K(");

            if(event.key_data.type == NT_KEY_EVENT_UTF32)
            {
                if(event.key_data.utf32_data.alt == true)
                    printf("a+");

                printf("%d", event.key_data.utf32_data.codepoint);
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
        else if(event.type == NT_EVENT_RESIZE)
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

    nt_color_rgb fg = nt_color_rgb_new(255, 255, 255);
    nt_color_rgb bg = nt_color_rgb_new(100, 0, 200);

    struct nt_gfx gfx = {
        .gfx_8 = {
            .fg = NT_COLOR_8_RED,
            .bg = NT_COLOR_8_BLACK,
            .style = NT_STYLE_BLINK
        },
        .gfx_256 = {
            .fg = nt_color_256_from_rgb(fg),
            .bg = nt_color_256_from_rgb(bg),
            .style = NT_STYLE_DEFAULT
        },
        .gfx_rgb = {
            .fg = fg,
            .bg = bg,
            .style = NT_STYLE_DEFAULT
        }
    };

    nt_write_str("Novak", gfx, &_status);
    assert(_status == NT_SUCCESS);

    while(getchar() != 'q');

    __nt_deinit__();
    return 0;
}
