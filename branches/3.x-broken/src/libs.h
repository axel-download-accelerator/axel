// Include all system libraries

#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#ifndef	NOGETOPTLONG
#define _GNU_SOURCE
#include <getopt.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>

/* Internationalization							*/
#ifdef I18N
#define PACKAGE			"axel"
#define _( x )			gettext( x )
#include <libintl.h>
#include <locale.h>
#else
#define _( x )			x
#endif
