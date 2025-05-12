#ifndef _NT_CHARBUFF_H_
#define _NT_CHARBUFF_H_

#include "nt_shared.h"
#include <stddef.h>

typedef struct nt_charbuff
{
    char* data;
    size_t len;
    size_t capacity;
} nt_charbuff_t;

void nt_charbuff_init(nt_charbuff_t* buff, nt_status_t* out_status);
void nt_charbuff_destroy(nt_charbuff_t* buff);

void nt_charbuff_append(nt_charbuff_t* buff, const char* text,
        nt_status_t* out_status);

void nt_charbuff_rewind(nt_charbuff_t* buff, nt_status_t* out_status);

#endif // _NT_CHARBUFF_H_
