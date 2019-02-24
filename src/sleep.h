#ifndef AXEL_SLEEP_H
#define AXEL_SLEEP_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE
#endif

static inline int
axel_sleep(struct timespec delay)
{
	int res;
	while ((res = nanosleep(&delay, &delay)) && errno == EINTR) ;
	return res;
}

#endif /* AXEL_SLEEP_H */
