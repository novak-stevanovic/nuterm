#ifndef NT_ERROR_H
#define NT_ERROR_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#error "C99 or newer is required"
#endif /* C99 check */

#include "nt_shared.h"

#ifndef NT_ERR_BASE
#define NT_ERR_BASE 2000
#endif // NT_ERR_BASE

#define NT_ERR_INIT_PIPE (NT_ERR_BASE + 1)
#define NT_ERR_UNEXPECTED (NT_ERR_BASE + 2)
#define NT_ERR_FUNC_NOT_SUPP (NT_ERR_BASE + 3)
#define NT_ERR_INVALID_ARG (NT_ERR_BASE + 4)
#define NT_ERR_ALLOC_FAIL (NT_ERR_BASE + 5)
#define NT_ERR_INIT_TERM_ENV (NT_ERR_BASE + 6)
#define NT_ERR_TERM_NOT_SUPP (NT_ERR_BASE + 7)
#define NT_ERR_INVALID_UTF32 (NT_ERR_BASE + 8)
#define NT_ERR_OUT_OF_BOUNDS (NT_ERR_BASE + 9)
#define NT_ERR_ALR_BUFF (NT_ERR_BASE + 10)

#endif // NT_ERROR_H
