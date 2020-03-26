#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include "compat-ssl.h"

const unsigned char *
ASN1_STRING_get0_data(const ASN1_STRING *x)
{
	return ASN1_STRING_data((ASN1_STRING *)x);
}
