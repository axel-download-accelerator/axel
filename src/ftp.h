/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2008      Y Giridhar Appaji Nag

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* FTP control include file */

#define FTP_PASSIVE	1
#define FTP_PORT	2

typedef struct
{
	char cwd[MAX_STRING];
	char *message;
	int status;
	tcp_t tcp;
	tcp_t data_tcp;
	int proto;
	int ftp_mode;
	char *local_if;
} ftp_t;

int ftp_connect( ftp_t *conn, int proto, char *host, int port, char *user, char *pass );
void ftp_disconnect( ftp_t *conn );
int ftp_wait( ftp_t *conn );
int ftp_command( ftp_t *conn, char *format, ... );
int ftp_cwd( ftp_t *conn, char *cwd );
int ftp_data( ftp_t *conn );
long long int ftp_size( ftp_t *conn, char *file, int maxredir );
