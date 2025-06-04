/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nt_charbuff.h"
#include "_nt_shared.h"

struct nt_charbuff
{
    char* _data;
    size_t _len;
    size_t _cap;
};

nt_charbuff_t* nt_charbuff_new(size_t cap)
{
    nt_charbuff_t* buff = (nt_charbuff_t*)malloc(sizeof(struct nt_charbuff));

    buff->_len = 0;

    buff->_data = malloc((sizeof(char) * cap) + 1);
    if(buff->_data == NULL) return NULL;

    buff->_data[0] = '\0';
    buff->_cap = cap;

    return buff;
}

void nt_charbuff_destroy(nt_charbuff_t* buff)
{
    if(buff == NULL) return;

    if(buff->_data != NULL) free(buff->_data);

    free(buff);
}

void nt_charbuff_append(nt_charbuff_t* buff, const char* str)
{
    if(str == NULL) return;

    size_t len = strlen(str);
    if((buff->_len + len) <= buff->_cap) // enough allocated space
    {
        memcpy(buff->_data + buff->_len, str, len);
        buff->_len += len;
    }
    else
    {
        nt_charbuff_flush(buff);
        if(buff->_cap >= len) // enough allocated space for this func
        {
            memcpy(buff->_data, str, len);
            buff->_len = len;
        }
        else
        {
            nt_awrite(STDOUT_FILENO, str, len);
        }
    }

}

void nt_charbuff_flush(nt_charbuff_t* buff)
{
    if(buff->_len > 0)
        nt_awrite(STDOUT_FILENO, buff->_data, buff->_len);

    buff->_len = 0;
}

int nt_charbuff_set_cap(nt_charbuff_t* buff, size_t cap)
{
    if(buff->_cap == cap) return 0;

    if(buff->_len > cap)
        nt_charbuff_flush(buff);

    void* new_data = realloc(buff->_data, cap + 1);
    if(new_data == NULL) return 1;

    buff->_data = new_data;
    buff->_cap = cap;

    return 0;
}
