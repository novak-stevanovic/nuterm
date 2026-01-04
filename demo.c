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

void loop_lib()
{
    nt_status _status;
    unsigned int elapsed;
    struct nt_event event;
    bool loop = true;
    while(loop)
    {
        elapsed = nt_event_wait(&event, 1000, &_status);
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
        else if(event.type == NT_EVENT_TIMEOUT)
        {
            printf("T | ");
            fflush(stdout);
        }
        else // other
        {
            printf("C(%d) | ", event.type);
            fflush(stdout);
        }
    }

    printf("Done\n\r");
}

void loop_lib2()
{
    nt_status _status;
    struct nt_event event;
    while(true)
    {
        nt_event_wait(&event, 4000, &_status);
        assert(_status == NT_SUCCESS);

        if(event.type == NT_EVENT_KEY)
        {
            struct nt_key key = *(struct nt_key*)event.data;
            if(nt_key_utf32_check(key, 'q', false)) break;
        }
        else if(event.type >= 100)
        {
            assert(event.data_size = sizeof(size_t));
            printf("%zu ", *(size_t*)event.data);
        }
        else printf("%d", event.type);
    }

    printf("Done\n\r");
}

void* test_thread_fn(void* _)
{
    nt_status _status;
    size_t type = 100;
    long int data = (long int)_;
    while(true)
    {
        sleep(5);
        nt_event_push(type, &data, sizeof(long int), &_status);
        assert(_status == NT_SUCCESS);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    nt_status _status;
    nt_init(&_status);
    assert(_status == NT_SUCCESS);

    pthread_t test_threads[1];
    long int i;
    for(i = 0; i < 1; i++)
    {
        pthread_create(test_threads + i, NULL, test_thread_fn, (void*)i);
    }

    loop_lib();

    nt_deinit();

    return 0;
}
