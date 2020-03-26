/*
  Helper functions to perform basic hostname validation using OpenSSL.

  Author:  Alban Diquet
  Copyright (C) 2012, iSEC Partners.

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
  of the Software, and to permit persons to whom the Software is furnished to do
  so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 */

#include "axel.h"

#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include <ctype.h>

#if OPENSSL_VERSION_NUMBER < 0x10101000L
#define ASN1_STRING_data_compat ASN1_STRING_data
#else
#define ASN1_STRING_data_compat ASN1_STRING_get0_data
#endif

typedef enum {
	MatchFound,
	MatchNotFound,
	NoSANPresent,
	MalformedCertificate,
	Error
} validate_result;

static bool
memeq_ncase(const char *a, const char *b, size_t length)
{
	if (length == 0) {
		return true;
	}

	do {
		if (tolower(*a++) != tolower(*b++)) {
			return false;
		}
	} while (--length != 0);
	return true;
}

static bool
contains_nul(const char *str, size_t length)
{
	if (length == 0) {
		return false;
	}

	do {
		if (*str++ == '\0') {
			return true;
		}
	} while (--length != 0);
	return false;
}

static validate_result
ssl_matches_name(const char *hostname, ASN1_STRING *certname_asn1)
{
	char *certname_str = NULL;
	int certname_len = 0;
	int hostname_len = 0;

	certname_str = (char *) ASN1_STRING_data_compat(certname_asn1);
	certname_len = ASN1_STRING_length(certname_asn1);
	hostname_len = strlen(hostname);

	if (certname_len < 0 || hostname_len < 0) {
		return MalformedCertificate;
	}

	// Make sure there isn't an embedded NUL character in the DNS name
	if (contains_nul(certname_str, certname_len)) {
		return MalformedCertificate;
	}

	// Remove last '.' from hostname
	if (hostname_len != 0 && hostname[hostname_len - 1] == '.') {
		--hostname_len;
	}

	// Skip the first segment if wildcard
	if (certname_len > 2 && certname_str[0] == '*' && certname_str[1] == '.') {
		if (hostname_len != 0) {
			do {
				--hostname_len;
				if (*hostname++ == '.') {
					break;
				}
			} while (hostname_len != 0);
		}
		certname_str += 2;
		certname_len -= 2;
	}
	// Compare expected hostname with the DNS name
	if (certname_len != hostname_len) {
		return MatchNotFound;
	}

	return memeq_ncase(hostname, certname_str, hostname_len) ? MatchFound : MatchNotFound;
}

static validate_result
ssl_matches_common_name(const char *hostname, const X509 *server_cert)
{
	int common_name_loc = -1;
	X509_NAME_ENTRY *common_name_entry = NULL;
	ASN1_STRING *common_name_asn1 = NULL;

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

	return ssl_matches_name(hostname, common_name_asn1);
}

static validate_result
ssl_matches_subject_alternative_name(const char *hostname, const X509 *server_cert)
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
			result = ssl_matches_name(hostname, current_name->d.dNSName);
			if (result != MatchNotFound) {
				break;
			}
		}
	}
	sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);

	return result;
}

bool
ssl_validate_hostname(const char *hostname, const X509 *server_cert)
{
	validate_result result;

	if ((hostname == NULL) || (server_cert == NULL)) {
		return false;
	}

	// First try the Subject Alternative Names extension
	result = ssl_matches_subject_alternative_name(hostname, server_cert);
	if (result == NoSANPresent) {
		// Extension was not found: try the Common Name
		result = ssl_matches_common_name(hostname, server_cert);
	}

	return result == MatchFound;
}
