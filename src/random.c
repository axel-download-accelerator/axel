#include <stdatomic.h>
#include <unistd.h>
#include "config.h"
#include "axel.h"

uint64_t
axel_rand64(void)
{
	static int fd = -1;
	if (fd == -1) {
		int tmp = open("/dev/random", O_RDONLY);
		int expect = -1;
		if (!atomic_compare_exchange_strong(&fd, &expect, tmp))
			close(tmp);
	}

	uint64_t ret[1];
	read(fd, ret, sizeof(*ret));
	return *ret;
}
