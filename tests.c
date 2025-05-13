#include "_nt_term.h"
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
    printf("\n");
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

int main(int argc, char *argv[])
{
    nt_status_t _status;
    nt_init(&_status);
    assert(_status == NT_SUCCESS);

    nt_alt_screen_enable(NULL);

    loop_basic();

    nt_alt_screen_disable(NULL);

    nt_destroy();
    return 0;;
}
