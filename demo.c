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

    nt_color fg = nt_color_new(
            NT_COLOR_C8_BLACK,
            nt_rgb_to_c256(nt_rgb_new(255, 0, 0)),
            nt_rgb_new(100, 0, 200));

    nt_color bg = nt_color_new(
            NT_COLOR_C8_WHITE,
            nt_rgb_to_c256(nt_rgb_new(0, 255, 0)),
            nt_rgb_new(255, 255, 200));

    nt_style style = nt_style_new(
            NT_STYLE_VAL_BOLD,
            NT_STYLE_VAL_DEFAULT,
            NT_STYLE_VAL_DEFAULT);

    struct nt_gfx gfx = {
        .fg = fg,
        .bg = bg,
        .style = style
    };

    nt_write_str("Novak", gfx, &_status);
    assert(_status == NT_SUCCESS);

    while(getchar() != 'q');

    __nt_deinit__();
    return 0;
}
