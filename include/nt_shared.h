#ifndef NT_SHARED_H
#define NT_SHARED_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Checks */

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#error "C99 or newer is required"
#endif /* C99 check */

#if !defined(UINT32_MAX) || !defined(UINT8_MAX)
#error "This library requires uint32_t and uint8_t support"
#endif

/* Library defines */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
typedef max_align_t nt__max_align_t;
#else
typedef union nt__max_align {
    long double ld;
    void* p;
    long long ll;
} nt__max_align_t;
#endif

#ifdef NT_EXPORT
#define NT_API __attribute__((visibility("default")))
#else
#define NT_API
#endif

#endif // NT_SHARED_H
