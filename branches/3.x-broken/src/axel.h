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

#include "../cfg/config.h"

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
#define MAX_REDIR		5
#define AXEL_VERSION_STRING	"3.0_rc1"
#define DEFAULT_USER_AGENT	"Axel " AXEL_VERSION_STRING " (" ARCH ")"
#define STATEFILE_SUFFIX ".st"

#include "helper.h"
#include "url.h"
#include "conf.h"
#include "tcp.h"
#include "ftp.h"
#include "http.h"
#include "proto.h"
#include "conn.h"
#include "search.h"
#include "urllist.h"

// Message relevance
#define VERBOSITY_VERBOSE 2
#define VERBOSITY_NORMAL 1
#define VERBOSITY_QUIET 0

// Emergency error codes
#define AXEL_EXIT_MALLOC_FAIL 91
#define AXEL_EXIT_REALLOC_FAIL 92

// Actual file size is unknown
#define AXEL_SIZE_UNDETERMINED (-1)

// Download correctly finished
#define AXEL_STATE_FINISHED 0
// axel struct just created
#define AXEL_STATE_INIT 1
// Downloading
#define AXEL_STATE_DOWNLOADING 2
// Finished with an error
#define AXEL_STATE_ERROR 3

struct string_li_struct {
	char* str;
	struct string_li_struct* next;
};
typedef struct string_li_struct string_li_t;

struct int_li_struct {
	int i;
	struct int_li_struct* next;
};
typedef struct int_li_struct int_li_t;


// A download to a single file, probably from multiple sources
struct axel_struct {
	/*
	Unlike all other elements of this structure, the following handlers may be set by functions not starting with axel_.
	They are guaranteed to be called in a single thread, i.e. may safely ignore any multithread issues.
	*/
	
	/** A function that displays messages with the following parameters:
	
	axel: A pointer to the calling axel structure
	verbosity: one of the VERBOSITY_* values
	message: The message to display (already i18ned), will be freed upon returning
	
	NULL for no message display
	*/
	void (*message_handler)(const struct axel_struct* axel, int verbosity, const char* message);
	/**
	* A handler that displays the current download state.
	* May be called frequently. Is guaranteed to be called on state changes and messages.
	* Leave as NULL for no update notifications.
	*/
	void (*display_handler)(const struct axel_struct* axel);
	
	const conf_t* conf; // Not owned by this structure
	urllist_t urls[1]; // Sorted list of URLs to read
	
	int conncount; // The number of connections, -1 if conn is not yet initialized
	conn_t** conn; // array of connections, of size conncount. Owned by this struct.
	
	
	/**
	* The local file name to write to. Belongs to this struct.
	* NULL if this is not determined.
	*/
	char* filename;
	int outfd; // Handle where to write to or -1 if it is not yet opened
	
	char* statefilename; // Name of the state file, NULL for no state file
	
	long long size; // The full file size in Byte, or AXEL_SIZE_UNDETERMINED if the file size is not yet determined or undeterminable
	axel_time start_utime; // Start time in microseconds
	
	// The download's state, one of the AXEL_STATE_* constants
	int state;
	
	// Time to wait because of speed limit.
	int delay_time;
};
typedef struct axel_struct axel_t;

// Main axel API: The following methods are used by the frontend.
void axel_init(axel_t* ax, const conf_t *conf);
_Bool axel_addurlstr(axel_t* axel, const char* urlstr, int priority);
axel_state axel_download(axel_t* axel);
void axel_destroy(axel_t* axel);

// These functions are only called from axel's core
void axel_message(const axel_t* axel, int verbosity, const char* message);
void axel_message_fmt(const axel_t *axel, int verbosity, const char *format, ... );
