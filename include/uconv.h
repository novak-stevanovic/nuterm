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

/* ========================================================================== */
/* -------------------------------------------------------------------------- */
/* PUBLIC */
/* -------------------------------------------------------------------------- */
/* ========================================================================== */

#ifndef UCONV_H
#define UCONV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UC_UNICODE_MAX 0x10FFFF
#define UC_UNICODE_SURROGATE_START 0xD800
#define UC_UNICODE_SURROGATE_END 0xDFFF

#define UC_ERR_BASE 3000
#define UC_ERR_INV_ARG (UC_ERR_BASE + 1)
#define UC_ERR_NO_CAP (UC_ERR_BASE + 2)
#define UC_ERR_INV_SIZE (UC_ERR_BASE + 3)

#define UC_ERR_OVERLONG (UC_ERR_BASE + 101)
#define UC_ERR_SURROGATE (UC_ERR_BASE + 102)
#define UC_ERR_INV_CP (UC_ERR_BASE + 103) /* invalid codepoint */
#define UC_ERR_INV_SB (UC_ERR_BASE + 104) /* invalid start byte in sequence */
#define UC_ERR_INV_CB (UC_ERR_BASE + 105) /* invalid continuation byte in sequence */

enum uc_flag
{
    UC_ALLOW_SURROGATE = (1 << 0),
    UC_ALLOW_OVERLONG = (1 << 1)
};

/* ========================================================================== */
/* CONVENIENCE */
/* ========================================================================== */

/* Checks whether a UTF-32 code point is within the allowed Unicode range.
 *
 * POSSIBLE FLAGS:
 * 1) UC_ALLOW_SURROGATE - Treats surrogate code points as valid. */

bool uc_utf32_is_in_range(uint32_t utf32_codepoint, uint8_t flags);

/* ------------------------------------------------------ */

/* Returns the length of a UTF-8 unit based on its start byte.
 *
 * RETURN VALUE: Length in bytes [1-4], or SIZE_MAX if the start byte is
 * invalid. */

size_t uc_utf8_unit_len(uint8_t utf8_sbyte);

/* ------------------------------------------------------ */

/* Decodes a single UTF-8 code point and stores it in `out_cp`.
 *
 * POSSIBLE FLAGS:
 * 1) UC_ALLOW_SURROGATE - Allows surrogate code points.
 * 2) UC_ALLOW_OVERLONG - Allows overlong UTF-8 encodings.
 *
 * RETURN VALUE: 0 on success, error code on failure.
 *
 * ERROR CODES:
 * 1) UC_ERR_INV_ARG - `utf8_seq` or `out_cp` is NULL.
 * 2) UC_ERR_NO_CAP - More input remains after one code point was decoded.
 * 3) UC_ERR_INV_SIZE - The input ends before a complete UTF-8 unit is read.
 * 4) UC_ERR_OVERLONG - An overlong encoding was encountered.
 * 5) UC_ERR_SURROGATE - A surrogate code point was encountered.
 * 6) UC_ERR_INV_CP - The decoded code point is outside the Unicode range.
 * 7) UC_ERR_INV_SB - An invalid UTF-8 start byte was encountered.
 * 8) UC_ERR_INV_CB - An invalid UTF-8 continuation byte was encountered. */

int uc_utf8_to_utf32_single(const uint8_t* utf8_seq, size_t len,
                            uint8_t flags, uint32_t* out_cp);

/* ------------------------------------------------------ */

/* Encodes a single UTF-32 code point as UTF-8.
 *
 * If `out_utf8_seq` is NULL, the encoded length can still be returned through
 * `out_len`.
 *
 * POSSIBLE FLAGS:
 * 1) UC_ALLOW_SURROGATE - Allows surrogate code points.
 *
 * RETURN VALUE: 0 on success, error code on failure.
 *
 * ERROR CODES:
 * 1) UC_ERR_SURROGATE - A surrogate code point was provided.
 * 2) UC_ERR_INV_CP - The code point is outside the Unicode range. */

int uc_utf32_to_utf8_single(uint32_t utf32, uint8_t flags,
                            uint8_t* out_utf8_seq, size_t* out_len);

/* ========================================================================== */
/* UTF8 -> UTF32 */
/* ========================================================================== */

/* Decodes up to `len` bytes from `utf8_seq` into UTF-32 code points.
 *
 * If `out_utf32_seq` is NULL, decoding is performed without storing the
 * result and `capacity` is ignored. This can be used to validate the input
 * and determine the required output width.
 *
 * If `out_width` is not NULL, it receives the number of code points decoded
 * before the function returns, on both success and failure.
 *
 * POSSIBLE FLAGS:
 * 1) UC_ALLOW_SURROGATE - Allows surrogate code points.
 * 2) UC_ALLOW_OVERLONG - Allows overlong UTF-8 encodings.
 *
 * RETURN VALUE: 0 on success, error code on failure.
 *
 * ERROR CODES:
 * 1) UC_ERR_INV_ARG - `utf8_seq` is NULL.
 * 2) UC_ERR_NO_CAP - `out_utf32_seq` does not have enough capacity.
 * 3) UC_ERR_INV_SIZE - The input ends before a complete UTF-8 unit is read.
 * 4) UC_ERR_OVERLONG - An overlong encoding was encountered.
 * 5) UC_ERR_SURROGATE - A surrogate code point was encountered.
 * 6) UC_ERR_INV_CP - A decoded code point is outside the Unicode range.
 * 7) UC_ERR_INV_SB - An invalid UTF-8 start byte was encountered.
 * 8) UC_ERR_INV_CB - An invalid UTF-8 continuation byte was encountered. */

int uc_utf8_to_utf32(const uint8_t* utf8_seq, size_t len,
                     uint32_t* out_utf32_seq, size_t capacity,
                     uint8_t flags, size_t* out_width);

/* ========================================================================== */
/* UTF32 -> UTF8 */
/* ========================================================================== */

/* Encodes `width` UTF-32 code points from `utf32_seq` as UTF-8.
 *
 * If `out_utf8_seq` is NULL, encoding is performed without storing the result
 * and `capacity` is ignored. This can be used to validate the input and
 * determine the required output length.
 *
 * If provided, `out_width` receives the number of code points encoded and
 * `out_len` receives the number of UTF-8 bytes encoded before the function
 * returns, on both success and failure.
 *
 * POSSIBLE FLAGS:
 * 1) UC_ALLOW_SURROGATE - Allows surrogate code points.
 *
 * RETURN VALUE: 0 on success, error code on failure.
 *
 * ERROR CODES:
 * 1) UC_ERR_INV_ARG - `utf32_seq` is NULL.
 * 2) UC_ERR_NO_CAP - `out_utf8_seq` does not have enough capacity.
 * 3) UC_ERR_SURROGATE - A surrogate code point was encountered.
 * 4) UC_ERR_INV_CP - A code point is outside the Unicode range. */

int uc_utf32_to_utf8(const uint32_t* utf32_seq, size_t width,
                     uint8_t* out_utf8_seq, size_t capacity, uint8_t flags,
                     size_t* out_width, size_t* out_len);

/* ========================================================================== */

#ifdef __cplusplus
}
#endif

#endif // UCONV_H

/* ========================================================================== */
/* -------------------------------------------------------------------------- */
/* INTERNAL */
/* -------------------------------------------------------------------------- */
/* ========================================================================== */

#ifdef UCONV_IMPLEMENTATION

/* UTF8 SEQUENCES:
 * 1 BYTE SEQUENCE - START BYTE FORMAT: 0xxxxxxx
 * 2 BYTE SEQUENCE - START BYTE FORMAT: 110xxxxx
 * 3 BYTE SEQUENCE - START BYTE FORMAT: 1110xxxx
 * 4 BYTE SEQUENCE - START BYTE FORMAT: 11110xxx
 * CONTINUATION BYTE FORMAT: 10xxxxxx */

#define uc_set_out(out_param, out_val) \
    if((out_param) != NULL) { (*(out_param)) = (out_val); }

static inline bool uc__utf8_overlong(uint32_t utf32_cp, size_t utf8_unit_size)
{
    switch(utf8_unit_size)
    {
        case 1:
            return false;
        case 2:
            return (utf32_cp < 0x0080);
        case 3:
            return (utf32_cp < 0x0800);
        case 4:
            return (utf32_cp < 0x010000);
        default: // undefined
            return true;
    }
}

static inline bool uc__utf32_cp_surrogate(uint32_t utf32_cp)
{
    return ((utf32_cp >= UC_UNICODE_SURROGATE_START) &&
            (utf32_cp <= UC_UNICODE_SURROGATE_END));
}

static inline bool uc__utf32_cp_overflow(uint32_t utf32_cp)
{
    return (utf32_cp > UC_UNICODE_MAX);
}

static inline size_t uc__utf8_unit_len(uint8_t utf8_sbyte)
{
    if((utf8_sbyte & 0x80) == 0) return 1;
    else if((utf8_sbyte & 0xE0) == 0xC0) return 2;
    else if((utf8_sbyte & 0xF0) == 0xE0) return 3;
    else if((utf8_sbyte & 0xF8) == 0xF0) return 4;
    else return SIZE_MAX;
}

bool uc_utf32_is_in_range(uint32_t utf32_codepoint, uint8_t flags)
{
    if(flags & UC_ALLOW_SURROGATE)
    {
        return (!uc__utf32_cp_overflow(utf32_codepoint));
    }
    else
    {
        return (!uc__utf32_cp_overflow(utf32_codepoint) &&
                !(uc__utf32_cp_surrogate(utf32_codepoint)));
    }
}


size_t uc_utf8_unit_len(uint8_t utf8_sbyte)
{
    return uc__utf8_unit_len(utf8_sbyte);
}

int uc_utf8_to_utf32_single(const uint8_t* utf8_seq, size_t len,
                            uint8_t flags, uint32_t* out_cp)
{
    if(out_cp == NULL)
        return UC_ERR_INV_ARG;

    uint32_t cp;
    int status = uc_utf8_to_utf32(utf8_seq, len, &cp, 1, flags, NULL);
    if(status != 0)
        return status;

    *out_cp = cp;
    return 0;
}

int uc_utf32_to_utf8_single(uint32_t utf32, uint8_t flags,
                            uint8_t* out_utf8_seq, size_t* out_len)
{
    return uc_utf32_to_utf8(&utf32, 1, out_utf8_seq, 4, flags,
                            NULL, out_len);
}

int uc_utf8_to_utf32(const uint8_t* utf8_seq, size_t len,
                     uint32_t* out_utf32_seq, size_t capacity, uint8_t flags,
                     size_t* out_width)
{
    if(utf8_seq == NULL)
    {
        uc_set_out(out_width, 0);
        return UC_ERR_INV_ARG;
    }

    size_t i = 0;
    size_t counter = 0;
    size_t i_len;
    uint32_t i_cp;
    for(; i < len; counter++)
    {
        if((out_utf32_seq != NULL) && (counter >= capacity))
        {
            uc_set_out(out_width, counter);
            return UC_ERR_NO_CAP;
        }
        
        i_len = uc_utf8_unit_len(utf8_seq[i]);
        switch(i_len)
        {
            case 1:
                i_cp = (uint32_t)(utf8_seq[i]);
                break;
            case 2:
                i_cp = ((uint32_t)(utf8_seq[i] & 0x1F)) << 6;
                break;
            case 3:
                i_cp = ((uint32_t)(utf8_seq[i] & 0x0F) << 12);
                break;
            case 4:
                i_cp = ((uint32_t)(utf8_seq[i] & 0x07) << 18);
                break;
            default:
                uc_set_out(out_width, counter);
                return UC_ERR_INV_SB;
        }

        if((i + i_len) > len)
        {
            uc_set_out(out_width, counter);
            return UC_ERR_INV_SIZE;
        }

        // Process (it_len - 1) continuation bytes
        size_t j, j_shift;
        for(j = 1; j < i_len; j++)
        {
            if((utf8_seq[i + j] & 0xC0) != 0x80) // invalid continuation byte
            {
                uc_set_out(out_width, counter);
                return UC_ERR_INV_CB;
            }
            j_shift = (i_len - j - 1) * 6;

            i_cp |= ((utf8_seq[i + j] & 0x3F) << j_shift);
        }
        i += j;

        // Check for overlong if needed
        if(!(flags & UC_ALLOW_OVERLONG) &&
            uc__utf8_overlong(i_cp, i_len))
        {
            uc_set_out(out_width, counter);
            return UC_ERR_OVERLONG;
        }

        // Check for surrogate if needed
        if(!(flags & UC_ALLOW_SURROGATE) && uc__utf32_cp_surrogate(i_cp))
        {
            uc_set_out(out_width, counter);
            return UC_ERR_SURROGATE;
        }

        if(uc__utf32_cp_overflow(i_cp))
        {
            uc_set_out(out_width, counter);
            return UC_ERR_INV_CP;
        }

        if(out_utf32_seq != NULL)
            out_utf32_seq[counter] = i_cp;
    }

    uc_set_out(out_width, counter);
    return 0;
}

int uc_utf32_to_utf8(const uint32_t* utf32_seq, size_t width,
                     uint8_t* out_utf8_seq, size_t capacity, uint8_t flags,
                     size_t* out_width, size_t* out_len)
{
    if(utf32_seq == NULL)
    {
        uc_set_out(out_width, 0);
        uc_set_out(out_len, 0);
        return UC_ERR_INV_ARG;
    }

    size_t i = 0, j;
    size_t bcount = 0;
    uint32_t i_cp;
    size_t i_len;
    uint8_t i_char;
    for(; i < width; i++, bcount += i_len)
    {
        i_cp = utf32_seq[i];
        if(!(flags & UC_ALLOW_SURROGATE) && uc__utf32_cp_surrogate(i_cp))
        {
            uc_set_out(out_width, i);
            uc_set_out(out_len, bcount);
            return UC_ERR_SURROGATE;
        }
        if(uc__utf32_cp_overflow(i_cp))
        {
            uc_set_out(out_width, i);
            uc_set_out(out_len, bcount);
            return UC_ERR_INV_CP;
        }

        if(i_cp <= 0x007F)
        {
            i_len = 1;
            i_char = (uint8_t)i_cp;
        }
        else if(i_cp <= 0x07FF)
        {
            // 0 0 00000abc defghijk
            // 1. 110abcde
            // 2. 10fghijk

            i_len = 2;
            i_char = ((uint8_t)(i_cp >> 6)) | 0xC0;
        }
        else if(i_cp <= 0xFFFF)
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
            uc_set_out(out_width, i);
            uc_set_out(out_len, bcount);
            return UC_ERR_NO_CAP;
        }

        // Process continuation bytes
        size_t j_shift;
        uint8_t j_cbyte;
        for(j = 1; j < i_len; j++)
        {
            j_shift = 6 * (i_len - 1 - j);
            j_cbyte = (((uint8_t)(i_cp >> j_shift)) & (0x3F)) | 0x80;

            if(out_utf8_seq != NULL) 
                out_utf8_seq[bcount + j] = j_cbyte;
        }
    }

    uc_set_out(out_width, i);
    uc_set_out(out_len, bcount);
    return 0;
}

#endif // UCONV_IMPLEMENTATION
