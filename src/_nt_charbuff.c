#include "_nt_charbuff.h"
#include "_nt_shared.h"
#include <stdlib.h>
#include <string.h>

#define CHARBUFF_INIT_CAP 10

void nt_charbuff_init(nt_charbuff_t* buff, nt_status_t* out_status)
{
    if(buff == NULL)
    {
        _VRETURN(out_status, NT_ERR_INVALID_ARG);
    }

    buff->data = malloc(CHARBUFF_INIT_CAP * sizeof(char));
    if(buff->data == NULL)
    {
        buff->len = 0;
        buff->capacity = 0;
        _VRETURN(out_status, NT_ERR_ALLOC_FAIL);
    }

    buff->len = 0;
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

    if((buff->len + len) > buff->capacity)
    {
        void* new_data = realloc(buff->data, buff->capacity * 2);
        if(new_data == NULL)
        {
            _VRETURN(out_status, NT_ERR_ALLOC_FAIL);
        }

        buff->capacity *= 2;
        buff->data = new_data;
    }

    memcpy(buff->data + buff->len, text, len);
}
