  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* A single downloading thread						*/

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

#define CONNB_UNKWOWN (-1)

enum connstate {
	INITIALIZED, // Just initialized
	REQUESTED, // First byte went out to the network
	DOWNLOADING, // Read complete header, currently downloading the file itself
	FINISHED, // Completed entire download
	ERROR
};

typedef struct {
	const conf_t *conf;
	
	const url_t* url; /* The URL to download from. Not owned by this struct */
	proto_t[1] proto;
	
	long long currentbyte; // The index of the byte we're currently reading, starting with zero.
	long long lastbyte; // The zero-based index of the last byte we should read. CONNB_UNKWOWN if everything should be read.
	
	volatile connstate cstate;
	
	// Must only be read from other modules while cstate == DOWNLOADING
	int fd;
	
	// The following header settings are only set if state in {DOWNLOADING,FINISHED}
	long long segsize; // Size of the whole segment we're downloading, CONNB_UNKWOWN if unknown
	_Bool resume_supported; // 1 iff the server we're downloading from allows ranged downloading.
	
	// The thread that handles this connection. Only defined while connstate in {INITIALIZED, REQUESTED}
	pthread_t[1] thread;
} conn_t;

void conn_init(conn_t *conn, const url_t* url, const conf_t* conf, long long startbyte, long long endbyte);
// Start a thread that initiates downloading
void conn_start(conn_t* conn);
// Reads all headers, blocks until read. cstate is guaranteed to be either DOWNLOADING or FINISHED afterwards.
void conn_readheaders(conn_t* conn);
// Read up to bufsize bytes from this connection into buf. Segmantics are same as for the POSIX read() call.
ssize_t conn_read(conn_t* conn, char* buf, size_t bufsize);
void conn_destroy(conn_t* conn);
