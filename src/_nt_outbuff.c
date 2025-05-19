/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "_nt_outbuff.h"
#include "_nt_shared.h"

int nt_outbuff_init(nt_outbuff_t* buff, size_t cap)
{
    buff->_len = 0;

    buff->_data = malloc((sizeof(char) * cap) + 1);
    if(buff->_data == NULL) return 1;

    buff->_data[0] = '\0';
    buff->_cap = cap;

    return 0;
}

void nt_outbuff_destroy(nt_outbuff_t* buff)
{
    if(buff->_data != NULL) free(buff->_data);

    buff->_cap = 0;
    buff->_len = 0;
}

void nt_outbuff_append(nt_outbuff_t* buff, const char* str)
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
        nt_outbuff_flush(buff);
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

void nt_outbuff_flush(nt_outbuff_t* buff)
{
    if(buff->_len > 0)
        nt_awrite(STDOUT_FILENO, buff->_data, buff->_len);

    buff->_len = 0;
}

int nt_outbuff_set_cap(nt_outbuff_t* buff, size_t cap)
{
    if(buff->_cap == cap) return 0;

    if(buff->_len > cap)
        nt_outbuff_flush(buff);

    void* new_data = realloc(buff->_data, cap + 1);
    if(new_data == NULL) return 1;

    buff->_data = new_data;
    buff->_cap = cap;

    return 0;
}
