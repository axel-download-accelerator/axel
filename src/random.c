#include <stdatomic.h>
#include <unistd.h>
#include "config.h"
#include "axel.h"

ssize_t
axel_rand64(uint64_t *out)
{
	static _Atomic int fd = -1;
	if (fd == -1) {
		int tmp = open("/dev/random", O_RDONLY);
		int expect = -1;
		if (!atomic_compare_exchange_strong(&fd, &expect, tmp))
			close(tmp);
	}
	return read(fd, out, sizeof(*out));
}
