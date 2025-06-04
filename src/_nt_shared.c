/*
 * Copyright (c) 2025 Novak Stevanović
 * Licensed under the MIT License. See LICENSE file in project root.
 */
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "_nt_shared.h"

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
