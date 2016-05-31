/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2007-2008 Y Giridhar Appaji Nag
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

/* FTP control file */

#include "axel.h"

int ftp_connect( ftp_t *conn, int proto, char *host, int port, char *user, char *pass )
{
	conn->data_tcp.fd = -1;
	conn->message = malloc( MAX_STRING );
	conn->proto = proto;

	if( tcp_connect( &conn->tcp, host, port, PROTO_IS_SECURE(conn->proto),
		conn->local_if, conn->message ) == -1 )
		return( 0 );

	if( ftp_wait( conn ) / 100 != 2 )
		return( 0 );

	ftp_command( conn, "USER %s", user );
	if( ftp_wait( conn ) / 100 != 2 )
	{
		if( conn->status / 100 == 3 )
		{
			ftp_command( conn, "PASS %s", pass );
			if( ftp_wait( conn ) / 100 != 2 )
				return( 0 );
		}
		else
		{
			return( 0 );
		}
	}

	/* ASCII mode sucks. Just use Binary.. */
	ftp_command( conn, "TYPE I" );
	if( ftp_wait( conn ) / 100 != 2 )
		return( 0 );

	return( 1 );
}

void ftp_disconnect( ftp_t *conn )
{
	tcp_close( &conn->tcp );
	tcp_close( &conn->data_tcp );
	if( conn->message )
	{
		free( conn->message );
		conn->message = NULL;
	}

	*conn->cwd = 0;
}

/* Change current working directory */
int ftp_cwd( ftp_t *conn, char *cwd )
{
	/* Necessary at all? */
	if( strncmp( conn->cwd, cwd, MAX_STRING ) == 0 )
		return( 1 );

	ftp_command( conn, "CWD %s", cwd );
	if( ftp_wait( conn ) / 100 != 2 )
	{
		fprintf( stderr, _("Can't change directory to %s\n"), cwd );
		return( 0 );
	}

	strncpy( conn->cwd, cwd, MAX_STRING );

	return( 1 );
}

/* Get file size. Should work with all reasonable servers now */
long long int ftp_size( ftp_t *conn, char *file, int maxredir )
{
	long long int i, j, size = MAX_STRING;
	char *reply, *s, fn[MAX_STRING];

	/* Try the SIZE command first, if possible */
	if( !strchr( file, '*' ) && !strchr( file, '?' ) )
	{
		ftp_command( conn, "SIZE %s", file );
		if( ftp_wait( conn ) / 100 == 2 )
		{
			sscanf( conn->message, "%*i %lld", &i );
			return( i );
		}
		else if( conn->status / 10 != 50 )
		{
			sprintf( conn->message, _("File not found.\n") );
			return( -1 );
		}
	}

	if( maxredir == 0 )
	{
		sprintf( conn->message, _("Too many redirects.\n") );
		return( -1 );
	}

	if( !ftp_data( conn ) )
		return( -1 );

	ftp_command( conn, "LIST %s", file );
	if( ftp_wait( conn ) / 100 != 1 )
		return( -1 );

	/* Read reply from the server. */
	reply = malloc( size );
	memset( reply, 0, size );
	*reply = '\n';
	i = 1;
	while( ( j = tcp_read( &conn->data_tcp, reply + i, size - i - 3 ) ) > 0 )
	{
		i += j;
		reply[i] = 0;
		if( size - i <= 10 )
		{
			size *= 2;
			reply = realloc( reply, size );
			memset( reply + size / 2, 0, size / 2 );
		}
	}
	tcp_close( &conn->data_tcp );

	if( ftp_wait( conn ) / 100 != 2 )
	{
		free( reply );
		return( -1 );
	}

#ifdef DEBUG
	fprintf( stderr, reply );
#endif

	/* Count the number of probably legal matches: Files&Links only */
	j = 0;
	for( i = 1; reply[i] && reply[i+1]; i ++ )
		if( reply[i] == '-' || reply[i] == 'l' )
			j ++;
		else
			while( reply[i] != '\n' && reply[i] )
				i ++;

	/* No match or more than one match */
	if( j != 1 )
	{
		if( j == 0 )
			sprintf( conn->message, _("File not found.\n") );
		else
			sprintf( conn->message, _("Multiple matches for this URL.\n") );
		free( reply );
		return( -1 );
	}

	/* Symlink handling */
	if( ( s = strstr( reply, "\nl" ) ) != NULL )
	{
		/* Get the real filename */
		sscanf( s, "%*s %*i %*s %*s %*i %*s %*i %*s %100s", fn );
		strcpy( file, fn );

		/* Get size of the file linked to */
		strncpy( fn, strstr( s, "->" ) + 3, MAX_STRING );
		free( reply );
		if( ( reply = strchr( fn, '\r' ) ) != NULL )
			*reply = 0;
		if( ( reply = strchr( fn, '\n' ) ) != NULL )
			*reply = 0;
		return( ftp_size( conn, fn, maxredir - 1 ) );
	}
	/* Normal file, so read the size! And read filename because of
	   possible wildcards. */
	else
	{
		s = strstr( reply, "\n-" );
		i = sscanf( s, "%*s %*i %*s %*s %lld %*s %*i %*s %100s", &size, fn );
		if( i < 2 )
		{
			i = sscanf( s, "%*s %*i %lld %*i %*s %*i %*i %100s", &size, fn );
			if( i < 2 )
			{
				return( -2 );
			}
		}
		strcpy( file, fn );

		free( reply );
		return( size );
	}
}

/* Open a data connection. Only Passive mode supported yet, easier.. */
int ftp_data( ftp_t *conn )
{
	int i, info[6];
	char host[MAX_STRING];

	/* Already done? */
	if( conn->data_tcp.fd > 0 )
		return( 0 );

/*	if( conn->ftp_mode == FTP_PASSIVE )
	{
*/		ftp_command( conn, "PASV" );
		if( ftp_wait( conn ) / 100 != 2 )
			return( 0 );
		*host = 0;
		for( i = 0; conn->message[i]; i ++ )
		{
			if( sscanf( &conn->message[i], "%i,%i,%i,%i,%i,%i",
			            &info[0], &info[1], &info[2], &info[3],
			            &info[4], &info[5] ) == 6 )
			{
				sprintf( host, "%i.%i.%i.%i",
				         info[0], info[1], info[2], info[3] );
				break;
			}
		}
		if( !*host )
		{
			sprintf( conn->message, _("Error opening passive data connection.\n") );
			return( 0 );
		}
		if( tcp_connect( &conn->data_tcp, host, info[4] * 256 + info[5],
		    PROTO_IS_SECURE(conn->proto), conn->local_if,
		    conn->message ) == -1 )
			return( 0 );

		return( 1 );
/*	}
	else
	{
		sprintf( conn->message, _("Active FTP not implemented yet.\n" ) );
		return( 0 );
	} */
}

/* Send a command to the server */
int ftp_command( ftp_t *conn, char *format, ... )
{
	va_list params;
	char cmd[MAX_STRING];

	va_start( params, format );
	vsnprintf( cmd, MAX_STRING - 3, format, params );
	strcat( cmd, "\r\n" );
	va_end( params );

#ifdef DEBUG
	fprintf( stderr, "fd(%i)<--%s", conn->fd, cmd );
#endif

	if( tcp_write( &conn->tcp, cmd, strlen( cmd ) ) != strlen( cmd ) )
	{
		sprintf( conn->message, _("Error writing command %s\n"), format );
		return( 0 );
	}
	else
	{
		return( 1 );
	}
}

/* Read status from server. Should handle multi-line replies correctly.
   Multi-line replies suck... */
int ftp_wait( ftp_t *conn )
{
	int size = MAX_STRING, r = 0, complete, i, j;
	char *s;

	conn->message = realloc( conn->message, size );

	do
	{
		do
		{
			r += i = tcp_read( &conn->tcp, conn->message + r, 1 );
			if( i <= 0 )
			{
				sprintf( conn->message, _("Connection gone.\n") );
				return( -1 );
			}
			if( ( r + 10 ) >= size )
			{
				size += MAX_STRING;
				conn->message = realloc( conn->message, size );
			}
		}
		while( conn->message[r-1] != '\n' );
		conn->message[r] = 0;
		sscanf( conn->message, "%i", &conn->status );
		if( conn->message[3] == ' ' )
			complete = 1;
		else
			complete = 0;

		for( i = 0; conn->message[i]; i ++ ) if( conn->message[i] == '\n' )
		{
			if( complete == 1 )
			{
				complete = 2;
				break;
			}
			if( conn->message[i+4] == ' ' )
			{
				j = -1;
				sscanf( &conn->message[i+1], "%3i", &j );
				if( j == conn->status )
					complete = 1;
			}
		}
	}
	while( complete != 2 );

#ifdef DEBUG
	fprintf( stderr, "fd(%i)-->%s", conn->fd, conn->message );
#endif

	if( ( s = strchr( conn->message, '\n' ) ) != NULL )
		*s = 0;
	if( ( s = strchr( conn->message, '\r' ) ) != NULL )
		*s = 0;
	conn->message = realloc( conn->message, max( strlen( conn->message ) + 1, MAX_STRING ) );

	return( conn->status );
}
