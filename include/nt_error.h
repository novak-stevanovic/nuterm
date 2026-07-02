#ifndef NT_ERROR_H
#define NT_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nt_shared.h"

/* A function that accepts an `out_status` parameter reports success by
 * setting it to 0, and reports failure by setting it to the appropriate
 * error code. */

#define NT_ERROR_BASE 2000
#define NT_ERR_INIT_PIPE (NT_ERROR_BASE + 1)
#define NT_ERR_UNEXPECTED (NT_ERROR_BASE + 2)
#define NT_ERR_FUNC_NOT_SUPP (NT_ERROR_BASE + 3)
#define NT_ERR_INVALID_ARG (NT_ERROR_BASE + 4)
#define NT_ERR_ALLOC_FAIL (NT_ERROR_BASE + 5)
#define NT_ERR_INIT_TERM_ENV (NT_ERROR_BASE + 6)
#define NT_ERR_TERM_NOT_SUPP (NT_ERROR_BASE + 7)
#define NT_ERR_INVALID_UTF32 (NT_ERROR_BASE + 8)
#define NT_ERR_OUT_OF_BOUNDS (NT_ERROR_BASE + 9)
#define NT_ERR_ALR_BUFF (NT_ERROR_BASE + 10)

#ifdef __cplusplus
}
#endif

#endif // NT_ERROR_H
