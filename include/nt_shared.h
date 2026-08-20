#ifndef NT_SHARED_H
#define NT_SHARED_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#error "C99 or newer is required"
#endif /* C99 check */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#include <stddef.h>
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
