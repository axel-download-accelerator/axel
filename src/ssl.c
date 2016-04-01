/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2008      Y Giridhar Appaji Nag
  Copyright 2008-2009 Philipp Hagemeister
  Copyright 2015      Joao Eriberto Mota Filho
  Copyright 2016      Ivan Gimenez

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

/* SSL interface */

#include "axel.h"

#include <openssl/err.h>

static SSL_CTX *ssl_ctx = NULL;
static conf_t *conf = NULL;

void ssl_init( conf_t *global_conf )
{
	conf = global_conf;
}

void ssl_startup( void )
{
	if( ssl_ctx != NULL )
		return;

	SSL_library_init();
	SSL_load_error_strings();

	ssl_ctx = SSL_CTX_new( SSLv23_client_method() );
	if( !conf->insecure ) {
		SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, NULL);
		SSL_CTX_set_verify_depth(ssl_ctx, 0);
	}
}

SSL* ssl_connect( int fd, char *message )
{
	SSL* ssl;

	ssl_startup();

	ssl = SSL_new( ssl_ctx );
	SSL_set_fd( ssl, fd );

	int err = SSL_connect( ssl );
	if( err <= 0 ) {
		sprintf(message, _("SSL error: %s\n"), ERR_reason_error_string(ERR_get_error()));
		return NULL;
	}

	return ssl;
}

void ssl_disconnect( SSL *ssl )
{
	SSL_shutdown( ssl );
	SSL_free( ssl );
}
