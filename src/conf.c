/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2008      Philipp Hagemeister
  Copyright 2008      Y Giridhar Appaji Nag
  Copyright 2016      Ivan Gimenez
  Copyright 2016      Stephen Thirlwall
  Copyright 2017      Ismael Luceno

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

/* Configuration handling file */

#include "axel.h"

/* Some nifty macro's.. */
#define MATCH if( 0 );
#define KEY( name )				\
	else if( !strcmp( key, #name ) )	\
		dst = &conf->name;

int parse_interfaces( conf_t *conf, char *s );

static int axel_fscanf( FILE *fp, const char *format, ...)
{
	va_list params;
	int ret;

	va_start( params, format );
	ret = vfscanf( fp, format, params );
	va_end( params );

	ret = !( ret == EOF  && ferror(fp) );
	if( !ret )
	{
		fprintf( stderr, _("I/O error while reading config file: %s\n"),
			 strerror(errno) );
	}

	return( ret );
}

static int parse_protocol( conf_t *conf, const char *value )
{
	if( strcasecmp( value, "ipv4" ) == 0 )
		conf->ai_family = AF_INET;
	else if( strcasecmp( value, "ipv6" ) == 0 )
		conf->ai_family = AF_INET6;
	else
	{
		fprintf( stderr, _( "Unknown protocol %s\n" ), value );
		return( 0 );
	}

	return( 1 );
}

int conf_loadfile( conf_t *conf, char *file )
{
	int line = 0, ret = 1;
	FILE *fp;
	char s[MAX_STRING], key[MAX_STRING];

	fp = fopen( file, "r" );
	if( fp == NULL )
		return( 1 );			/* Not a real failure */

	while( !feof( fp ) )
	{
		char *tmp, *value = NULL;
		void *dst;

		line ++;

		*s = 0;

		if( !( ret = axel_fscanf( fp, "%100[^\n#]s", s ) ) )
			break;
		if( !( ret = axel_fscanf( fp, "%*[^\n]s" ) ) )
			break;
		fgetc( fp );			/* Skip newline */
		tmp = strchr( s, '=' );
		if( tmp == NULL )
			continue;		/* Probably empty? */

		sscanf( s, "%[^= \t]s", key );

		/* Skip the "=" and any spaces following it */
		while( isspace( * ++ tmp ) ); /* XXX isspace('\0') is false */
		value = tmp;
		/* Get to the end of the value string */
		while ( *tmp && !isspace( *tmp ) ) tmp++;
		*tmp = '\0';

		/* String options */
		MATCH
			KEY( default_filename )
			KEY( http_proxy )
			KEY( no_proxy )
			KEY( user_agent )
		else goto num_keys;

		/* Save string option */
		strcpy( dst, value );
		continue;

		/* Numeric options */
	num_keys:
		MATCH
			KEY( strip_cgi_parameters )
			KEY( save_state_interval )
			KEY( connection_timeout )
			KEY( reconnect_delay )
			KEY( num_connections )
			KEY( max_redirect )
			KEY( buffer_size )
			KEY( max_speed )
			KEY( verbose )
			KEY( alternate_output )
			KEY( insecure )
			KEY( no_clobber )
			KEY( search_timeout )
			KEY( search_threads )
			KEY( search_amount )
			KEY( search_top )
		else goto other_keys;

		/* Save numeric option */
		*( ( int * ) dst ) = atoi( value );
		continue;

	other_keys:
		/* Option defunct but shouldn't be an error */
		if( strcmp( key, "speed_type" ) == 0 )
			continue;
		else if( strcmp( key, "interfaces" ) == 0 )
		{
			if( parse_interfaces( conf, value ) )
				continue;
		}
		else if( strcmp( key, "use_protocol" ) == 0 )
		{
			if( parse_protocol( conf, value ) )
				continue;
		}

#if 0
		/* FIXME broken code */
		get_config_number( add_header_count );
		for(int i=0;i<conf->add_header_count;i++)
			get_config_string( add_header[i] );
#endif

		fprintf( stderr, _("Error in %s line %i.\n"), file, line );
		ret = 0;
		break;
	}

	fclose( fp );
	return( ret );
}

int conf_init( conf_t *conf )
{
	char *s2;
	int i;

	/* Set defaults */
	memset( conf, 0, sizeof( conf_t ) );
	strcpy( conf->default_filename, "default" );
	*conf->http_proxy		= 0;
	*conf->no_proxy			= 0;
	conf->strip_cgi_parameters	= 1;
	conf->save_state_interval	= 10;
	conf->connection_timeout	= 45;
	conf->reconnect_delay		= 20;
	conf->num_connections		= 4;
	conf->max_redirect		= MAX_REDIRECT;
	conf->buffer_size		= 5120;
	conf->max_speed			= 0;
	conf->verbose			= 1;
	conf->alternate_output		= 0;
	conf->insecure			= 0;
	conf->no_clobber		= 0;

	conf->search_timeout		= 10;
	conf->search_threads		= 3;
	conf->search_amount		= 15;
	conf->search_top		= 3;
	conf->add_header_count		= 0;

	conf->ai_family			= AF_UNSPEC;

	strncpy( conf->user_agent, DEFAULT_USER_AGENT, MAX_STRING );

	conf->interfaces = malloc( sizeof( if_t ) );
	memset( conf->interfaces, 0, sizeof( if_t ) );
	conf->interfaces->next = conf->interfaces;

	if( ( s2 = getenv( "http_proxy" ) ) != NULL )
		strncpy( conf->http_proxy, s2, MAX_STRING );
	else if( ( s2 = getenv( "HTTP_PROXY" ) ) != NULL )
		strncpy( conf->http_proxy, s2, MAX_STRING );

	if( !conf_loadfile( conf, ETCDIR "/axelrc" ) )
		return( 0 );

	if( ( s2 = getenv( "HOME" ) ) != NULL )
	{
		char s[MAX_STRING];
		sprintf( s, "%s/%s", s2, ".axelrc" );
		if( !conf_loadfile( conf, s ) )
			return( 0 );
	}

	/* Convert no_proxy to a 0-separated-and-00-terminated list.. */
	for( i = 0; conf->no_proxy[i]; i ++ )
		if( conf->no_proxy[i] == ',' )
			conf->no_proxy[i] = 0;
	conf->no_proxy[i+1] = 0;

	return( 1 );
}

/* release resources allocated by conf_init() */
void conf_free( conf_t *conf )
{
	free( conf->interfaces );
}

int parse_interfaces( conf_t *conf, char *s )
{
	char *s2;
	if_t *iface;

	iface = conf->interfaces->next;
	while( iface != conf->interfaces )
	{
		if_t *i;

		i = iface->next;
		free( iface );
		iface = i;
	}
	free( conf->interfaces );

	if( !*s )
	{
		conf->interfaces = malloc( sizeof( if_t ) );
		memset( conf->interfaces, 0, sizeof( if_t ) );
		conf->interfaces->next = conf->interfaces;
		return( 1 );
	}

	s[strlen(s)+1] = 0;
	conf->interfaces = iface = malloc( sizeof( if_t ) );
	while( 1 )
	{
		while( ( *s == ' ' || *s == '\t' ) && *s ) s ++;
		for( s2 = s; *s2 != ' ' && *s2 != '\t' && *s2; s2 ++ );
		*s2 = 0;
		if( *s < '0' || *s > '9' )
			get_if_ip( s, iface->text );
		else
			strcpy( iface->text, s );
		s = s2 + 1;
		if( *s )
		{
			iface->next = malloc( sizeof( if_t ) );
			iface = iface->next;
		}
		else
		{
			iface->next = conf->interfaces;
			break;
		}
	}

	return( 1 );
}
