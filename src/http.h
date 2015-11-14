  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* HTTP control include file						*/

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

#define MAX_QUERY	2048		/* Should not grow larger..	*/

typedef struct
{
	char host[MAX_STRING];
	char auth[MAX_STRING];
	char request[MAX_QUERY];
	char headers[MAX_QUERY];
	int proto;			/* FTP through HTTP proxies	*/
	int proxy;
	long long int firstbyte;
	long long int lastbyte;
	int status;
	int fd;
	char *local_if;
} http_t;

int http_connect( http_t *conn, int proto, char *proxy, char *host, int port, char *user, char *pass );
void http_disconnect( http_t *conn );
void http_get( http_t *conn, char *lurl );
void http_addheader( http_t *conn, char *format, ... );
int http_exec( http_t *conn );
char *http_header( http_t *conn, char *header );
long long int http_size( http_t *conn );
void http_encode( char *s );
void http_decode( char *s );
