#include "nt.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ENABLE_PRINT 1

void loop_basic()
{
    unsigned char c;
    while(true)
    {
        c = getchar();
        if(c == 'q') break;
        if(ENABLE_PRINT) printf("%x ", c);
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
        elapsed = nt_event_wait(&event, 10000, &_status);
        // if(ENABLE_PRINT) printf("(e:%d)", elapsed);

        assert(_status == NT_SUCCESS);
        if(event.type == NT_EVENT_KEY)
        {
            if(ENABLE_PRINT) printf("K(");

            struct nt_key_event key = *(struct nt_key_event*)event.data;

            if(key.type == NT_KEY_EVENT_UTF32)
            {
                if(key.utf32.alt == true)
                    if(ENABLE_PRINT) printf("a+");

                if(ENABLE_PRINT) printf("%d", key.utf32.cp);
                if(key.utf32.cp == 'q')
                    loop = false;

            }
            else
            {
                if(ENABLE_PRINT) printf("e%d", key.esc.val);
            }

            if(ENABLE_PRINT) printf(") | ");
            fflush(stdout);
        }
        else if(event.type == NT_EVENT_MOUSE)
        {
            struct nt_mouse_event mouse_event = *(struct nt_mouse_event*)event.data;
            if(ENABLE_PRINT) printf("M(%d, %ld, %ld))", mouse_event.type,
                    mouse_event.x, mouse_event.y);

            if(ENABLE_PRINT) printf(" | ");

            fflush(stdout);
        }
        else if(event.type == NT_EVENT_SIGNAL)
        {
            uint8_t signum = *(uint8_t*)event.data;

            if(ENABLE_PRINT) printf("S(%d)", signum);

            if(ENABLE_PRINT) printf(" | ");

            fflush(stdout);
        }
        else if(event.type == NT_EVENT_RESIZE)
        {
            if(ENABLE_PRINT) printf("R | ");
            fflush(stdout);
        }
        else if(event.type == NT_EVENT_TIMEOUT)
        {
            if(ENABLE_PRINT) printf("T | ");
            fflush(stdout);
        }
        else // other
        {
            if(ENABLE_PRINT) printf("C(%d, %ld) | ", event.type, *(long int*)event.data);
            fflush(stdout);
        }
    }

    if(ENABLE_PRINT) printf("Done\n\r");
}

void loop_lib_byte()
{
    char c;
    while(true)
    {
        c = getchar();
        if(c == 'q') break;
        else
        {
            printf("%d ", c);
            fflush(stdout);
        }
    }
}

void* test_thread_fn(void* _)
{
    nt_status _status;
    uint32_t type = (NT_EVENT_CUSTOM_BASE << 1);
    long int data = (long int)_;
    struct nt_event event;
    while(true)
    {
        sleep(5);
        event = nt_event_new(type, &data, sizeof(long int));
        assert(nt_event_is_valid(event));
        nt_event_push(event, &_status);
        assert(_status == NT_SUCCESS);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    nt_status _status;
    nt_init(&_status);

    // pthread_t test_threads[1];
    // long int i;
    // for(i = 0; i < 1; i++)
    // {
    //     pthread_create(test_threads + i, NULL, test_thread_fn, (void*)69);
    // }
    //
    // loop_lib();
    //
    // void* buff = malloc(100000);

    // nt_buffer_enable(buff, 1000, &_status);
    // assert(_status == NT_SUCCESS);

    // const char* str = "Novak";

    // size_t i;
    // for(i = 0; i < 10; i++)
    // {
    //     nt_buffer_flush();
    //     nt_write_str(str, strlen(str), NT_GFX_DEFAULT, &_status);
    //     assert(_status == NT_SUCCESS);
    // }
    nt_mouse_mode_enable(&_status);
    loop_lib();
    nt_mouse_mode_disable(&_status);
    // nt_buffer_disable(NT_BUFF_FLUSH);

    nt_deinit();

    return 0;
}
