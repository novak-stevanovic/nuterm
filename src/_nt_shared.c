#include "_nt_shared.h"
#include "nt_shared.h"
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct nt_xy nt_get_term_size(nt_status_t* out_status)
{
    struct winsize size;
    int status = ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
    if(status == -1)
    {
        _RETURN((struct nt_xy) {0}, out_status, NT_ERR_FUNC_NOT_SUPPORTED);
    }

    struct nt_xy ret;
    ret.x = size.ws_col;
    ret.y = size.ws_row;

    _RETURN(ret, out_status, NT_SUCCESS);
}

int nt_aread(int fd, void* dest, size_t count)
{
    int status;
    do
    {
        status = read(fd, dest, count);
    } while((status == -1) && (errno == EINTR));

    return status;
}

int nt_apoll(struct pollfd pollfds[], size_t count, size_t timeout)
{
    int status;
    do
    {
        status = poll(pollfds, count, timeout);
    } while((status == -1) && (errno == EINTR));

    return status;
}

int nt_awrite(int fd, const void* src, size_t count)
{
    int status;
    do
    {
        status = write(fd, src, count);
    } while((status == -1) && (errno == EINTR));

    return status;
}
