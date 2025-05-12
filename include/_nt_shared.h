#ifndef __NT_SHARED_H__
#define __NT_SHARED_H__

#define _RETURN(ret_val, out_status_param, out_status)                         \
    if((out_status_param) != NULL)                                             \
        (*out_status_param) = (out_status);                                    \
    return (ret_val)                                                           \

#define _VRETURN(out_status_param, out_status)                                 \
    if((out_status_param) != NULL)                                             \
        (*out_status_param) = out_status;                                      \
    return                                                                     \

#endif // __NT_SHARED_H__
