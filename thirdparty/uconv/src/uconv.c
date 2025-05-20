/* MIT License
 *
 * Copyright (c) 2025 Novak Stevanović
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights  
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell  
 * copies of the Software, and to permit persons to whom the Software is  
 * furnished to do so, subject to the following conditions:  
 * 
 * The above copyright notice and this permission notice shall be included in all  
 * copies or substantial portions of the Software.  
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN  
 * THE SOFTWARE. 
 */
#include "uconv.h"

#define _SURROGATE_RANGE_START 0xD800
#define _SURROGATE_RANGE_END 0xDFFF

#define _1BYTE_RANGE_START 0x0000
#define _1BYTE_RANGE_END 0x007F
#define _2BYTE_RANGE_START 0x0080
#define _2BYTE_RANGE_END 0x07FF
#define _3BYTE_RANGE_START 0x0800
#define _3BYTE_RANGE_END 0xFFFF
#define _4BYTE_RANGE_START 0x010000
#define _4BYTE_RANGE_END 0x10FFFF

static inline bool _codepoint_surrogate(uint32_t codepoint)
{
    return ((codepoint >= _SURROGATE_RANGE_START) &&
            (codepoint <= _SURROGATE_RANGE_END));
}

static inline bool _codepoint_overflow(uint32_t codepoint)
{
    return (codepoint > _4BYTE_RANGE_END);
}

static inline bool _utf8_overlong(uint32_t codepoint, size_t unit_utf8_size)
{
    switch(unit_utf8_size)
    {
        case 1:
            return false;
        case 2:
            return (codepoint < _2BYTE_RANGE_START);
        case 3:
            return (codepoint < _3BYTE_RANGE_START);
        case 4:
            return (codepoint < _4BYTE_RANGE_START);
        default: // undefined
            return true;
    }
}

#define __RETURN(width, status)                                                \
    if(out_width != NULL)                                                      \
        *out_width = (width);                                                  \
    if(out_status != NULL)                                                     \
        *out_status = (status);                                                \
    return                                                                     \

void uc_utf8_to_utf32(const uint8_t* utf8_seq, size_t len,
        uint32_t* out_utf32_seq, size_t capacity, uc_flags_t flags,
        size_t* out_width, uc_status_t* out_status)
{
    if(utf8_seq == NULL)
    {
        __RETURN(0, UC_ERR_INVALID_ARG);
    }

    size_t i = 0;
    size_t counter = 0;
    size_t i_len;
    uint32_t i_cp;
    for(; i < len; counter++)
    {
        if((out_utf32_seq != NULL) && (counter >= capacity))
        {
            __RETURN(counter, UC_ERR_NOT_ENOUGH_CAPACITY);
        }
        
        i_len = uc_utf8_char_len(utf8_seq[i]);
        switch(i_len)
        {
            case 1:
                i_cp = (uint32_t)(utf8_seq[i]);
                break;
            case 2:
                i_cp = ((uint32_t)(utf8_seq[i] & 0x3F)) << 6;
                break;
            case 3:
                i_cp = ((uint32_t)(utf8_seq[i] & 0x1F) << 12);
                break;
            case 4:
                i_cp = ((uint32_t)(utf8_seq[i] & 0x0F) << 18);
                break;
            default:
                __RETURN(counter, UC_ERR_INVALID_SBYTE);
        }

        if((i + i_len) > len)
        {
            __RETURN(counter, UC_ERR_INVALID_SIZE);
        }

        // Process (it_len - 1) continuation bytes
        size_t j, j_shift;
        for(j = 1; j < i_len; j++)
        {
            if((utf8_seq[i + j] & 0xC0) != 0x80) // invalid continuation byte
            {
                __RETURN(counter, UC_ERR_INVALID_CBYTE);
            }
            j_shift = (i_len - j - 1) * 6;

            i_cp |= ((utf8_seq[i + j] & 0x7F) << j_shift);
        }
        i += j;

        // Check for overlong if needed
        if(!(flags & UC_ALLOW_OVERLONG) &&
            _utf8_overlong(i_cp, i_len))
        {
            __RETURN(counter, UC_ERR_OVERLONG);
        }

        // Check for surrogate if needed
        if(!(flags & UC_ALLOW_SURROGATE) && _codepoint_surrogate(i_cp))
        {
            __RETURN(counter, UC_ERR_SURROGATE);
        }

        if(out_utf32_seq != NULL)
            out_utf32_seq[counter] = i_cp;
    }

    __RETURN(counter, UC_SUCCESS);
}


size_t uc_utf8_char_len(const uint8_t utf8_start_byte)
{
    if((utf8_start_byte & 0x80) == 0) return 1;
    else if((utf8_start_byte & 0xE0) == 0xC0) return 2;
    else if((utf8_start_byte & 0xF0) == 0xE0) return 3;
    else if((utf8_start_byte & 0xF8) == 0xF0) return 4;
    else return SIZE_MAX;
}

#undef __RETURN
#define __RETURN(width, len, status)                                           \
    if(out_width != NULL)                                                      \
        *out_width = (width);                                                  \
    if(out_len != NULL)                                                        \
        *out_len = (len);                                                      \
    if(out_status != NULL)                                                     \
        *out_status = (status);                                                \
    return                                                                     \

void uc_utf32_to_utf8(const uint32_t* utf32_seq, size_t width,
        uint8_t* out_utf8_seq, size_t capacity, uc_flags_t flags,
        size_t* out_width, size_t* out_len, uc_status_t* out_status)
{
    if(utf32_seq == NULL)
    {
        __RETURN(0, 0, UC_ERR_INVALID_ARG);
    }

    size_t i = 0, j;
    size_t bcount = 0;
    uint32_t i_cp;
    size_t i_len;
    uint8_t i_char;
    for(; i < width; i++, bcount += i_len)
    {
        i_cp = utf32_seq[i];
        if(!(flags & UC_ALLOW_SURROGATE) && _codepoint_surrogate(i_cp))
        {
            __RETURN(i, bcount, UC_ERR_SURROGATE);
        }
        if(_codepoint_overflow(i_cp))
        {
            __RETURN(i, bcount, UC_ERR_INVALID_CODEPOINT);
        }

        if(i_cp <= _1BYTE_RANGE_END)
        {
            i_len = 1;
            out_utf8_seq[bcount] = (uint8_t)i_cp;
        }
        else if(i_cp <= _2BYTE_RANGE_END)
        {
            // 0 0 00000abc defghijk
            // 1. 110abcde
            // 2. 10fghijk

            i_len = 2;
            i_char = ((uint8_t)(i_cp >> 6)) | 0xC0;
        }
        else if(i_cp <= _3BYTE_RANGE_END)
        {
            // 0 0 abcdefgh ijklmnop
            // 1. 1110abcd
            // 2. 10efghij
            // 3. 10klmnop

            i_len = 3;
            i_char = ((uint8_t)(i_cp >> 12)) | 0xE0;
        }
        else // 4 bytes
        {
            // 0 000abcde fghijklm nopqrstu
            // 1. 11110abc
            // 2. 10defghi
            // 3. 10jklmno
            // 4. 10pqrstu

            i_len = 4;
            i_char = ((uint8_t)(i_cp >> 18)) | 0xF0;
        }
        
        if(out_utf8_seq != NULL)
            out_utf8_seq[bcount] = i_char;

        if((out_utf8_seq != NULL) && ((bcount + i_len) > capacity))
        {
            __RETURN(i, bcount, UC_ERR_NOT_ENOUGH_CAPACITY);
        }

        // Process continuation bytes
        size_t j_shift;
        uint8_t j_cbyte;
        for(j = 1; j < i_len; j++)
        {
            j_shift = 6 * (i_len - 1 - j);
            j_cbyte = (((uint8_t)(i_cp >> j_shift)) & (~0xC0)) | 0x80;

            if(out_utf8_seq != NULL) 
                out_utf8_seq[bcount + j] = j_cbyte;
        }

    }

    __RETURN(i, bcount, UC_SUCCESS);
}
