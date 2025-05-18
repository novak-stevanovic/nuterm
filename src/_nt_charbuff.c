#include <stdlib.h>
#include <string.h>

#include "_nt_charbuff.h"
#include "_nt_shared.h"

#define CHARBUFF_INIT_CAP 100

void nt_charbuff_init(nt_charbuff_t* buff, nt_status_t* out_status)
{
    if(buff == NULL)
    {
        _VRETURN(out_status, NT_ERR_INVALID_ARG);
    }

    buff->data = malloc(CHARBUFF_INIT_CAP + 1);
    if(buff->data == NULL)
    {
        buff->len = 0;
        buff->capacity = 0;
        _VRETURN(out_status, NT_ERR_ALLOC_FAIL);
    }

    buff->len = 0;
    buff->data[0] = '\0';
    buff->capacity = CHARBUFF_INIT_CAP;

    _VRETURN(out_status, NT_SUCCESS);
}

void nt_charbuff_destroy(nt_charbuff_t* buff)
{
    if(buff == NULL) return;

    if(buff->data != NULL)
        free(buff->data);

    buff->data = NULL;
    buff->len = 0;
    buff->capacity = 0;
}

void nt_charbuff_append(nt_charbuff_t* buff, const char* text,
        nt_status_t* out_status)
{
    if((buff == NULL) || (text == NULL))
    {
        _VRETURN(out_status, NT_ERR_INVALID_ARG);
    }

    size_t len = strlen(text);

    size_t total_len = len + buff->len;

    if((total_len) > buff->capacity)
    {
        size_t new_cap_default = buff->capacity * 2;

        size_t new_cap = (total_len > new_cap_default) ?
            total_len * 2 : new_cap_default;

        void* new_data = realloc(buff->data, new_cap + 1);
        if(new_data == NULL)
        {
            _VRETURN(out_status, NT_ERR_ALLOC_FAIL);
        }

        buff->capacity = new_cap;
        buff->data = new_data;
    }

    memcpy(buff->data + buff->len, text, len);
    buff->len += len;
    buff->data[buff->len] = '\0';

    _VRETURN(out_status, NT_SUCCESS);
}

void nt_charbuff_rewind(nt_charbuff_t* buff, nt_status_t* out_status)
{
    if(buff == NULL)
    {
        _VRETURN(out_status, NT_ERR_INVALID_ARG);
    }
    
    buff->len = 0;
    if(buff->data != NULL)
        buff->data[0] = '\0';

    _VRETURN(out_status, NT_SUCCESS);
}
