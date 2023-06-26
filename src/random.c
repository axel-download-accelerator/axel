#include <stdatomic.h>
#include <unistd.h>
#include "config.h"
#include "axel.h"
#include <stdatomic.h>

ssize_t axel_rand64(uint64_t *out)
{
    static _Atomic int fd = -1;
    if (atomic_load(&fd) == -1) {
        int tmp = open("/dev/random", O_RDONLY);
        int expect = -1;
        if (!atomic_compare_exchange_strong(&fd, &expect, tmp))
            close(tmp);
    }
    return read(atomic_load(&fd), out, sizeof(*out));
}
