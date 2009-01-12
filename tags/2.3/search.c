  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* filesearching.com searcher						*/

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

static char *strrstr( char *haystack, char *needle );
static void *search_speedtest( void *r );
static int search_sortlist_qsort( const void *a, const void *b );

#ifdef STANDALONE
int main( int argc, char *argv[] )
{
	conf_t conf[1];
	search_t *res;
	int i, j;
	
	if( argc != 2 )
	{
		fprintf( stderr, "Incorrect amount of arguments\n" );
		return( 1 );
	}
	
	conf_init( conf );
	
	res = malloc( sizeof( search_t ) * ( conf->search_amount + 1 ) );
	memset( res, 0, sizeof( search_t ) * ( conf->search_amount + 1 ) );
	res->conf = conf;
	
	i = search_makelist( res, argv[1] );
	if( i == -1 )
	{
		fprintf( stderr, "File not found\n" );
		return( 1 );
	}
	printf( "%i usable mirrors:\n", search_getspeeds( res, i ) );
	search_sortlist( res, i );
	for( j = 0; j < i; j ++ )
		printf( "%-70.70s %5i\n", res[j].url, res[j].speed );
	
	return( 0 );
}
#endif

int search_makelist( search_t *results, char *url )
{
	int i, size = 8192, j = 0;
	char *s, *s1, *s2, *s3;
	conn_t conn[1];
	double t;
	
	memset( conn, 0, sizeof( conn_t ) );
	
	conn->conf = results->conf;
	t = gettime();
	if( !conn_set( conn, url ) )
		return( -1 );
	if( !conn_init( conn ) )
		return( -1 );
	if( !conn_info( conn ) )
		return( -1 );
	
	strcpy( results[0].url, url );
	results[0].speed = 1 + 1000 * ( gettime() - t );
	results[0].size = conn->size;
	
	s = malloc( size );
	
	sprintf( s, "http://www.filesearching.com/cgi-bin/s?q=%s&w=a&l=en&"
		"t=f&e=on&m=%i&o=n&s1=%lld&s2=%lld&x=15&y=15",
		conn->file, results->conf->search_amount,
		conn->size, conn->size );
	
	conn_disconnect( conn );
	memset( conn, 0, sizeof( conn_t ) );
	conn->conf = results->conf;
	
	if( !conn_set( conn, s ) )
	{
		free( s );
		return( 1 );
	}
	if( !conn_setup( conn ) )
	{
		free( s );
		return( 1 );
	}
	if( !conn_exec( conn ) )
	{
		free( s );
		return( 1 );
	}
	
	while( ( i = read( conn->fd, s + j, size - j ) ) > 0 )
	{
		j += i;
		if( j + 10 >= size )
		{
			size *= 2;
			s = realloc( s, size );
			memset( s + size / 2, 0, size / 2 );
		}
	}

	conn_disconnect( conn );
	
	s1 = strstr( s, "<pre class=list" );
	s1 = strchr( s1, '\n' ) + 1;
	if( strstr( s1, "</pre>" ) == NULL )
	{
		/* Incomplete list					*/
		free( s );
		return( 1 );
	}
	for( i = 1; strncmp( s1, "</pre>", 6 ) && i < results->conf->search_amount && *s1; i ++ )
	{
		s3 = strchr( s1, '\n' ); *s3 = 0;
		s2 = strrstr( s1, "<a href=" ) + 8;
		*s3 = '\n';
		s3 = strchr( s2, ' ' ); *s3 = 0;
		if( strcmp( results[0].url, s2 ) )
		{
			strncpy( results[i].url, s2, MAX_STRING );
			results[i].size = results[0].size;
			results[i].conf = results->conf;
		}
		else
		{
			/* The original URL might show up		*/
			i --;
		}
		for( s1 = s3; *s1 != '\n'; s1 ++ );
		s1 ++;
	}
	
	free( s );
	
	return( i );
}

#define SPEED_ACTIVE	-1
#define SPEED_ERROR	-2
#define SPEED_DONE	-3	/* Or >0				*/

int search_getspeeds( search_t *results, int count )
{
	int i, running = 0, done = 0, correct = 0;
	
	for( i = 0; i < count; i ++ ) if( results[i].speed )
	{
		results[i].speed_start_time = 0;
		done ++;
		if( results[i].speed > 0 )
			correct ++;
	}
	
	while( done < count )
	{
		for( i = 0; i < count; i ++ )
		{
			if( running < results->conf->search_threads && !results[i].speed )
			{
				results[i].speed = SPEED_ACTIVE;
				results[i].speed_start_time = gettime();
				if( pthread_create( results[i].speed_thread,
					NULL, search_speedtest, &results[i] ) == 0 )
				{
					running ++;
					break;
				}
				else
				{
					return( 0 );
				}
			}
			else if( ( results[i].speed == SPEED_ACTIVE ) &&
				( gettime() > results[i].speed_start_time + results->conf->search_timeout ) )
			{
				pthread_cancel( *results[i].speed_thread );
				results[i].speed = SPEED_DONE;
				running --;
				done ++;
				break;
			}
			else if( results[i].speed > 0 && results[i].speed_start_time )
			{
				results[i].speed_start_time = 0;
				running --;
				correct ++;
				done ++;
				break;
			}
			else if( results[i].speed == SPEED_ERROR )
			{
				results[i].speed = SPEED_DONE;
				running --;
				done ++;
				break;
			}
		}
		if( i == count )
		{
			usleep( 100000 );
		}
	}

	return( correct );
}

void *search_speedtest( void *r )
{
	search_t *results = r;
	conn_t conn[1];
	int oldstate;
	
	/* Allow this thread to be killed at any time.			*/
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &oldstate );
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate );
	
	memset( conn, 0, sizeof( conn_t ) );
	conn->conf = results->conf;
	if( !conn_set( conn, results->url ) )
		results->speed = SPEED_ERROR;
	else if( !conn_init( conn ) )
		results->speed = SPEED_ERROR;
	else if( !conn_info( conn ) )
		results->speed = SPEED_ERROR;
	else if( conn->size == results->size )
		/* Add one because it mustn't be zero			*/
		results->speed = 1 + 1000 * ( gettime() - results->speed_start_time );
	else
		results->speed = SPEED_ERROR;

	conn_disconnect( conn );
	
	return( NULL );
}

char *strrstr( char *haystack, char *needle )
{
	int i, j;
	
	for( i = strlen( haystack ) - strlen( needle ); i > 0; i -- )
	{
		for( j = 0; needle[j] && haystack[i+j] == needle[j]; j ++ );
		if( !needle[j] )
			return( haystack + i );
	}
	
	return( NULL );
}

void search_sortlist( search_t *results, int count )
{
	qsort( results, count, sizeof( search_t ), search_sortlist_qsort );
}

int search_sortlist_qsort( const void *a, const void *b )
{
	if( ((search_t *)a)->speed < 0 && ((search_t *)b)->speed > 0 )
		return( 1 );
	if( ((search_t *)a)->speed > 0 && ((search_t *)b)->speed < 0 )
		return( -1 );
	if( ((search_t *)a)->speed < ((search_t *)b)->speed )
		return( -1 );
	else
		return( ((search_t *)a)->speed > ((search_t *)b)->speed );
}
