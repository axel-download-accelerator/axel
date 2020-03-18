/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2016      Sjjad Hashemian
  Copyright 2016-2017 Stephen Thirlwall
  Copyright 2017      Antonio Quartulli
  Copyright 2012      iSEC Partners (SSL hostname validation)
  Copyright 2017-2019 Ismael Luceno

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

/* SSL interface */

#include "axel.h"

#ifdef HAVE_SSL

#include <openssl/err.h>
#include <openssl/x509v3.h>

typedef enum {
	MatchFound,
	MatchNotFound,
	NoSANPresent,
	MalformedCertificate,
	Error
} validate_result;

validate_result
validate_hostname(const char *hostname, const X509 *server_cert);

static pthread_mutex_t ssl_lock;
static bool ssl_inited = false;
static conf_t *conf = NULL;

void
ssl_init(conf_t *global_conf)
{
	pthread_mutex_init(&ssl_lock, NULL);
	conf = global_conf;
}

static
void
ssl_startup(void)
{
	pthread_mutex_lock(&ssl_lock);
	if (!ssl_inited) {
		SSL_library_init();
		SSL_load_error_strings();

		ssl_inited = true;
	}
	pthread_mutex_unlock(&ssl_lock);
}

static validate_result
matches_common_name(const char *hostname, const X509 *server_cert)
{
	int common_name_loc = -1;
	X509_NAME_ENTRY *common_name_entry = NULL;
	ASN1_STRING *common_name_asn1 = NULL;
	char *common_name_str = NULL;

	// Find the position of the CN field in the Subject field of the certificate
	common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name((X509 *) server_cert), NID_commonName, -1);
	if (common_name_loc < 0) {
		return Error;
	}

	// Extract the CN field
	common_name_entry = X509_NAME_get_entry(X509_get_subject_name((X509 *) server_cert), common_name_loc);
	if (common_name_entry == NULL) {
		return Error;
	}

	// Convert the CN field to a C string
	common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
	if (common_name_asn1 == NULL) {
		return Error;
	}
	#if OPENSSL_VERSION_NUMBER < 0x10101000L
		common_name_str = (char *) ASN1_STRING_data(common_name_asn1);
	#else
		common_name_str = (char *) ASN1_STRING_get0_data(common_name_asn1);
	#endif

	// Make sure there isn't an embedded NUL character in the CN
	if ((size_t) ASN1_STRING_length(common_name_asn1) != strlen(common_name_str)) {
		return MalformedCertificate;
	}

	// Compare expected hostname with the CN
	if (strcasecmp(hostname, common_name_str) == 0) {
		return MatchFound;
	} else {
		return MatchNotFound;
	}
}

static validate_result
matches_subject_alternative_name(const char *hostname, const X509 *server_cert)
{
	validate_result result = MatchNotFound;
	int i;
	int san_names_nb = -1;
	STACK_OF(GENERAL_NAME) *san_names = NULL;

	// Try to extract the names within the SAN extension from the certificate
	san_names = X509_get_ext_d2i((X509 *) server_cert, NID_subject_alt_name, NULL, NULL);
	if (san_names == NULL) {
		return NoSANPresent;
	}
	san_names_nb = sk_GENERAL_NAME_num(san_names);

	// Check each name within the extension
	for (i = 0; i < san_names_nb; i++) {
		const GENERAL_NAME *current_name = sk_GENERAL_NAME_value(san_names, i);

		if (current_name->type == GEN_DNS) {
			// Current name is a DNS name, let's check it
#if OPENSSL_VERSION_NUMBER < 0x10101000L
			char *dns_name = (char *) ASN1_STRING_data(current_name->d.dNSName);
#else
			char *dns_name = (char *) ASN1_STRING_get0_data(current_name->d.dNSName);
#endif
			// Make sure there isn't an embedded NUL character in the DNS name
			if ((size_t) ASN1_STRING_length(current_name->d.dNSName) != strlen(dns_name)) {
				result = MalformedCertificate;
				break;
			} else {
				// Compare expected hostname with the DNS name
				if (strcasecmp(hostname, dns_name) == 0) {
					result = MatchFound;
					break;
				}
			}
		}
	}
	sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);

	return result;
}


validate_result
validate_hostname(const char *hostname, const X509 *server_cert)
{
	validate_result result;

	if ((hostname == NULL) || (server_cert == NULL)) {
		return Error;
	}

	// First try the Subject Alternative Names extension
	result = matches_subject_alternative_name(hostname, server_cert);
	if (result == NoSANPresent) {
		// Extension was not found: try the Common Name
		result = matches_common_name(hostname, server_cert);
	}

	return result;
}

SSL *
ssl_connect(int fd, char *hostname)
{
	X509 *server_cert;
	SSL_CTX *ssl_ctx;
	SSL *ssl;

	ssl_startup();

	ssl_ctx = SSL_CTX_new(SSLv23_client_method());
	if (!conf->insecure) {
		SSL_CTX_set_default_verify_paths(ssl_ctx);
		SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, NULL);
	}
	SSL_CTX_set_mode(ssl_ctx, SSL_MODE_AUTO_RETRY);

	ssl = SSL_new(ssl_ctx);
	SSL_set_fd(ssl, fd);
	SSL_set_tlsext_host_name(ssl, hostname);

	int err = SSL_connect(ssl);
	if (err <= 0) {
		fprintf(stderr, _("SSL error: %s\n"),
			ERR_reason_error_string(ERR_get_error()));
		SSL_CTX_free(ssl_ctx);
		return NULL;
	}

	err = SSL_get_verify_result(ssl);
	if (err != X509_V_OK) {
		fprintf(stderr, _("SSL error: Certificate error"));
		SSL_CTX_free(ssl_ctx);
		return NULL;
	}

	server_cert =  SSL_get_peer_certificate(ssl);
	if (server_cert == NULL) {
		fprintf(stderr, _("SSL error: Certificate not found"));
		SSL_CTX_free(ssl_ctx);
		return NULL;
	}

	if (validate_hostname(hostname, server_cert) != MatchFound) {
		fprintf(stderr, _("SSL error: Hostname verification failed"));
		X509_free(server_cert);
		SSL_CTX_free(ssl_ctx);
		return NULL;
	}

	X509_free(server_cert);

	return ssl;
}

void
ssl_disconnect(SSL *ssl)
{
	SSL_shutdown(ssl);
	SSL_free(ssl);
}

#endif				/* HAVE_SSL */
