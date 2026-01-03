#include "nt.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
    unsigned int elapsed;
    struct nt_event event;
    bool loop = true;
    while(loop)
    {
        elapsed = nt_wait_for_event(&event, LOOP_TIMEOUT, &_status);
        printf("(e:%d)", elapsed);

        assert(_status == NT_SUCCESS);
        if(event.type == NT_EVENT_KEY)
        {
            printf("K(");

            struct nt_key key = *(struct nt_key*)event.data;

            if(key.type == NT_KEY_UTF32)
            {
                if(key.utf32.alt == true)
                    printf("a+");

                printf("%d", key.utf32.cp);
                if(key.utf32.cp == 'q')
                    loop = false;

            }
            else
            {
                printf("e%d", key.esc.val);
            }

            printf(") | ");
            fflush(stdout);
        }
        else if(event.type == NT_EVENT_SIGNAL)
        {
            uint8_t signum = *(uint8_t*)event.data;

            printf("S(%d)", signum);

            printf(" | ");

            fflush(stdout);
        }
        else
        {
            printf("T | ");
            fflush(stdout);
        }
    }

    printf("Done\n\r");
}

void* test_thread_fn(void* _)
{
    while(true)
    {
        usleep(1000);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    nt_status _status;
    nuterm_init(&_status);
    assert(_status == NT_SUCCESS);

    pthread_t test_thread;
    pthread_create(&test_thread, NULL, test_thread_fn, NULL);

    struct nt_gfx gfx = {
        .bg = nt_color_new_auto(nt_rgb_new(255, 0, 0)),
        .fg = nt_color_new_auto(nt_rgb_new(0, 255, 0)),
        .style = NT_STYLE_DEFAULT

    };

    // const char* str = "Novak";

    // nt_write_str(str, strlen(str), gfx, NULL);
    // nt_write_str("\n", 1, gfx, NULL);

    loop_lib();

    nuterm_deinit();

    return 0;
}
