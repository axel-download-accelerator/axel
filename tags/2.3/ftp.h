  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* FTP control include file						*/

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

#define FTP_PASSIVE	1
#define FTP_PORT	2

typedef struct
{
	char cwd[MAX_STRING];
	char *message;
	int status;
	int fd;
	int data_fd;
	int ftp_mode;
	char *local_if;
} ftp_t;

int ftp_connect( ftp_t *conn, char *host, int port, char *user, char *pass );
void ftp_disconnect( ftp_t *conn );
int ftp_wait( ftp_t *conn );
int ftp_command( ftp_t *conn, char *format, ... );
int ftp_cwd( ftp_t *conn, char *cwd );
int ftp_data( ftp_t *conn );
long long int ftp_size( ftp_t *conn, char *file, int maxredir );
