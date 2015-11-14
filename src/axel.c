  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* Main control								*/

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

/* Axel */
static void save_state( axel_t *axel );
static void *setup_thread( void * );
static void axel_message( axel_t *axel, char *format, ... );
static void axel_divide( axel_t *axel );

static char *buffer = NULL;

/* Create a new axel_t structure					*/
axel_t *axel_new( conf_t *conf, int count, void *url )
{
	search_t *res;
	axel_t *axel;
	url_t *u;
	char *s;
	int i;
	
	axel = malloc( sizeof( axel_t ) );
	memset( axel, 0, sizeof( axel_t ) );
	*axel->conf = *conf;
	axel->conn = malloc( sizeof( conn_t ) * axel->conf->num_connections );
	memset( axel->conn, 0, sizeof( conn_t ) * axel->conf->num_connections );
	if( axel->conf->max_speed > 0 )
	{
		if( (float) axel->conf->max_speed / axel->conf->buffer_size < 0.5 )
		{
			if( axel->conf->verbose >= 2 )
				axel_message( axel, _("Buffer resized for this speed.") );
			axel->conf->buffer_size = axel->conf->max_speed;
		}
		axel->delay_time = (int) ( (float) 1000000 / axel->conf->max_speed * axel->conf->buffer_size * axel->conf->num_connections );
	}
	if( buffer == NULL )
		buffer = malloc( max( MAX_STRING, axel->conf->buffer_size ) );
	
	if( count == 0 )
	{
		axel->url = malloc( sizeof( url_t ) );
		axel->url->next = axel->url;
		strncpy( axel->url->text, (char *) url, MAX_STRING );
	}
	else
	{
		res = (search_t *) url;
		u = axel->url = malloc( sizeof( url_t ) );
		for( i = 0; i < count; i ++ )
		{
			strncpy( u->text, res[i].url, MAX_STRING );
			if( i < count - 1 )
			{
				u->next = malloc( sizeof( url_t ) );
				u = u->next;
			}
			else
			{
				u->next = axel->url;
			}
		}
	}
	
	axel->conn[0].conf = axel->conf;
	if( !conn_set( &axel->conn[0], axel->url->text ) )
	{
		axel_message( axel, _("Could not parse URL.\n") );
		axel->ready = -1;
		return( axel );
	}

	axel->conn[0].local_if = axel->conf->interfaces->text;
	axel->conf->interfaces = axel->conf->interfaces->next;
	
	strncpy( axel->filename, axel->conn[0].file, MAX_STRING );
	http_decode( axel->filename );
	if( *axel->filename == 0 )	/* Index page == no fn		*/
		strncpy( axel->filename, axel->conf->default_filename, MAX_STRING );
	if( ( s = strchr( axel->filename, '?' ) ) != NULL && axel->conf->strip_cgi_parameters )
		*s = 0;		/* Get rid of CGI parameters		*/
	
	if( !conn_init( &axel->conn[0] ) )
	{
		axel_message( axel, axel->conn[0].message );
		axel->ready = -1;
		return( axel );
	}
	
	/* This does more than just checking the file size, it all depends
	   on the protocol used.					*/
	if( !conn_info( &axel->conn[0] ) )
	{
		axel_message( axel, axel->conn[0].message );
		axel->ready = -1;
		return( axel );
	}
	s = conn_url( axel->conn );
	strncpy( axel->url->text, s, MAX_STRING );
	if( ( axel->size = axel->conn[0].size ) != INT_MAX )
	{
		if( axel->conf->verbose > 0 )
			axel_message( axel, _("File size: %lld bytes"), axel->size );
	}
	
	/* Wildcards in URL --> Get complete filename			*/
	if( strchr( axel->filename, '*' ) || strchr( axel->filename, '?' ) )
		strncpy( axel->filename, axel->conn[0].file, MAX_STRING );
	
	return( axel );
}

/* Open a local file to store the downloaded data			*/
int axel_open( axel_t *axel )
{
	int i, fd;
	long long int j;
	
	if( axel->conf->verbose > 0 )
		axel_message( axel, _("Opening output file %s"), axel->filename );
	snprintf( buffer, MAX_STRING, "%s.st", axel->filename );
	
	axel->outfd = -1;
	
	/* Check whether server knows about RESTart and switch back to
	   single connection download if necessary			*/
	if( !axel->conn[0].supported )
	{
		axel_message( axel, _("Server unsupported, "
			"starting from scratch with one connection.") );
		axel->conf->num_connections = 1;
		axel->conn = realloc( axel->conn, sizeof( conn_t ) );
		axel_divide( axel );
	}
	else if( ( fd = open( buffer, O_RDONLY ) ) != -1 )
	{
		read( fd, &axel->conf->num_connections, sizeof( axel->conf->num_connections ) );
		
		axel->conn = realloc( axel->conn, sizeof( conn_t ) * axel->conf->num_connections );
		memset( axel->conn + 1, 0, sizeof( conn_t ) * ( axel->conf->num_connections - 1 ) );

		axel_divide( axel );
		
		read( fd, &axel->bytes_done, sizeof( axel->bytes_done ) );
		for( i = 0; i < axel->conf->num_connections; i ++ )
			read( fd, &axel->conn[i].currentbyte, sizeof( axel->conn[i].currentbyte ) );

		axel_message( axel, _("State file found: %lld bytes downloaded, %lld to go."),
			axel->bytes_done, axel->size - axel->bytes_done );
		
		close( fd );
		
		if( ( axel->outfd = open( axel->filename, O_WRONLY, 0666 ) ) == -1 )
		{
			axel_message( axel, _("Error opening local file") );
			return( 0 );
		}
	}

	/* If outfd == -1 we have to start from scrath now		*/
	if( axel->outfd == -1 )
	{
		axel_divide( axel );
		
		if( ( axel->outfd = open( axel->filename, O_CREAT | O_WRONLY, 0666 ) ) == -1 )
		{
			axel_message( axel, _("Error opening local file") );
			return( 0 );
		}
		
		/* And check whether the filesystem can handle seeks to
		   past-EOF areas.. Speeds things up. :) AFAIK this
		   should just not happen:				*/
		if( lseek( axel->outfd, axel->size, SEEK_SET ) == -1 && axel->conf->num_connections > 1 )
		{
			/* But if the OS/fs does not allow to seek behind
			   EOF, we have to fill the file with zeroes before
			   starting. Slow..				*/
			axel_message( axel, _("Crappy filesystem/OS.. Working around. :-(") );
			lseek( axel->outfd, 0, SEEK_SET );
			memset( buffer, 0, axel->conf->buffer_size );
			j = axel->size;
			while( j > 0 )
			{
				write( axel->outfd, buffer, min( j, axel->conf->buffer_size ) );
				j -= axel->conf->buffer_size;
			}
		}
	}
	
	return( 1 );
}

/* Start downloading							*/
void axel_start( axel_t *axel )
{
	int i;
	
	/* HTTP might've redirected and FTP handles wildcards, so
	   re-scan the URL for every conn				*/
	for( i = 0; i < axel->conf->num_connections; i ++ )
	{
		conn_set( &axel->conn[i], axel->url->text );
		axel->url = axel->url->next;
		axel->conn[i].local_if = axel->conf->interfaces->text;
		axel->conf->interfaces = axel->conf->interfaces->next;
		axel->conn[i].conf = axel->conf;
		if( i ) axel->conn[i].supported = 1;
	}
	
	if( axel->conf->verbose > 0 )
		axel_message( axel, _("Starting download") );
	
	for( i = 0; i < axel->conf->num_connections; i ++ )
	if( axel->conn[i].currentbyte <= axel->conn[i].lastbyte )
	{
		if( axel->conf->verbose >= 2 )
		{
			axel_message( axel, _("Connection %i downloading from %s:%i using interface %s"),
		        	      i, axel->conn[i].host, axel->conn[i].port, axel->conn[i].local_if );
		}
		
		axel->conn[i].state = 1;
		if( pthread_create( axel->conn[i].setup_thread, NULL, setup_thread, &axel->conn[i] ) != 0 )
		{
			axel_message( axel, _("pthread error!!!") );
			axel->ready = -1;
		}
		else
		{
			axel->conn[i].last_transfer = gettime();
		}
	}
	
	/* The real downloading will start now, so let's start counting	*/
	axel->start_time = gettime();
	axel->ready = 0;
}

/* Main 'loop'								*/
void axel_do( axel_t *axel )
{
	fd_set fds[1];
	int hifd, i;
	long long int remaining,size;
	struct timeval timeval[1];
	
	/* Create statefile if necessary				*/
	if( gettime() > axel->next_state )
	{
		save_state( axel );
		axel->next_state = gettime() + axel->conf->save_state_interval;
	}
	
	/* Wait for data on (one of) the connections			*/
	FD_ZERO( fds );
	hifd = 0;
	for( i = 0; i < axel->conf->num_connections; i ++ )
	{
		if( axel->conn[i].enabled )
			FD_SET( axel->conn[i].fd, fds );
		hifd = max( hifd, axel->conn[i].fd );
	}
	if( hifd == 0 )
	{
		/* No connections yet. Wait...				*/
		usleep( 100000 );
		goto conn_check;
	}
	else
	{
		timeval->tv_sec = 0;
		timeval->tv_usec = 100000;
		/* A select() error probably means it was interrupted
		   by a signal, or that something else's very wrong...	*/
		if( select( hifd + 1, fds, NULL, NULL, timeval ) == -1 )
		{
			axel->ready = -1;
			return;
		}
	}
	
	/* Handle connections which need attention			*/
	for( i = 0; i < axel->conf->num_connections; i ++ )
	if( axel->conn[i].enabled ) {
	if( FD_ISSET( axel->conn[i].fd, fds ) )
	{
		axel->conn[i].last_transfer = gettime();
		size = read( axel->conn[i].fd, buffer, axel->conf->buffer_size );
		if( size == -1 )
		{
			if( axel->conf->verbose )
			{
				axel_message( axel, _("Error on connection %i! "
					"Connection closed"), i );
			}
			axel->conn[i].enabled = 0;
			conn_disconnect( &axel->conn[i] );
			continue;
		}
		else if( size == 0 )
		{
			if( axel->conf->verbose )
			{
				/* Only abnormal behaviour if:		*/
				if( axel->conn[i].currentbyte < axel->conn[i].lastbyte && axel->size != INT_MAX )
				{
					axel_message( axel, _("Connection %i unexpectedly closed"), i );
				}
				else
				{
					axel_message( axel, _("Connection %i finished"), i );
				}
			}
			if( !axel->conn[0].supported )
			{
				axel->ready = 1;
			}
			axel->conn[i].enabled = 0;
			conn_disconnect( &axel->conn[i] );
			continue;
		}
		/* remaining == Bytes to go					*/
		remaining = axel->conn[i].lastbyte - axel->conn[i].currentbyte + 1;
		if( remaining < size )
		{
			if( axel->conf->verbose )
			{
				axel_message( axel, _("Connection %i finished"), i );
			}
			axel->conn[i].enabled = 0;
			conn_disconnect( &axel->conn[i] );
			size = remaining;
			/* Don't terminate, still stuff to write!	*/
		}
		/* This should always succeed..				*/
		lseek( axel->outfd, axel->conn[i].currentbyte, SEEK_SET );
		if( write( axel->outfd, buffer, size ) != size )
		{
			
			axel_message( axel, _("Write error!") );
			axel->ready = -1;
			return;
		}
		axel->conn[i].currentbyte += size;
		axel->bytes_done += size;
	}
	else
	{
		if( gettime() > axel->conn[i].last_transfer + axel->conf->connection_timeout )
		{
			if( axel->conf->verbose )
				axel_message( axel, _("Connection %i timed out"), i );
			conn_disconnect( &axel->conn[i] );
			axel->conn[i].enabled = 0;
		}
	} }
	
	if( axel->ready )
		return;
	
conn_check:
	/* Look for aborted connections and attempt to restart them.	*/
	for( i = 0; i < axel->conf->num_connections; i ++ )
	{
		if( !axel->conn[i].enabled && axel->conn[i].currentbyte < axel->conn[i].lastbyte )
		{
			if( axel->conn[i].state == 0 )
			{	
				// Wait for termination of this thread
				pthread_join(*(axel->conn[i].setup_thread), NULL);
				
				conn_set( &axel->conn[i], axel->url->text );
				axel->url = axel->url->next;
				/* axel->conn[i].local_if = axel->conf->interfaces->text;
				axel->conf->interfaces = axel->conf->interfaces->next; */
				if( axel->conf->verbose >= 2 )
					axel_message( axel, _("Connection %i downloading from %s:%i using interface %s"),
				        	      i, axel->conn[i].host, axel->conn[i].port, axel->conn[i].local_if );
				
				axel->conn[i].state = 1;
				if( pthread_create( axel->conn[i].setup_thread, NULL, setup_thread, &axel->conn[i] ) == 0 )
				{
					axel->conn[i].last_transfer = gettime();
				}
				else
				{
					axel_message( axel, _("pthread error!!!") );
					axel->ready = -1;
				}
			}
			else
			{
				if( gettime() > axel->conn[i].last_transfer + axel->conf->reconnect_delay )
				{
					pthread_cancel( *axel->conn[i].setup_thread );
					axel->conn[i].state = 0;
				}
			}
		}
	}

	/* Calculate current average speed and finish_time		*/
	axel->bytes_per_second = (int) ( (double) ( axel->bytes_done - axel->start_byte ) / ( gettime() - axel->start_time ) );
	axel->finish_time = (int) ( axel->start_time + (double) ( axel->size - axel->start_byte ) / axel->bytes_per_second );

	/* Check speed. If too high, delay for some time to slow things
	   down a bit. I think a 5% deviation should be acceptable.	*/
	if( axel->conf->max_speed > 0 )
	{
		if( (float) axel->bytes_per_second / axel->conf->max_speed > 1.05 )
			axel->delay_time += 10000;
		else if( ( (float) axel->bytes_per_second / axel->conf->max_speed < 0.95 ) && ( axel->delay_time >= 10000 ) )
			axel->delay_time -= 10000;
		else if( ( (float) axel->bytes_per_second / axel->conf->max_speed < 0.95 ) )
			axel->delay_time = 0;
		usleep( axel->delay_time );
	}
	
	/* Ready?							*/
	if( axel->bytes_done == axel->size )
		axel->ready = 1;
}

/* Close an axel connection						*/
void axel_close( axel_t *axel )
{
	int i;
	message_t *m;
	
	/* Terminate any thread still running				*/
	for( i = 0; i < axel->conf->num_connections; i ++ )
		/* don't try to kill non existing thread */
		if ( *axel->conn[i].setup_thread != 0 )
			pthread_cancel( *axel->conn[i].setup_thread );
	
	/* Delete state file if necessary				*/
	if( axel->ready == 1 )
	{
		snprintf( buffer, MAX_STRING, "%s.st", axel->filename );
		unlink( buffer );
	}
	/* Else: Create it.. 						*/
	else if( axel->bytes_done > 0 )
	{
		save_state( axel );
	}

	/* Delete any message not processed yet				*/
	while( axel->message )
	{
		m = axel->message;
		axel->message = axel->message->next;
		free( m );
	}
	
	/* Close all connections and local file				*/
	close( axel->outfd );
	for( i = 0; i < axel->conf->num_connections; i ++ )
		conn_disconnect( &axel->conn[i] );

	free( axel->conn );
	free( axel );
}

/* time() with more precision						*/
double gettime()
{
	struct timeval time[1];
	
	gettimeofday( time, 0 );
	return( (double) time->tv_sec + (double) time->tv_usec / 1000000 );
}

/* Save the state of the current download				*/
void save_state( axel_t *axel )
{
	int fd, i;
	char fn[MAX_STRING+4];

	/* No use for such a file if the server doesn't support
	   resuming anyway..						*/
	if( !axel->conn[0].supported )
		return;
	
	snprintf( fn, MAX_STRING, "%s.st", axel->filename );
	if( ( fd = open( fn, O_CREAT | O_TRUNC | O_WRONLY, 0666 ) ) == -1 )
	{
		return;		/* Not 100% fatal..			*/
	}
	write( fd, &axel->conf->num_connections, sizeof( axel->conf->num_connections ) );
	write( fd, &axel->bytes_done, sizeof( axel->bytes_done ) );
	for( i = 0; i < axel->conf->num_connections; i ++ )
	{
		write( fd, &axel->conn[i].currentbyte, sizeof( axel->conn[i].currentbyte ) );
	}
	close( fd );
}

/* Thread used to set up a connection					*/
void *setup_thread( void *c )
{
	conn_t *conn = c;
	int oldstate;
	
	/* Allow this thread to be killed at any time.			*/
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &oldstate );
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate );
	
	if( conn_setup( conn ) )
	{
		conn->last_transfer = gettime();
		if( conn_exec( conn ) )
		{
			conn->last_transfer = gettime();
			conn->enabled = 1;
			conn->state = 0;
			return( NULL );
		}
	}
	
	conn_disconnect( conn );
	conn->state = 0;
	return( NULL );
}

/* Add a message to the axel->message structure				*/
static void axel_message( axel_t *axel, char *format, ... )
{
	message_t *m = malloc( sizeof( message_t ) ), *n = axel->message;
	va_list params;
	
	memset( m, 0, sizeof( message_t ) );
	va_start( params, format );
	vsnprintf( m->text, MAX_STRING, format, params );
	va_end( params );
	
	if( axel->message == NULL )
	{
		axel->message = m;
	}
	else
	{
		while( n->next != NULL )
			n = n->next;
		n->next = m;
	}
}

/* Divide the file and set the locations for each connection		*/
static void axel_divide( axel_t *axel )
{
	int i;
	
	axel->conn[0].currentbyte = 0;
	axel->conn[0].lastbyte = axel->size / axel->conf->num_connections - 1;
	for( i = 1; i < axel->conf->num_connections; i ++ )
	{
#ifdef DEBUG
		printf( "Downloading %lld-%lld using conn. %i\n", axel->conn[i-1].currentbyte, axel->conn[i-1].lastbyte, i - 1 );
#endif
		axel->conn[i].currentbyte = axel->conn[i-1].lastbyte + 1;
		axel->conn[i].lastbyte = axel->conn[i].currentbyte + axel->size / axel->conf->num_connections;
	}
	axel->conn[axel->conf->num_connections-1].lastbyte = axel->size - 1;
#ifdef DEBUG
	printf( "Downloading %lld-%lld using conn. %i\n", axel->conn[i-1].currentbyte, axel->conn[i-1].lastbyte, i - 1 );
#endif
}
