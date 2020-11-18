/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2008      Y Giridhar Appaji Nag
  Copyright 2016      Stephen Thirlwall
  Copyright 2017      Antonio Quartulli
  Copyright 2017-2019 Ismael Luceno

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

/* FTP control include file */

#ifndef AXEL_FTP_H
#define AXEL_FTP_H

#define FTP_PASSIVE	1
#define FTP_PORT	2

typedef struct {
	char cwd[MAX_STRING];
	char *message;
	int status;
	tcp_t tcp;
	tcp_t data_tcp;
	int proto;
	int ftp_mode;
	char *local_if;
} ftp_t;

int ftp_connect(ftp_t *conn, int proto, char *host, int port, char *user,
		char *pass, unsigned io_timeout);
void ftp_disconnect(ftp_t *conn);
int ftp_wait(ftp_t *conn);
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif /* __GNUC__ */
int ftp_command(ftp_t *conn, const char *format, ...);
int ftp_cwd(ftp_t *conn, char *cwd);
int ftp_data(ftp_t *conn, unsigned io_timeout);
off_t ftp_size(ftp_t *conn, char *file, int maxredir, unsigned io_timeout);

#endif				/* AXEL_FTP_H */
