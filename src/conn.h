  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* Connection stuff							*/

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

#define PROTO_FTP	1
#define PROTO_HTTP	2
#define PROTO_DEFAULT	PROTO_FTP

typedef struct
{
	conf_t *conf;
	
	int proto;
	int port;
	int proxy;
	char host[MAX_STRING];
	char dir[MAX_STRING];
	char file[MAX_STRING];
	char user[MAX_STRING];
	char pass[MAX_STRING];
	
	ftp_t ftp[1];
	http_t http[1];
	long long int size;		/* File size, not 'connection size'..	*/
	long long int currentbyte;
	long long int lastbyte;
	int fd;
	int enabled;
	int supported;
	int last_transfer;
	char *message;
	char *local_if;

	int state;
	pthread_t setup_thread[1];
} conn_t;

int conn_set( conn_t *conn, char *set_url );
char *conn_url( conn_t *conn );
void conn_disconnect( conn_t *conn );
int conn_init( conn_t *conn );
int conn_setup( conn_t *conn );
int conn_exec( conn_t *conn );
int conn_info( conn_t *conn );
