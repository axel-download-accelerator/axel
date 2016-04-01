/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2008      Philipp Hagemeister
  Copyright 2008      Y Giridhar Appaji Nag

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

/* Configuration handling include file */

typedef struct
{
	char default_filename[MAX_STRING];
	char http_proxy[MAX_STRING];
	char no_proxy[MAX_STRING];
	int strip_cgi_parameters;
	int save_state_interval;
	int connection_timeout;
	int reconnect_delay;
	int num_connections;
	int buffer_size;
	int max_speed;
	int verbose;
	int alternate_output;
	int insecure;

	if_t *interfaces;

	int search_timeout;
	int search_threads;
	int search_amount;
	int search_top;

	int add_header_count;
	char add_header[MAX_ADD_HEADERS][MAX_STRING];

	char user_agent[MAX_STRING];
} conf_t;

int conf_loadfile( conf_t *conf, char *file );
int conf_init( conf_t *conf );
