  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* Main include file							*/

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License with
  the Debian GNU/Linux distribution in file /usr/doc/copyright/GPL;
  if not, write to the Free Software Foundation, Inc., 59 Temple Place,
  Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"

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

/* Compiled-in settings							*/
#define MAX_STRING		1024
#define MAX_ADD_HEADERS	10
#define MAX_REDIR		5
#define AXEL_VERSION_STRING	"2.4"
#define DEFAULT_USER_AGENT	"Axel " AXEL_VERSION_STRING " (" ARCH ")"

typedef struct
{
	void *next;
	char text[MAX_STRING];
} message_t;

typedef message_t url_t;
typedef message_t if_t;

#include "conf.h"
#include "tcp.h"
#include "ftp.h"
#include "http.h"
#include "conn.h"
#include "search.h"

#define min( a, b )		( (a) < (b) ? (a) : (b) )
#define max( a, b )		( (a) > (b) ? (a) : (b) )

typedef struct
{
	conn_t *conn;
	conf_t conf[1];
	char filename[MAX_STRING];
	double start_time;
	int next_state, finish_time;
	long long bytes_done, start_byte, size;
	int bytes_per_second;
	int delay_time;
	int outfd;
	int ready;
	message_t *message;
	url_t *url;
} axel_t;

axel_t *axel_new( conf_t *conf, int count, void *url );
int axel_open( axel_t *axel );
void axel_start( axel_t *axel );
void axel_do( axel_t *axel );
void axel_close( axel_t *axel );

double gettime();
