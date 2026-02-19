#ifndef NT_SHARED_H
#define NT_SHARED_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#include <stddef.h>
#else
typedef union max_align_t {
    long double ld;
    void* p;
    long long ll;
} max_align_t;
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#include <stdbool.h>
#else
#define bool _Bool
#define true 1
#define false 0
#endif

#ifdef NT_EXPORT
#define NT_API __attribute__((visibility("default")))
#else
#define NT_API
#endif

#ifdef __cplusplus
}
#endif

#endif // NT_SHARED_H
