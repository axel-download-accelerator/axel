/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast

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

/* TCP control include file */

typedef struct {
	int fd;
	SSL *ssl;
} tcp_t;

int tcp_connect( tcp_t *tcp, char *hostname, int port, int secure, char *local_if );
void tcp_close( tcp_t *tcp );

int tcp_read( tcp_t *tcp, void *buffer, int size );
int tcp_write( tcp_t *tcp, void *buffer, int size );

int get_if_ip( char *iface, char *ip );
