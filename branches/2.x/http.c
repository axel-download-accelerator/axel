  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* HTTP control file							*/

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

int http_connect( http_t *conn, int proto, char *proxy, char *host, int port, char *user, char *pass )
{
	char base64_encode[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz0123456789+/";
	char auth[MAX_STRING];
	conn_t tconn[1];
	int i;
	
	strncpy( conn->host, host, MAX_STRING );
	conn->proto = proto;
	
	if( proxy != NULL ) { if( *proxy != 0 )
	{
		sprintf( conn->host, "%s:%i", host, port );
		if( !conn_set( tconn, proxy ) )
		{
			/* We'll put the message in conn->headers, not in request */
			sprintf( conn->headers, _("Invalid proxy string: %s\n"), proxy );
			return( 0 );
		}
		host = tconn->host;
		port = tconn->port;
		conn->proxy = 1;
	}
	else
	{
		conn->proxy = 0;
	} }
	
	if( ( conn->fd = tcp_connect( host, port, conn->local_if ) ) == -1 )
	{
		/* We'll put the message in conn->headers, not in request */
		sprintf( conn->headers, _("Unable to connect to server %s:%i\n"), host, port );
		return( 0 );
	}
	
	if( *user == 0 )
	{
		*conn->auth = 0;
	}
	else
	{
		memset( auth, 0, MAX_STRING );
		snprintf( auth, MAX_STRING, "%s:%s", user, pass );
		for( i = 0; auth[i*3]; i ++ )
		{
			conn->auth[i*4] = base64_encode[(auth[i*3]>>2)];
			conn->auth[i*4+1] = base64_encode[((auth[i*3]&3)<<4)|(auth[i*3+1]>>4)];
			conn->auth[i*4+2] = base64_encode[((auth[i*3+1]&15)<<2)|(auth[i*3+2]>>6)];
			conn->auth[i*4+3] = base64_encode[auth[i*3+2]&63];
			if( auth[i*3+2] == 0 ) conn->auth[i*4+3] = '=';
			if( auth[i*3+1] == 0 ) conn->auth[i*4+2] = '=';
		}
	}
	
	return( 1 );
}

void http_disconnect( http_t *conn )
{
	if( conn->fd > 0 )
		close( conn->fd );
	conn->fd = -1;
}

void http_get( http_t *conn, char *lurl )
{
	*conn->request = 0;
	if( conn->proxy )
	{
		http_addheader( conn, "GET %s://%s%s HTTP/1.0",
			conn->proto == PROTO_HTTP ? "http" : "ftp", conn->host, lurl );
	}
	else
	{
		http_addheader( conn, "GET %s HTTP/1.0", lurl );
		http_addheader( conn, "Host: %s", conn->host );
	}
	if( *conn->auth )
		http_addheader( conn, "Authorization: Basic %s", conn->auth );
	if( conn->firstbyte )
	{
		if( conn->lastbyte )
			http_addheader( conn, "Range: bytes=%lld-%lld", conn->firstbyte, conn->lastbyte );
		else
			http_addheader( conn, "Range: bytes=%lld-", conn->firstbyte );
	}
}

void http_addheader( http_t *conn, char *format, ... )
{
	char s[MAX_STRING];
	va_list params;
	
	va_start( params, format );
	vsnprintf( s, MAX_STRING - 3, format, params );
	strcat( s, "\r\n" );
	va_end( params );
	
	strncat( conn->request, s, MAX_QUERY - strlen(conn->request) - 1);
}

int http_exec( http_t *conn )
{
	int i = 0;
	char s[2] = " ", *s2;

#ifdef DEBUG
	fprintf( stderr, "--- Sending request ---\n%s--- End of request ---\n", conn->request );
#endif

	http_addheader( conn, "" );
	write( conn->fd, conn->request, strlen( conn->request ) );
	
	*conn->headers = 0;
	/* Read the headers byte by byte to make sure we don't touch the
	   actual data							*/
	while( 1 )
	{
		if( read( conn->fd, s, 1 ) <= 0 )
		{
			/* We'll put the message in conn->headers, not in request */
			sprintf( conn->headers, _("Connection gone.\n") );
			return( 0 );
		}
		if( *s == '\r' )
		{
			continue;
		}
		else if( *s == '\n' )
		{
			if( i == 0 )
				break;
			i = 0;
		}
		else
		{
			i ++;
		}
		strncat( conn->headers, s, MAX_QUERY );
	}

#ifdef DEBUG
	fprintf( stderr, "--- Reply headers ---\n%s--- End of headers ---\n", conn->headers );
#endif
	
	sscanf( conn->headers, "%*s %3i", &conn->status );
	s2 = strchr( conn->headers, '\n' ); *s2 = 0;
	strcpy( conn->request, conn->headers );
	*s2 = '\n';
	
	return( 1 );
}

char *http_header( http_t *conn, char *header )
{
	char s[32];
	int i;
	
	for( i = 1; conn->headers[i]; i ++ )
		if( conn->headers[i-1] == '\n' )
		{
			sscanf( &conn->headers[i], "%31s", s );
			if( strcasecmp( s, header ) == 0 )
				return( &conn->headers[i+strlen(header)] );
		}
	
	return( NULL );
}

long long int http_size( http_t *conn )
{
	char *i;
	long long int j;
	
	if( ( i = http_header( conn, "Content-Length:" ) ) == NULL )
		return( -2 );
	
	sscanf( i, "%lld", &j );
	return( j );
}

/* Decode%20a%20file%20name						*/
void http_decode( char *s )
{
	char t[MAX_STRING];
	int i, j, k;
	
	for( i = j = 0; s[i]; i ++, j ++ )
	{
		t[j] = s[i];
		if( s[i] == '%' )
			if( sscanf( s + i + 1, "%2x", &k ) )
			{
				t[j] = k;
				i += 2;
			}
	}
	t[j] = 0;
	
	strcpy( s, t );
}

void http_encode( char *s )
{
	char t[MAX_STRING];
	int i, j;
	
	for( i = j = 0; s[i]; i ++, j ++ )
	{
		/* Fix buffer overflow */
		if (j >= MAX_STRING - 1) {
			break;
		}
		
		t[j] = s[i];
		if( s[i] == ' ' )
		{
			/* Fix buffer overflow */
			if (j >= MAX_STRING - 3) {
				break;
			}
			
			strcpy( t + j, "%20" );
			j += 2;
		}
	}
	t[j] = 0;
	
	strcpy( s, t );
}
