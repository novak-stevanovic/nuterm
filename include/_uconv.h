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
#ifndef _UCONV_H_
#define _UCONV_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UC_UNICODE_MAX 0x10FFFF

/* UTF8 SEQUENCES:
 * 1 BYTE SEQUENCE - START BYTE FORMAT: 0xxxxxxx
 * 2 BYTE SEQUENCE - START BYTE FORMAT: 110xxxxx
 * 3 BYTE SEQUENCE - START BYTE FORMAT: 1110xxxx
 * 4 BYTE SEQUENCE - START BYTE FORMAT: 11110xxx
 * CONTINUATION BYTE - FORMAT: 10xxxxxx */

/* -------------------------------------------------------------------------- */

typedef int uc_status_t;

#define UC_SUCCESS 0
#define UC_ERR_INVALID_ARG 1
#define UC_ERR_NOT_ENOUGH_CAPACITY 2
#define UC_ERR_INVALID_SIZE 3

#define UC_ERR_OVERLONG 101
#define UC_ERR_SURROGATE 102
#define UC_ERR_INVALID_CODEPOINT 103
#define UC_ERR_INVALID_SBYTE 104
#define UC_ERR_INVALID_CBYTE 105

/* -------------------------------------------------------------------------- */

typedef uint8_t uc_flags_t;

#define UC_ALLOW_SURROGATE (1 << 0)
#define UC_ALLOW_OVERLONG (1 << 1)

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
 * 1) UC_ALLOW_SURROGATE - If the function runs into a surrogate value, it will
 * not return with an error, but proceed decoding.
 * 2) UC_ALLOW_OVERLONG - If the function runs into an "overlong-ly encoded"
 * codepoint, it will not return with an error, but proceed decoding.
 *
 * STATUS CODES:
 * 1) UC_SUCCESS - Decoding is successful.
 * 2) UC_ERR_INVALID_ARG - `utf8_seq` is NULL.
 * 3) UC_ERR_NOT_ENOUGH_CAPACITY - `out_utf32_seq` was not NULL and the
 * decoding would exceed the provided `capacity`.
 * * 4) UC_ERR_INVALID_SIZE - The last processed UTF-8 "character" is cut-off due to
 * `len`.
 * 5) UC_ERR_OVERLONG - Flag UC_ALLOW_OVERLONG was not set and the UTF-8 sequence
 * ran into overlong encoding.
 * 6) UC_ERR_SURROGATE - Flag UCUC_ERR_SURROGATE was not set and the UTF-8 sequence
 * ran into a surrogate unicode value.
 * 7) UC_ERR_INVALID_SBYTE - The function ran into a UTF-8 character with a
 * start byte that has an invalid format.
 * 8) UC_ERR_INVALID_CBYTE - The function ran into a UTF-8 character with a
 * continuation byte of invalid format.
 */
void uc_utf8_to_utf32(const uint8_t* utf8_seq, size_t len,
        uint32_t* out_utf32_seq, size_t capacity, uc_flags_t flags,
        size_t* out_width, uc_status_t* out_status);

/* ------------------------------------------------------ */

/**
 * @brief Determines the length of the UTF-8 unit based on the starting byte.
 *
 * RETURN VALUE:
 * 1) ON SUCCESS: An integer value in the range [1, 4] representing the length
 * of the UTF-8 "character".
 * 2) ON FAILURE: SIZE_MAX. This can occur if the given `utf8_start_byte` is of
 * invalid format.
 */
size_t uc_utf8_char_len(const uint8_t utf8_start_byte);

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
 * 1) UC_ALLOW_SURROGATE - If the function encounters a surrogate code point,
 * it will not return an error but proceed encoding.
 *
 * STATUS CODES:
 * 1) UC_SUCCESS - Encoding completed successfully.
 * 2) UC_ERR_INVALID_ARG - `utf32_seq` is NULL.
 * 3) UC_ERR_NOT_ENOUGH_CAPACITY - `out_utf8_seq` is not NULL and the encoding
 * would exceed the `capacity` of the output buffer.
 * 4) UC_ERR_SURROGATE - Flag UC_ALLOW_SURROGATE was not set and a surrogate
 * code point was encountered.
 * 5) UC_ERR_INVALID_CODEPOINT - A code point is outside the valid Unicode
 * range (greater than U+10FFFF).
 */
void uc_utf32_to_utf8(const uint32_t* utf32_seq, size_t width,
        uint8_t* out_utf8_seq, size_t capacity, uc_flags_t flags,
        size_t* out_width, size_t* out_len, uc_status_t* out_status);

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif // _UCONV_H_
