#include <nt_terms/_nt_terms.h>

static struct nt_term* _active = NULL;

void nt_terms_init()
{
    // TODO: determine active terminal
}

const struct nt_term* nt_terms_get_active()
{
    return _active;
}

void nt_terms_destroy()
{
    _active = NULL;
}
