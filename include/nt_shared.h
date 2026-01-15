#ifndef NT_SHARED_H
#define NT_SHARED_H

#ifdef __cplusplus
extern "C" {
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
