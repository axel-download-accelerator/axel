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
#define AXEL_VERSION_STRING	"2.99.0"
#define DEFAULT_USER_AGENT	"Axel " AXEL_VERSION_STRING " (" ARCH ")"
#define STATEFILE_SUFFIX ".st"

#define VERBOSITY_VERBOSE 2
#define VERBOSITY_NORMAL 1
#define VERBOSITY_QUIET 0

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

// Special value for delay_time, see below
#define AXEL_DELAY_TIME_ABORT (-1)

// Actual file size is unknown
#define AXEL_SIZE_UNDETERMINED (-1)

// axel struct just created
#define AXEL_STATE_INIT 1
// preparations complete
#define AXEL_STATE_READY 2
// Downloading
#define AXEL_STATE_DOWNLOADING 3
// Finished with an error
#define AXEL_STATE_ERROR 4
// Download correctly finished
#define AXEL_STATE_FINISHED 5

// TODO can we guarantee anything for the handlers?

// A download to a single file, probably from multiple sources
struct axel_struct {
	const conf_t* conf; // Not owned by this structure
	urllist_t urls; // Linked list of URLs to read
	
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
	long long start_utime; // Start time in microseconds
	
	// The download's state, one of the AXEL_STATE_* constants
	volatile sig_atomic_t state; // TODO check whether we read this state at all
	
	/**
	* Time to wait because of speed limit.
	* AXEL_DELAY_TIME_ABORT if all threads should stop instantaneously.
	*/
	volatile sig_atomic_t delay_time;
	
	/** A function that displays messages with the following parameters:
	
	axel: A pointer to the calling axel structure
	message: The message to display (already i18ned), will be freed upon returning
	verbosity: one of the VERBOSITY_* values
	
	NULL for no messages
	*/
	void (*message_handler)(const struct axel_struct* axel, const char* message, int verbosity);
	/** A pointer to a function that aborts this run of Axel. ret is a return value (non-zero for errornous abort). Leave as NULL for an exit call.
	Note the axel structure is already torn down at the point and can be cleaned with axel_free.
	*/
	void (*abort_handler)(struct axel_struct* axel, int ret);
	/**
	* A handler that displays the download's state. Is called "frequently" and on every update of state.
	* Leave as NULL for no status display
	*/
	void (*status_handler)(const struct axel_struct* axel);
};
typedef struct axel_struct axel_t;

// Main axel API: The following methods are used by the frontend.
axel_t* axel_new(const conf_t *conf);
_Bool axel_addurlstr(axel_t* axel, const char* urlstr, int priority);
void axel_start(axel_t* axel);
void axel_free(axel_t* axel);

void axel_message(const axel_t* axel, const char* message, int verbosity);
void axel_message_fmt(const axel_t *axel, const char *format, ... );
void axel_update_status(const axel_t* axel);
void axel_abort(axel_t* axel, int ret);
