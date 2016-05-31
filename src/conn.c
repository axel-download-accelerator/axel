/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2007-2008 Y Giridhar Appaji Nag
  Copyright 2008      Philipp Hagemeister
  Copyright 2015      Joao Eriberto Mota Filho
  Copyright 2016      Sjjad Hashemian
  Copyright 2016      Stephen Thirlwall

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

/* Connection stuff */

#include "axel.h"

char string[MAX_STRING];

/* Convert an URL to a conn_t structure */
int conn_set( conn_t *conn, char *set_url )
{
	char url[MAX_STRING];
	char *i, *j;

	/* protocol:// */
	if( ( i = strstr( set_url, "://" ) ) == NULL )
	{
		conn->proto = PROTO_DEFAULT;
		conn->port = PROTO_DEFAULT_PORT;
		conn->proto_name = PROTO_DEFAULT_NAME;
		strncpy( url, set_url, MAX_STRING );
	}
	else
	{
		int proto_len = i - set_url;
		if( strncmp( set_url, PROTO_FTP_NAME, proto_len ) == 0 )
		{
			conn->proto = PROTO_FTP;
			conn->port = PROTO_FTP_PORT;
			conn->proto_name = PROTO_FTP_NAME;
		}
		else if( strncmp( set_url, PROTO_HTTP_NAME, proto_len ) == 0 )
		{
			conn->proto = PROTO_HTTP;
			conn->port = PROTO_HTTP_PORT;
			conn->proto_name = PROTO_HTTP_NAME;
		}
#ifdef HAVE_OPENSSL
		else if( strncmp( set_url, PROTO_FTPS_NAME, proto_len ) == 0 )
		{
			conn->proto = PROTO_FTPS;
			conn->port = PROTO_FTPS_PORT;
			conn->proto_name = PROTO_FTPS_NAME;
		}
		else if( strncmp( set_url, PROTO_HTTPS_NAME, proto_len ) == 0 )
		{
			conn->proto = PROTO_HTTPS;
			conn->port = PROTO_HTTPS_PORT;
			conn->proto_name = PROTO_HTTPS_NAME;
		}
#endif /* HAVE_OPENSSL */
		else
		{
			return( 0 );
		}
		strncpy( url, i + 3, MAX_STRING );
	}

	/* Split */
	if( ( i = strchr( url, '/' ) ) == NULL )
	{
		strcpy( conn->dir, "/" );
	}
	else
	{
		*i = 0;
		snprintf( conn->dir, MAX_STRING, "/%s", i + 1 );
		if( conn->proto == PROTO_HTTP || conn->proto == PROTO_HTTPS )
			http_encode( conn->dir );
	}
	strncpy( conn->host, url, MAX_STRING );
	j = strchr( conn->dir, '?' );
	if( j != NULL )
		*j = 0;
	i = strrchr( conn->dir, '/' );
	*i = 0;
	if( j != NULL )
		*j = '?';
	if( i == NULL )
	{
		strncpy( conn->file, conn->dir, MAX_STRING );
		strcpy( conn->dir, "/" );
	}
	else
	{
		strncpy( conn->file, i + 1, MAX_STRING );
		strcat( conn->dir, "/" );
	}

	/* Check for username in host field */
	if( strrchr( conn->host, '@' ) != NULL )
	{
		strncpy( conn->user, conn->host, MAX_STRING );
		i = strrchr( conn->user, '@' );
		*i = 0;
		strncpy( conn->host, i + 1, MAX_STRING );
		*conn->pass = 0;
	}
	/* If not: Fill in defaults */
	else
	{
		if( PROTO_IS_FTP( conn->proto ) )
		{
			/* Dash the password: Save traffic by trying
			   to avoid multi-line responses */
			strcpy( conn->user, "anonymous" );
			strcpy( conn->pass, "mailto:axel@axel.project" );
		}
		else
		{
			*conn->user = *conn->pass = 0;
		}
	}

	/* Password? */
	if( ( i = strchr( conn->user, ':' ) ) != NULL )
	{
		*i = 0;
		strncpy( conn->pass, i + 1, MAX_STRING );
	}
	/* Port number? */
	if( ( i = strchr( conn->host, ':' ) ) != NULL )
	{
		*i = 0;
		sscanf( i + 1, "%i", &conn->port );
	}

	return( conn->port > 0 );
}

/* Generate a nice URL string. */
char *conn_url( conn_t *conn )
{
	strcpy( string, conn->proto_name );
	strcat( string, "://" );

	if( *conn->user != 0 && strcmp( conn->user, "anonymous" ) != 0 )
		sprintf( string + strlen( string ), "%s:%s@",
			conn->user, conn->pass );

	sprintf( string + strlen( string ), "%s:%i%s%s",
		conn->host, conn->port, conn->dir, conn->file );

	return( string );
}

/* Simple... */
void conn_disconnect( conn_t *conn )
{
	if( PROTO_IS_FTP( conn->proto ) && !conn->proxy )
		ftp_disconnect( conn->ftp );
	else
		http_disconnect( conn->http );
	conn->tcp = NULL;
}

int conn_init( conn_t *conn )
{
	char *proxy = conn->conf->http_proxy, *host = conn->conf->no_proxy;
	int i;

	if( *conn->conf->http_proxy == 0 )
	{
		proxy = NULL;
	}
	else if( *conn->conf->no_proxy != 0 )
	{
		for( i = 0; ; i ++ )
			if( conn->conf->no_proxy[i] == 0 )
			{
				if( strstr( conn->host, host ) != NULL )
					proxy = NULL;
				host = &conn->conf->no_proxy[i+1];
				if( conn->conf->no_proxy[i+1] == 0 )
					break;
			}
	}

	conn->proxy = proxy != NULL;

	if( PROTO_IS_FTP( conn->proto ) && !conn->proxy )
	{
		conn->ftp->local_if = conn->local_if;
		conn->ftp->ftp_mode = FTP_PASSIVE;
		if( !ftp_connect( conn->ftp, conn->proto, conn->host, conn->port, conn->user, conn->pass ) )
		{
			conn->message = conn->ftp->message;
			conn_disconnect( conn );
			return( 0 );
		}
		conn->message = conn->ftp->message;
		if( !ftp_cwd( conn->ftp, conn->dir ) )
		{
			conn_disconnect( conn );
			return( 0 );
		}
	}
	else
	{
		conn->http->local_if = conn->local_if;
		if( !http_connect( conn->http, conn->proto, proxy, conn->host, conn->port, conn->user, conn->pass ) )
		{
			conn->message = conn->http->headers;
			conn_disconnect( conn );
			return( 0 );
		}
		conn->message = conn->http->headers;
		conn->tcp = &conn->http->tcp;
	}
	return( 1 );
}

int conn_setup( conn_t *conn )
{
	if( conn->ftp->tcp.fd <= 0 && conn->http->tcp.fd <= 0 )
		if( !conn_init( conn ) )
			return( 0 );

	if( PROTO_IS_FTP( conn->proto ) && !conn->proxy )
	{
		if( !ftp_data( conn->ftp ) )	/* Set up data connnection */
			return( 0 );
		conn->tcp = &conn->ftp->data_tcp;

		if( conn->currentbyte )
		{
			ftp_command( conn->ftp, "REST %lld", conn->currentbyte );
			if( ftp_wait( conn->ftp ) / 100 != 3 &&
			    conn->ftp->status / 100 != 2 )
				return( 0 );
		}
	}
	else
	{
		char s[MAX_STRING];
		int i;

		snprintf( s, MAX_STRING, "%s%s", conn->dir, conn->file );
		conn->http->firstbyte = conn->currentbyte;
		conn->http->lastbyte = conn->lastbyte;
		http_get( conn->http, s );
		http_addheader( conn->http, "User-Agent: %s", conn->conf->user_agent );
		for( i = 0; i < conn->conf->add_header_count; i++)
			http_addheader( conn->http, "%s", conn->conf->add_header[i] );
	}
	return( 1 );
}

int conn_exec( conn_t *conn )
{
	if( PROTO_IS_FTP( conn->proto ) && !conn->proxy )
	{
		if( !ftp_command( conn->ftp, "RETR %s", conn->file ) )
			return( 0 );
		return( ftp_wait( conn->ftp ) / 100 == 1 );
	}
	else
	{
		if( !http_exec( conn->http ) )
			return( 0 );
		return( conn->http->status / 100 == 2 );
	}
}

/* Get file size and other information */
int conn_info( conn_t *conn )
{
	/* It's all a bit messed up.. But it works. */
	if( PROTO_IS_FTP( conn->proto ) && !conn->proxy )
	{
		ftp_command( conn->ftp, "REST %lld", 1 );
		if( ftp_wait( conn->ftp ) / 100 == 3 ||
		    conn->ftp->status / 100 == 2 )
		{
			conn->supported = 1;
			ftp_command( conn->ftp, "REST %lld", 0 );
			ftp_wait( conn->ftp );
		}
		else
		{
			conn->supported = 0;
		}

		if( !ftp_cwd( conn->ftp, conn->dir ) )
			return( 0 );
		conn->size = ftp_size( conn->ftp, conn->file, MAX_REDIR );
		if( conn->size < 0 )
			conn->supported = 0;
		if( conn->size == -1 )
			return( 0 );
		else if( conn->size == -2 )
			conn->size = INT_MAX;
	}
	else
	{
		char s[MAX_STRING], *t;
		long long int i = 0;

		do
		{
			conn->currentbyte = 1;
			if( !conn_setup( conn ) )
				return( 0 );
			conn_exec( conn );
			conn_disconnect( conn );

			http_filename(conn->http, conn->output_filename);

			/* Code 3xx == redirect */
			if( conn->http->status / 100 != 3 )
				break;
			if( ( t = http_header( conn->http, "location:" ) ) == NULL )
				return( 0 );
			sscanf( t, "%1000s", s );
			if( strstr( s, "://" ) == NULL)
			{
				sprintf( conn->http->headers, "%s%s",
					conn_url( conn ), s );
				strncpy( s, conn->http->headers, MAX_STRING );
			}
			else if( s[0] == '/' )
			{
				sprintf( conn->http->headers, "http://%s:%i%s",
					conn->host, conn->port, s );
				strncpy( s, conn->http->headers, MAX_STRING );
			}
			conn_set( conn, s );
			i ++;
		}
		while( conn->http->status / 100 == 3 && i < MAX_REDIR );

		if( i == MAX_REDIR )
		{
			sprintf( conn->message, _("Too many redirects.\n") );
			return( 0 );
		}

		conn->size = http_size( conn->http );
		if( conn->http->status == 206 && conn->size >= 0 )
		{
			conn->supported = 1;
			conn->size ++;
		}
		else if( conn->http->status == 200 || conn->http->status == 206 )
		{
			conn->supported = 0;
			conn->size = INT_MAX;
		}
		else
		{
			t = strchr( conn->message, '\n' );
			if( t == NULL )
				sprintf( conn->message, _("Unknown HTTP error.\n") );
			else
				*t = 0;
			return( 0 );
		}
	}

	return( 1 );
}
