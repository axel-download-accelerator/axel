/* Generic protocol functions						*/

#include "axel.h"

/**
* @return The default port for the specified protocol or 0 if the protocol is not recognized
*/
int proto_defport(int protoid) {
	switch (protoid) {
	case PROTO_HTTP:
		return 80;
	#ifdef FTP
	case PROTO_FTP:
		return 21;
	#endif
	#ifdef SSL
		#ifdef FTP
		case PROTO_FTPS:
			return 990;
		#endif
		
		case PROTO_HTTPS:
			return 443;
	#endif
	default:
		return 0; /* Error code */
	}
}

/**
* @param protostr The protocol name or NULL for none.
* @return The protocol id of the protocol name specified in protostr. PROTO_ERR if the protocol is not recognized.
*/
int proto_getid(const char* protostr) {
	if (protostr == NULL) {
		return PROTO_DEFAULT;
	}
		
	if (strcmp(protostr, PROTO_HTTP_NAME) == 0) {
		return PROTO_HTTP;
	}
	#ifdef FTP
	else if (strcmp(protostr, PROTO_FTP_NAME) == 0) {
		return PROTO_FTP;
	}
	#endif
	#ifdef SSL
		else if (strcmp(protostr, PROTO_HTTPS_NAME) == 0) {
			return PROTO_HTTPS;
		}
		#ifdef FTP
		else if (strcmp(protostr, PROTO_FTPS_NAME) == 0) {
			return PROTO_FTPS;
		}
		#endif
	#endif
	else {
		return PROTO_ERR;
	}
}

/**
* @return The name of the protocol specified by protoid. Must NOT be freed. NULL if the protocol is invalid.
*/
const char* proto_getname(int protoid) {
	switch (protoid) {
	case PROTO_HTTP:
		return PROTO_HTTP_NAME;
	
	#ifdef FTP
	case PROTO_FTP:
		return PROTO_FTP_NAME;
	#endif
	
	#ifdef SSL
		case PROTO_HTTPS:
			return PROTO_HTTPS_NAME;
		
		#ifdef FTP
		case PROTO_FTPS:
			return PROTO_FTPS_NAME;
		#endif
	#endif
	default:
		return NULL;
	}
}
