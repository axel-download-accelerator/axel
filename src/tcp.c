  /*****************************************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices.                     *
  *                                                                                        *
  * Copyright 2001 Wilmer van der Gaast                                                    *
  * Copyright 2010 Mark Smith <nanog@85d5b20a518b8f6864949bd940457dc124746ddc.nosense.org> *
  \*****************************************************************************************/

/* TCP control file							*/

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

#include "axel.h"

/* Get a TCP connection */
int tcp_connect( char *hostname, int port, char *local_if )
{
	struct sockaddr_in local_addr;
	const int portstr_len = 10;
	char portstr[portstr_len];
	struct addrinfo ai_hints;
	struct addrinfo *gai_results, *gai_result;
	int ret;
	int sock_fd = -1;


	if (local_if && *local_if) {
		local_addr.sin_family = AF_INET;
		local_addr.sin_port = 0;
		local_addr.sin_addr.s_addr = inet_addr(local_if);
	}

	snprintf(portstr, portstr_len, "%d", port);

	memset(&ai_hints, 0, sizeof(ai_hints));
	ai_hints.ai_family = AF_UNSPEC;
	ai_hints.ai_socktype = SOCK_STREAM;
	ai_hints.ai_flags = AI_ADDRCONFIG;
	ai_hints.ai_protocol = 0;

	ret = getaddrinfo(hostname, portstr, &ai_hints, &gai_results);
	if (ret != 0) {
		return -1;
	}

	gai_result = gai_results;
	sock_fd = -1;
	while ((sock_fd == -1) && (gai_result != NULL)) {

		sock_fd = socket(gai_result->ai_family,
			gai_result->ai_socktype, gai_result->ai_protocol);

		if (sock_fd != -1) {

			if (gai_result->ai_family == AF_INET) {
				if (local_if && *local_if) {
					ret = bind(sock_fd,
						(struct sockaddr *) &local_addr,
						sizeof(local_addr));
					if (ret == -1) {
						close(sock_fd);
						sock_fd = -1;
						gai_result =
							gai_result->ai_next;
					}
				}
			}

			if (sock_fd != -1) {
				ret = connect(sock_fd, gai_result->ai_addr,
						gai_result->ai_addrlen);
				if (ret == -1) {
					close(sock_fd);
					sock_fd = -1;
					gai_result = gai_result->ai_next;
				}
			}
		}
	}

	freeaddrinfo(gai_results);

	return sock_fd;

}

int get_if_ip( char *iface, char *ip )
{
	struct ifreq ifr;
	int fd = socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );
	
	memset( &ifr, 0, sizeof( struct ifreq ) );
	
	strcpy( ifr.ifr_name, iface );
	ifr.ifr_addr.sa_family = AF_INET;
	if( ioctl( fd, SIOCGIFADDR, &ifr ) == 0 )
	{
		struct sockaddr_in *x = (struct sockaddr_in *) &ifr.ifr_addr;
		strcpy( ip, inet_ntoa( x->sin_addr ) );
		return( 1 );
	}
	else
	{
		return( 0 );
	}
}
