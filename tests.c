#include "_ansi.h"
#include "nuterm.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>

int main(int argc, char *argv[])
{

    struct termios init, raw;
    cfmakeraw(&raw);
    tcgetattr(STDIN_FILENO, &init);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    unsigned char c;
    while(true)
    {
        c = getchar();
        if(c == 'q') break;
        printf("%d ", c);
    }
    printf("\n");

    return 0;

    nt_status_t _status;
    nt_init(NULL);
    while(true)
    {
        struct nt_event event = nt_wait_for_event(-1, &_status);
        if(event.type == NT_EVENT_TYPE_KEY)
        {
            if(event.key_data.type == NT_KEY_EVENT_UTF32)
            {
                if(event.key_data.codepoint_data.codepoint == 'q') break;
                if(event.key_data.codepoint_data.alt == true) printf("A");
                printf("%d ", event.key_data.codepoint_data.codepoint);
                fflush(stdout);
            }
        }
    }

    printf("Done\n");

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &init);

    return EXIT_SUCCESS;
}
