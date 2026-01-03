#ifndef _NT_STATUS_H_
#define _NT_STATUS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int nt_status;

#define _NT_STATUS_BASE 0
#define NT_SUCCESS (_NT_STATUS_BASE + 0)
#define NT_ERR_INIT_PIPE (_NT_STATUS_BASE + 1)
#define NT_ERR_UNEXPECTED (_NT_STATUS_BASE + 2)
#define NT_ERR_FUNC_NOT_SUPP (_NT_STATUS_BASE + 3)
#define NT_ERR_INVALID_ARG (_NT_STATUS_BASE + 4)
#define NT_ERR_ALLOC_FAIL (_NT_STATUS_BASE + 5)
#define NT_ERR_INIT_TERM_ENV (_NT_STATUS_BASE + 6)
#define NT_ERR_TERM_NOT_SUPP (_NT_STATUS_BASE + 7)
#define NT_ERR_INVALID_UTF32 (_NT_STATUS_BASE + 8)
#define NT_ERR_OUT_OF_BOUNDS (_NT_STATUS_BASE + 9)
#define NT_ERR_ALR_BUFF (_NT_STATUS_BASE + 10)

#ifdef __cplusplus
}
#endif

#endif // _NT_STATUS_H_
