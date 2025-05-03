#include "nuterm.h"
#include <assert.h>
#include "_uconv.h"
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* -------------------------------------------------------------------------- */

#define _RETURN(ret_val, out_status_param, out_status)                         \
    if((out_status_param) != NULL)                                             \
        (*out_status_param) = (out_status);                                    \
    return (ret_val)                                                           \

#define _VRETURN(out_status_param, out_status)                                 \
    if((out_status_param) != NULL)                                             \
        (*out_status_param) = out_status;                                      \
    return                                                                     \

/* -------------------------------------------------------------------------- */

#define ESC_TIMEOUT 10

static int _resize_fds[2];
static struct pollfd _poll_fds[2];

struct nt_key
{
    size_t id;
    uint8_t seq[10];
};

static struct nt_key _keys[23] = {
    { .id = NT_KEY_ESC_ARROW_UP,    .seq = { 0x1B, 0x5B, 0x41, 0x00 } },
    { .id = NT_KEY_ESC_ARROW_RIGHT, .seq = { 0x1B, 0x5B, 0x43, 0x00 } },
    { .id = NT_KEY_ESC_ARROW_DOWN,  .seq = { 0x1B, 0x5B, 0x42, 0x00 } },
    { .id = NT_KEY_ESC_ARROW_LEFT,  .seq = { 0x1B, 0x5B, 0x44, 0x00 } },
    { .id = NT_KEY_ESC_F1,          .seq = { 0x1B, 0x4F, 0x50, 0x00 } },
    { .id = NT_KEY_ESC_F2,          .seq = { 0x1B, 0x4F, 0x51, 0x00 } },
    { .id = NT_KEY_ESC_F3,          .seq = { 0x1B, 0x4F, 0x52, 0x00 } },
    { .id = NT_KEY_ESC_F4,          .seq = { 0x1B, 0x4F, 0x53, 0x00 } },
    { .id = NT_KEY_ESC_F5,          .seq = { 0x1B, 0x5B, 0x31, 0x35, 0x7E, 0x00 } },
    { .id = NT_KEY_ESC_F6,          .seq = { 0x1B, 0x5B, 0x31, 0x37, 0x7E, 0x00 } },
    { .id = NT_KEY_ESC_F7,          .seq = { 0x1B, 0x5B, 0x31, 0x38, 0x7E, 0x00 } },
    { .id = NT_KEY_ESC_F8,          .seq = { 0x1B, 0x5B, 0x31, 0x39, 0x7E, 0x00 } },
    { .id = NT_KEY_ESC_F9,          .seq = { 0x1B, 0x5B, 0x32, 0x30, 0x7E, 0x00 } },
    { .id = NT_KEY_ESC_F10,         .seq = { 0x1B, 0x5B, 0x32, 0x31, 0x7E, 0x00 } },
    { .id = NT_KEY_ESC_F11,         .seq = { 0x1B, 0x5B, 0x32, 0x33, 0x7E, 0x00 } },
    { .id = NT_KEY_ESC_F12,         .seq = { 0x1B, 0x5B, 0x32, 0x34, 0x7E, 0x00 } },
};

/* -------------------------------------------------------------------------- */

/* Assume there is at least 1 byte of data for reading */

static struct nt_key_event _handle_key_event(nt_status_t* out_status);

static struct nt_key_event _handle_key_event_utf32(uint8_t buff[], bool alt,
        nt_status_t* out_status);

static struct nt_key_event _handle_key_event_csi(uint8_t buff[],
        nt_status_t* out_status);

static struct nt_resize_event _handle_resize_event(nt_status_t* out_status);

/* -------------------------------------------------------------------------- */

void nt_init(nt_status_t* out_status)
{
    int pipe_status = pipe(_resize_fds);
    if(pipe_status != 0)
    {
        _VRETURN(out_status, NT_ERR_PIPE);
    }

    _poll_fds[0] = (struct pollfd) {
        .fd = STDIN_FILENO,
        .events = POLLIN,
        .revents = 0
    };
    _poll_fds[1] = (struct pollfd) {
        .fd = _resize_fds[0],
        .events = POLLIN,
        .revents = 0
    };
}

static const struct nt_event _NT_EVENT_EMPTY = {0};

struct nt_event nt_wait_for_event(int timeout, nt_status_t* out_status)
{
    int poll_status;
    do
    {
        poll_status = poll(_poll_fds, 2, timeout);
    } while((poll_status == -1) && (errno == EINTR));
    if(poll_status == -1)
    {
        _RETURN(_NT_EVENT_EMPTY, out_status, NT_ERR_POLL);
    }

     struct nt_event event;
     if(_poll_fds[0].revents & POLLIN)
     {
         struct nt_key_event key_event = _handle_key_event(NULL);
         event.type = NT_EVENT_TYPE_KEY;
         event.key_data = key_event;
     }
     else // (poll_fds[1].revents & POLLIN)
     {
         struct nt_resize_event resize_event = _handle_resize_event(NULL);
         event.type = NT_EVENT_TYPE_RESIZE;
         event.resize_data = resize_event;
     }

     _RETURN(event, out_status, NT_SUCCESS);
}

static const struct nt_key_event _NT_KEY_EVENT_EMPTY = {0};

static struct nt_key_event _handle_key_event(nt_status_t* out_status)
{
    unsigned char buff[64];
    int poll_status, read_status;

    do // read()
    {
        read_status = read(STDIN_FILENO, buff, 1);
    } while((read_status < 0) && (errno == EINTR));
    if(read_status < 0)
    {
        _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_READ);
    }

    if(buff[0] == 27) // ESC or ESC SEQ or ALT + PRINTABLE
    {
        do // poll()
        {
            poll_status = poll(_poll_fds, 1, ESC_TIMEOUT);
        } while((poll_status == -1) && (errno == EINTR));
        if(poll_status == -1)
        {
            _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_POLL);
        }

        if(poll_status == 0) // timeout - no extra data
        {
            return (struct nt_key_event) {
                .type = NT_KEY_EVENT_UTF32,
                .codepoint_data = (struct nt_key_event_utf32) {
                    .codepoint = 27,
                    .alt = false
                }
            };
        }
        else // data present - ESC SEQ or ALT + PRINTABLE
        {
            do // read()
            {
                read_status = read(STDIN_FILENO, buff + 1, 1);
            } while((read_status < 0) && (errno == EINTR));
            if(read_status < 0)
            {
                _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_READ);
            }

            if(buff[1] == '[') // CSI
            {
                do // poll()
                {
                    poll_status = poll(_poll_fds, 1, ESC_TIMEOUT);
                } while((poll_status == -1) && (errno == EINTR));
                if(poll_status == -1)
                {
                    _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_POLL);
                }

                if(poll_status == 0)
                {
                    return (struct nt_key_event) {
                        .type = NT_KEY_EVENT_UTF32,
                        .codepoint_data = (struct nt_key_event_utf32) {
                            .codepoint = '[',
                            .alt = true
                        }
                    };
                }

                _handle_key_event_csi(buff, NULL);
                // handle status
            }
            else // ALT + PRINTABLE ?
            {
                struct nt_key_event key_event =
                    _handle_key_event_utf32(buff + 1, true, NULL);

                _RETURN(key_event, out_status, NT_SUCCESS);
                // handle status
            }
        }
    }
    else
    {
        struct nt_key_event key_event =
            _handle_key_event_utf32(buff, false, NULL);

        _RETURN(key_event, out_status, NT_SUCCESS);
        // handle status
    }

    return (struct nt_key_event) {0};
}

static struct nt_key_event _handle_key_event_utf32(uint8_t buff[], bool alt,
        nt_status_t* out_status)
{
    size_t utf32_len;
    utf32_len = uc_utf8_char_len(buff[0]);
    if(utf32_len == SIZE_MAX)
    {
        _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_INVALID_UTF8);
    }

    // read()
    int read_status;
    do
    {
        read_status = read(STDIN_FILENO, buff + 1, utf32_len - 1);
    } while((read_status < 0) && (errno == EINTR));
    if(read_status < 0)
    {
        _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_READ);
    }

    
    if(read_status != (utf32_len - 1))
    {
        _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_READ); // ?
    }

    uint32_t utf32;
    size_t utf32_width;
    uc_status_t _status;
    uc_utf8_to_utf32(buff, utf32_len, &utf32, 1, 0, &utf32_width, &_status);
    assert(_status == UC_SUCCESS); // TODO

    return (struct nt_key_event) {
        .type = NT_KEY_EVENT_UTF32,
        .codepoint_data = (struct nt_key_event_utf32) {
            .codepoint = utf32,
            .alt = alt
        }
    };
}

static inline bool _esc_are_equal(uint8_t* seq1, uint8_t* seq2)
{
    size_t i = 0;
    while(true)
    {
        if(seq1[i] != seq2[i]) // If not equal, terminate
            return false;

        // If (seq1[i] == 0) && (seq1[i] == seq2[i]) return true
        if(seq1[i] == 0) return true;

        i++;
    }
}

static struct nt_key_event _handle_key_event_csi(uint8_t buff[],
        nt_status_t* out_status)
{
    int read_status;
    int read_count = 1;
    while(true)
    {
        do // read()
        {
            read_status = read(STDIN_FILENO, buff + read_count, 1);
        } while((read_status < 0) && (errno == EINTR));
        if(read_status < 0)
        {
            _RETURN(_NT_KEY_EVENT_EMPTY, out_status, NT_ERR_READ);
        }

        read_count++;
        if((buff[read_count] >= 0x40) && (buff[read_count] <= 0x7E))
            break;

    }

    size_t i;
    size_t len = sizeof(_keys) / sizeof(struct nt_key);
    for(i = 0; i < len; i++)
    {
        if(_esc_are_equal(buff, _keys[i].seq))
        {
            return (struct nt_key_event) {
                .type = NT_KEY_EVENT_SPECIAL,
                .esc_seq_data = (struct nt_key_event_esc_seq) {
                    .key_id = _keys[i].id
                }
            };
        }
    }

    _RETURN();
}

// find esc seq

static struct nt_resize_event _handle_resize_event(nt_status_t* out_status)
{
    return (struct nt_resize_event) {0};
}
