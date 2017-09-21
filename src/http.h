/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2008      Y Giridhar Appaji Nag
  Copyright 2016      Phillip Berndt
  Copyright 2016      Sjjad Hashemian
  Copyright 2016      Stephen Thirlwall
  Copyright 2017      Antonio Quartulli
  Copyright 2017      Ismael Luceno

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  In addition, as a special exception, the copyright holders give
  permission to link the code of portions of this program with the
  OpenSSL library under certain conditions as described in each
  individual source file, and distribute linked combinations including
  the two.

  You must obey the GNU General Public License in all respects for all
  of the code used other than OpenSSL. If you modify file(s) with this
  exception, you may extend this exception to your version of the
  file(s), but you are not obligated to do so. If you do not wish to do
  so, delete this exception statement from your version. If you delete
  this exception statement from all source files in the program, then
  also delete it here.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* HTTP control include file */

#ifndef AXEL_HTTP_H
#define AXEL_HTTP_H

#define MAX_QUERY	2048	/* Should not grow larger.. */

typedef struct {
	char host[MAX_STRING];
	char auth[MAX_STRING];
	char request[MAX_QUERY];
	char headers[MAX_QUERY];
	int port;
	int proto;		/* FTP through HTTP proxies */
	int proxy;
	char proxy_auth[MAX_STRING];
	long long int firstbyte;
	long long int lastbyte;
	int status;
	tcp_t tcp;
	char *local_if;
} http_t;

int http_connect(http_t * conn, int proto, char *proxy, char *host, int port,
		 char *user, char *pass);
void http_disconnect(http_t * conn);
void http_get(http_t * conn, char *lurl);
void http_addheader(http_t * conn, char *format, ...);
int http_exec(http_t * conn);
const char *http_header(const http_t * conn, const char *header);
void http_filename(const http_t * conn, char *filename);
long long int http_size(http_t * conn);
long long int http_size_from_range(http_t * conn);
void http_encode(char *s);
void http_decode(char *s);

#endif				/* AXEL_HTTP_H */
