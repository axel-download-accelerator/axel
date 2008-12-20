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

enum connstate {
	INITIALIZED, // Just initialized
	REQUESTED, // First byte went out to the network
	DOWNLOADING, // Read complete header, currently downloading the file itself
	FINISHED, // Completed entire download
	ERROR
};

typedef struct {
	conf_t *conf;
	
	url_t* url; /* The URL to download from. Not owned by this struct */
	proto_t* proto;
	
	long long currentbyte;
	long long lastbyte;
	
	int fd;
	int enabled;
	int supported;
	int last_transfer;
	
	connstate cstate;
	char *message;
	
	pthread_t thread[1];
} conn_t;

void conn_init(conn_t *conn, url_t* url, conf_t* conf, long long startbyte, long long endbyte);

