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

/* -------------------------------------------------------------------------- */
/* START */
/* -------------------------------------------------------------------------- */

#ifndef _UCONV_H_
#define _UCONV_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UC_UNICODE_MAX 0x10FFFF
#define UC_UNICODE_SURROGATE_START 0xD800
#define UC_UNICODE_SURROGATE_END 0xDFFF

#define UC_UTF8_1BYTE_UNIT_UNICODE_RANGE_START 0x0000
#define UC_UTF8_1BYTE_UNIT_UNICODE_RANGE_END 0x007F
#define UC_UTF8_2BYTE_UNIT_UNICODE_RANGE_START 0x0080
#define UC_UTF8_2BYTE_UNIT_UNICODE_RANGE_END 0x07FF
#define UC_UTF8_3BYTE_UNIT_UNICODE_RANGE_START 0x0800
#define UC_UTF8_3BYTE_UNIT_UNICODE_RANGE_END 0xFFFF
#define UC_UTF8_4BYTE_UNIT_UNICODE_RANGE_START 0x010000
#define UC_UTF8_4BYTE_UNIT_UNICODE_RANGE_END 0x10FFFF

/* UTF8 SEQUENCES:
 * 1 BYTE SEQUENCE - START BYTE FORMAT: 0xxxxxxx
 * 2 BYTE SEQUENCE - START BYTE FORMAT: 110xxxxx
 * 3 BYTE SEQUENCE - START BYTE FORMAT: 1110xxxx
 * 4 BYTE SEQUENCE - START BYTE FORMAT: 11110xxx
 * CONTINUATION BYTE FORMAT: 10xxxxxx */

/* -------------------------------------------------------------------------- */
/* ERROR HANDLING */
/* -------------------------------------------------------------------------- */

typedef int uc_status;

#define UC_SUCCESS 0
#define UC_ERR_INVALID_ARG 1
#define UC_ERR_NOT_ENOUGH_CAPACITY 2
#define UC_ERR_INVALID_SIZE 3

#define UC_ERR_OVERLONG 101
#define UC_ERR_SURROGATE 102
#define UC_ERR_INVALID_CODEPOINT 103
#define UC_ERR_INVALID_SBYTE 104 /* invalid start byte in sequence */
#define UC_ERR_INVALID_CBYTE 105 /* invalid continuation byte in sequence */

/* -------------------------------------------------------------------------- */
/* FLAGS */
/* -------------------------------------------------------------------------- */

typedef uint8_t uc_flags;

#define UC_FLAG_ALLOW_SURROGATE (1 << 0)
#define UC_FLAG_ALLOW_OVERLONG (1 << 1)

/* -------------------------------------------------------------------------- */
/* CONVENIENCE FUNCTIONS/MACROS */
/* -------------------------------------------------------------------------- */

/**
 * @brief Checks if the codepoint is in Unicode range.
 *
 * POSSIBLE FLAGS:
 * 1) UC_FLAG_ALLOW_SURROGATE - If the codepoint is in the surrogate range,
 * the function will return true.
 *
 * RETURN VALUE: true or false.
*/
bool uc_utf32_is_in_range(uint32_t utf32_codepoint, uc_flags flags);

/* ------------------------------------------------------ */

/**
 * @brief Determines the length of a UTF8 unit depending on the provided
 * start byte.
 *
 * RETURN VALUE: 
 * ON SUCCESS: Integer value [1-4].
 * ON FALURE: SIZE_MAX if the start byte is of invalid format.
*/
size_t uc_utf8_unit_len(uint8_t utf8_sbyte);

/* -------------------------------------------------------------------------- */
/* UTF8 -> UTF32 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Converts a UTF-8 encoded byte sequence to UTF-32.
 *
 * This function reads up to `len` bytes from the input UTF-8 sequence and decodes
 * it into UTF-32 code points, storing the result in `out_utf32_seq`. The conversion
 * stops either when the input is fully consumed or when the output buffer reaches
 * its capacity(or when an error occurs).
 *
 * If `out_utf32_seq` is NULL, the function will perform the decoding but will not
 * store the result. This can be used to check for errors and to find out the
 * required capacity to store all of the codepoints.
 * In this case, `capacity` will be ignored.
 *
 * `out_width` will return the number of successfully decoded and stored codepoints.
 * This applies for all status codes(failure or success).
 *
 * @param utf8_eq        Pointer to the input UTF-8 sequence.
 * @param len            Number of bytes in `utf8_seq` to process.
 * @param out_utf32_seq  Output buffer for the resulting UTF-32 code points.
 * @param capacity       Maximum number of UTF-32 code points the output buffer can hold.
 * @param flags          Optional flags.
 * @param out_width      Number of successfully processed UTF-8 characters.
 * @param out_status     Indicates success or a specific error condition. 
 *
 * POSSIBLE FLAGS:
 * 1) UC_FLAG_ALLOW_SURROGATE - If the function runs into a surrogate value, it will
 * not return with an error, but proceed decoding.
 * 2) UC_FLAG_ALLOW_OVERLONG - If the function runs into an "overlong-ly encoded"
 * codepoint, it will not return with an error, but proceed decoding.
 *
 * STATUS CODES:
 * 1) UC_SUCCESS - Decoding is successful.
 * 2) UC_ERR_INVALID_ARG - `utf8_seq` is NULL.
 * 3) UC_ERR_NOT_ENOUGH_CAPACITY - `out_utf32_seq` was not NULL and the
 * decoding would exceed the provided `capacity`.
 * * 4) UC_ERR_INVALID_SIZE - The last processed UTF-8 "character" is cut-off due to
 * `len`.
 * 5) UC_ERR_OVERLONG - Flag UC_FLAG_ALLOW_OVERLONG was not set and the UTF-8 sequence
 * ran into overlong encoding.
 * 6) UC_ERR_SURROGATE - Flag UC_ERR_SURROGATE was not set and the UTF-8 sequence
 * ran into a surrogate unicode value.
 * 7) UC_ERR_INVALID_SBYTE - The function ran into a UTF-8 character with a
 * start byte that has an invalid format.
 * 8) UC_ERR_INVALID_CBYTE - The function ran into a UTF-8 character with a
 * continuation byte of invalid format.
 */
void uc_utf8_to_utf32(const uint8_t* utf8_seq, size_t len,
        uint32_t* out_utf32_seq, size_t capacity, uc_flags flags,
        size_t* out_width, uc_status* out_status);

/* -------------------------------------------------------------------------- */
/* UTF32 -> UTF8 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Converts a UTF-32 code point sequence to a UTF-8 encoded byte sequence.
 *
 * This function reads up to `width` UTF-32 code points from the input `utf32_seq`
 * and encodes them into UTF-8, storing the result in `out_utf8_seq`. The conversion
 * stops either when all input code points are processed or when the output buffer
 * reaches `capacity` bytes (or when an error occurs).
 *
 * If `out_utf32_seq` is NULL, the function will perform the encoding but will not
 * store the result. This can be used to check for errors and to find out the
 * required capacity to store all of the UTF-8 characters.
 * In this case, `capacity` will be ignored.
 *
 * `out_width` will return the number of successfully encoded code points, and
 * `out_len` will return the number of bytes successfully written to `out_utf8_seq`.
 * These counts are set regardless of whether the function succeeds or fails.
 *
 * @param utf32_seq      Pointer to the input UTF-32 sequence.
 * @param width          Number of UTF-32 code points.
 * @param out_utf8_seq   Output buffer for the resulting UTF-8 byte sequence.
 * @param capacity       Maximum number of bytes the output buffer can hold.
 * @param flags          Optional flags.
 * @param out_width      Number of successfully encoded UTF-32 code points.
 * @param out_len        Number of bytes successfully written to `out_utf8_seq`.
 * @param out_status     Indicates success or a specific error condition.
 *
 * POSSIBLE FLAGS:
 * 1) UC_FLAG_ALLOW_SURROGATE - If the function encounters a surrogate code point,
 * it will not return an error but proceed encoding.
 *
 * STATUS CODES:
 * 1) UC_SUCCESS - Encoding completed successfully.
 * 2) UC_ERR_INVALID_ARG - `utf32_seq` is NULL.
 * 3) UC_ERR_NOT_ENOUGH_CAPACITY - `out_utf8_seq` is not NULL and the encoding
 * would exceed the `capacity` of the output buffer.
 * 4) UC_ERR_SURROGATE - Flag UC_FLAG_ALLOW_SURROGATE was not set and a surrogate
 * code point was encountered.
 * 5) UC_ERR_INVALID_CODEPOINT - A code point is outside the valid Unicode
 * range (greater than U+10FFFF).
 */
void uc_utf32_to_utf8(const uint32_t* utf32_seq, size_t width,
        uint8_t* out_utf8_seq, size_t capacity, uc_flags flags,
        size_t* out_width, size_t* out_len, uc_status* out_status);

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif // _UCONV_H_

#ifdef _UCONV_IMPLEMENTATION_

static inline bool _uc_utf8_overlong(uint32_t utf32_cp, size_t utf8_unit_size)
{
    switch(utf8_unit_size)
    {
        case 1:
            return false;
        case 2:
            return (utf32_cp < UC_UTF8_2BYTE_UNIT_UNICODE_RANGE_START);
        case 3:
            return (utf32_cp < UC_UTF8_3BYTE_UNIT_UNICODE_RANGE_START);
        case 4:
            return (utf32_cp < UC_UTF8_4BYTE_UNIT_UNICODE_RANGE_START);
        default: // undefined
            return true;
    }
}

static inline bool _uc_utf32_cp_surrogate(uint32_t utf32_cp)
{
    return ((utf32_cp >= UC_UNICODE_SURROGATE_START) &&
            (utf32_cp <= UC_UNICODE_SURROGATE_END));
}

static inline bool _uc_utf32_cp_overflow(uint32_t utf32_cp)
{
    return (utf32_cp > UC_UNICODE_MAX);
}

bool uc_utf32_is_in_range(uint32_t utf32_codepoint, uc_flags flags)
{
    if(flags & UC_FLAG_ALLOW_SURROGATE)
    {
        return (!_uc_utf32_cp_overflow(utf32_codepoint));
    }
    else
    {
        return (!_uc_utf32_cp_overflow(utf32_codepoint) &&
                !(_uc_utf32_cp_surrogate(utf32_codepoint)));
    }
}

size_t uc_utf8_unit_len(uint8_t utf8_sbyte)
{
    if((utf8_sbyte & 0x80) == 0) return 1;
    else if((utf8_sbyte & 0xE0) == 0xC0) return 2;
    else if((utf8_sbyte & 0xF0) == 0xE0) return 3;
    else if((utf8_sbyte & 0xF8) == 0xF0) return 4;
    else return SIZE_MAX;
}

#define _uc_return(width, status)                                              \
    if(out_width != NULL)                                                      \
        *out_width = (width);                                                  \
    if(out_status != NULL)                                                     \
        *out_status = (status);                                                \
    return                                                                     \

void uc_utf8_to_utf32(const uint8_t* utf8_seq, size_t len,
        uint32_t* out_utf32_seq, size_t capacity, uc_flags flags,
        size_t* out_width, uc_status* out_status)
{
    if(utf8_seq == NULL)
    {
        _uc_return(0, UC_ERR_INVALID_ARG);
    }

    size_t i = 0;
    size_t counter = 0;
    size_t i_len;
    uint32_t i_cp;
    for(; i < len; counter++)
    {
        if((out_utf32_seq != NULL) && (counter >= capacity))
        {
            _uc_return(counter, UC_ERR_NOT_ENOUGH_CAPACITY);
        }
        
        i_len = uc_utf8_unit_len(utf8_seq[i]);
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
                _uc_return(counter, UC_ERR_INVALID_SBYTE);
        }

        if((i + i_len) > len)
        {
            _uc_return(counter, UC_ERR_INVALID_SIZE);
        }

        // Process (it_len - 1) continuation bytes
        size_t j, j_shift;
        for(j = 1; j < i_len; j++)
        {
            if((utf8_seq[i + j] & 0xC0) != 0x80) // invalid continuation byte
            {
                _uc_return(counter, UC_ERR_INVALID_CBYTE);
            }
            j_shift = (i_len - j - 1) * 6;

            i_cp |= ((utf8_seq[i + j] & 0x7F) << j_shift);
        }
        i += j;

        // Check for overlong if needed
        if(!(flags & UC_FLAG_ALLOW_OVERLONG) &&
            _uc_utf8_overlong(i_cp, i_len))
        {
            _uc_return(counter, UC_ERR_OVERLONG);
        }

        // Check for surrogate if needed
        if(!(flags & UC_FLAG_ALLOW_SURROGATE) && _uc_utf32_cp_surrogate(i_cp))
        {
            _uc_return(counter, UC_ERR_SURROGATE);
        }

        if(out_utf32_seq != NULL)
            out_utf32_seq[counter] = i_cp;
    }

    _uc_return(counter, UC_SUCCESS);
}


#undef _uc_return
#define _uc_return(width, len, status)                                         \
    if(out_width != NULL)                                                      \
        *out_width = (width);                                                  \
    if(out_len != NULL)                                                        \
        *out_len = (len);                                                      \
    if(out_status != NULL)                                                     \
        *out_status = (status);                                                \
    return                                                                     \

void uc_utf32_to_utf8(const uint32_t* utf32_seq, size_t width,
        uint8_t* out_utf8_seq, size_t capacity, uc_flags flags,
        size_t* out_width, size_t* out_len, uc_status* out_status)
{
    if(utf32_seq == NULL)
    {
        _uc_return(0, 0, UC_ERR_INVALID_ARG);
    }

    size_t i = 0, j;
    size_t bcount = 0;
    uint32_t i_cp;
    size_t i_len;
    uint8_t i_char;
    for(; i < width; i++, bcount += i_len)
    {
        i_cp = utf32_seq[i];
        if(!(flags & UC_FLAG_ALLOW_SURROGATE) && _uc_utf32_cp_surrogate(i_cp))
        {
            _uc_return(i, bcount, UC_ERR_SURROGATE);
        }
        if(_uc_utf32_cp_overflow(i_cp))
        {
            _uc_return(i, bcount, UC_ERR_INVALID_CODEPOINT);
        }

        if(i_cp <= UC_UTF8_1BYTE_UNIT_UNICODE_RANGE_END)
        {
            i_len = 1;
            i_char = (uint8_t)i_cp;
        }
        else if(i_cp <= UC_UTF8_2BYTE_UNIT_UNICODE_RANGE_END)
        {
            // 0 0 00000abc defghijk
            // 1. 110abcde
            // 2. 10fghijk

            i_len = 2;
            i_char = ((uint8_t)(i_cp >> 6)) | 0xC0;
        }
        else if(i_cp <= UC_UTF8_3BYTE_UNIT_UNICODE_RANGE_END)
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
            _uc_return(i, bcount, UC_ERR_NOT_ENOUGH_CAPACITY);
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

    _uc_return(i, bcount, UC_SUCCESS);
}

#endif // _UCONV_IMPLEMENTATION_
