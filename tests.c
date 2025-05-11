#include "_nt_term.h"
#include "nuterm.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

int main(int argc, char *argv[])
{
    // nt_init(NULL);
    // unsigned char c;
    // while(true)
    // {
    //     c = getchar();
    //     if(c == 'q') break;
    //     printf("%x ", c);
    // }
    // printf("\n");
    //
    // nt_destroy();
    // return 0;

    nt_status_t _status;
    nt_init(NULL);
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
            printf("R(%ld,%ld->%ld,%ld)", 
                    event.resize_data.old_size.x, event.resize_data.old_size.y,
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

    nt_destroy();

    return 0;;
}
