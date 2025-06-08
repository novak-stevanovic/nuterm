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
    nt_charbuff_t* new = (nt_charbuff_t*)malloc(sizeof(struct nt_charbuff));
    if(new == NULL) return NULL;

    new->_data = malloc(cap);
    if(new->_data == NULL)
    {
        free(new);
        return NULL;
    }

    new->_cap = cap;
    new->_len = 0;

    return new;
}

void nt_charbuff_destroy(nt_charbuff_t* buff)
{
    if(buff == NULL) return;

    free(buff->_data);

    free(buff);
}

void nt_charbuff_append(nt_charbuff_t* buff, const char* str, size_t len,
        nt_status_t* out_status)
{
    if(buff == NULL)
        _vreturn(out_status, NT_ERR_INVALID_ARG);

    if((buff->_len + len) > buff->_cap)
        _vreturn(out_status, NT_ERR_OUT_OF_BOUNDS);

    memcpy((buff->_data + buff->_len), str, len);
    buff->_len += len;

    _vreturn(out_status, NT_SUCCESS);
}

void nt_charbuff_rewind(nt_charbuff_t* buff, const char** out_str,
        size_t* out_len)
{
    if(buff == NULL)
    {
        if(out_str != NULL) *out_str = NULL;
        if(out_len != NULL) *out_len = 0;

        return;
    }

    if(out_str != NULL) *out_str = buff->_data;
    if(out_len != NULL) *out_len = buff->_len;

    buff->_len = 0;
}
