#include <unistd.h>
#include "config.h"
#include "axel.h"

static int rnd_fd = -1;

int
axel_rnd_init(void)
{
	const char rnd_dev[] = "/dev/random";
	rnd_fd = open(rnd_dev, O_RDONLY);
	if (rnd_fd == -1)
		perror(rnd_dev);
	return rnd_fd;
}

ssize_t
axel_rand64(uint64_t *out)
{
	return read(rnd_fd, out, sizeof(*out));
}
