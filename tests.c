#include "nuterm.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

int main(int argc, char *argv[])
{
    // unsigned char c;
    // while(true)
    // {
    //     c = getchar();
    //     if(c == 'q') break;
    //     printf("%d ", c);
    // }
    // printf("\n");
    //
    // tcsetattr(STDIN_FILENO, TCSAFLUSH, &init);
    // return 0;
    //
    nt_status_t _status;
    nt_init(NULL);
    while(true)
    {
        struct nt_event event = nt_wait_for_event(0, &_status);
        assert(_status == NT_SUCCESS);
        if(event.type == NT_EVENT_TYPE_KEY)
        {
            printf("K(");

            if(event.key_data.type == NT_KEY_EVENT_ESC_KEY)
                printf("e-");
            else if(event.key_data.alt == true)
                printf("a+");
            printf("cp:%zu)", event.key_data.codepoint);

            printf(" | ");
            fflush(stdout);

            if(event.key_data.codepoint == 'q')
                break;
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
