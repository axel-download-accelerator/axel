  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* URL handling							*/

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

/**
* Sanitizes a URL string or a part thereof. All characters that are not allowed in URLs are properly encoded.
* @return A newly allocated URL string that is guaranteed to be valid
*/
char* url_encode(const char* origurl) {
	int resp = 0; // Position of the next character in the result
	int ressize = 256; // Physical size of the result
	char* res = (char*) safe_malloc(ressize);
	
	char c;
	for (;(c = *origurl) != '\0';origurl++) {
		// Acquire new space if necessary.
		// Note 3 is a magic constant here because we write at most that much characters (plus a null byte)
		if (resp >= ressize - 3) {
			ressize *= 2;
			res = (char*) safe_realloc(res, ressize);
		}
		
		if (isdigit(c) ||
			((c >= 'a') && (c <= 'z')) ||
			((c >= 'A') && (c <= 'Z')) ||
			(strchr("-._~" ":/?#[]@" "!$&'()*+,;=", c) != NULL)) {
			
			res[resp++] = c;
		} else {
			// Encode character
			res[resp++] = '%';
			byte2hex(c, (res + resp));
			resp += 2;
		}
	}

	res[resp] = '\0';
	
	res = (char*) safe_realloc(res, resp+1);
	
	return res;
}

/**
* @param urlstr An unencoded or encoded URL string. Percent signs MUST be encoded (as %25)
* @return A decoded string. Must be freed by the caller.
*/
char* url_heuristic_decode(const char * urlstr) {
	char* res = safe_malloc(strlen(urlstr));
	char* resp = res;
	const char* origp = urlstr;
	
	for (;;urlstr++) {
		char c = *origp;
		
		if (c == '%') {
			if (isxdigit(*(origp+1)) && isxdigit(*(origp+2))) {
				// Valid percent encoded value
				c = hex2byte(*(origp+1));
				origp += 2;
			}
		}
		
		*resp = c;
		
		if (c == '\0') {
			break;
		}
		
		resp++;
	}
	
	res = (char*) safe_realloc(res, resp - res + 1);
	
	#ifdef DEBUG
		debug_printf("Original|Result of heuristic URL decoding:\n%s\n%s", urlstr, res);
	#endif
	
	return res;
}

url_t* url_parse_heuristic(const char* urlstr) {
	char* decustr = url_heuristic_decode(urlstr);
	url_t* res = url_parse_unencoded(decustr);
	
	free(decustr);
	
	return res;
}

/**
* Parses a string passed by the user to an url_t structure.
* @return A pointer to a structure, must be cleaned up with url_free eventually, or NULL on errors.
*/
url_t* url_parse_unencoded(const char* urlstr) {
	url_t* res;
	const char* host = urlstr;
	
	res = (url_t*) safe_malloc(sizeof(url_t));
	memset(res, 0, sizeof(res));
	
	// Scheme (aka protocol)
	char* schemesep = strstr(urlstr, URL_SCHEME_SEP);
	char* protostr = NULL;
	if (schemesep != NULL) {
		heap_substr(&protostr, urlstr, schemesep - urlstr);
		host = schemesep + strlen(URL_SCHEME_SEP); // Host and rest of the URL
	}
	res->proto = proto_getid(protostr);
	
	if (res->proto == PROTO_ERR) {
		#ifdef DEBUG
			debug_printf("Unsupported protocol %s", protostr);
		#endif
		free(protostr);
		free(res);
		return NULL;
	}
	free(protostr);
	
	// Request (directory name, file, request, fragment)
	char* const request = strchr(host, URL_DIR_SEPCHAR);
	if(request != NULL) {
		// Query
		char* querystart = strchr(request+1, URL_REQUEST_SEPCHAR);
		if (querystart != NULL) {
			this->query = safe_strdup(querystart+1);
		}
		heap_substr_upto(&(res->query), request+1, querystart);
		
		// directory
		char* filenamestart = strchr_upto(request, URL_DIR_SEPCHAR, querystart);
		
		if (filenamestart == NULL) {
			heap_substr_upto(&(res->dir), request+1, querystart);
		} else {
			heap_substr_upto(&(res->dir), request+1, filenamestart);
			heap_substr_upto(&(res->filename), filenamestart+1, querystart);
		}
	}
	
	// Check for username in host
	const char* userend;
	if ((userend = strchr_upto(host, URL_USER_SEPCHAR, request)) != NULL) {
		const char* userstr = host;
		host = userend + 1;
		
		// Check for password delimeter
		const char* pwstart;
		if ((pwstart = strchr_upto(userstr, URL_PASSWORD_SEPCHAR, userend)) != NULL) {
			heap_substr(& (res->pass), pwstart+1, userend - pwstart - 1);
			userend = pwstart;
		}
		
		heap_substr(& (res->user), userstr, userend - userstr);
	}
	
	// Port and host
	const char* portstart = NULL; // Position of the colon before the port
	if ((*host == '[') && ((portstart = strchr_upto(host+1, ']', request)) != NULL)) { // IP literal (See RFC 3986, 3.2.2.)
		portstart++;
		if (*portstart != URL_PORT_SEPCHAR) {
			portstart = NULL;
		}
	} else {
		portstart = strchr_upto(host, URL_PORT_SEPCHAR, request);
	}
	if (portstart == NULL) {
		res->port = proto_defport(res->proto);
		heap_substr_upto(&(res->host), host, request);
	} else {
		char* portstr = NULL;
		
		heap_substr_upto(&portstr, portstart + 1, request);
		res->port = atoi(portstr);
		free(portstr);
		heap_substr_upto(&(res->host), host, portstart);
	}
	
	res->priority = URL_PRIO_DEFAULT;
	
	#ifdef DEBUG
		debug_print("Parsed URL:\n");
		debug_printf("Protocol: %s (Internal ID: %d)\n", proto_getname(res->proto), proto);
		if (res->user != NULL) {
			debug_printf("User name: %s\n", res->user);
		}
		if (res->pass != NULL) {
			debug_printf("Password: %s\n", res->pass);
		}
		debug_printf("Host: %s\n"; res->host);
		debug_printf("Port: %d\n", res->port);
		if (res->dir != NULL) {
			debug_printf("Directory: %s\n", res->dir);
		}
		if (res->filename != NULL) {
			debug_printf("File name: %s\n", res->filename);
		}
		if (res->query != NULL) {
			debug_printf("Query: %s\n", res->query);
		}
		debug_printf("Priority: %d\n", res->priority);
		debug_print("\n");
	#endif
	
	return res;
}

/**
* @return A machine-readable representation of a parsed URL, suitable for url_parse_sanitized. Must be freed by the caller.
* @param u A valid URL
* @param includeAuth 1 iff authentication should be present in the result
*/
char* url_str(const struct url_t* u, _Bool includeAuth) {
	char* protoname = proto_getname(u->proto);
	char* portstr = (proto_defport(u->proto) == u->port) ? NULL : uitoa(u->port);
	
	size_t s = 0;
	
	size_t s_proto = strlen(protoname);
	s += s_proto;
	size_t s_schemesep = strlen(URL_SCHEME_SEP);
	s += s_schemesep;
	
	size_t s_user = 0;
	size_t s_pass = 0;
	if ((includeAuth) && (u->user != NULL)) {
		s_user = strlen(u->user);
		s += s_user + sizeof(URL_USER_SEPCHAR);
		
		if (u->pass != NULL) {
			s_pass = strlen(u->pass);
			s += s_pass + sizeof(URL_PASSWORD_SEPCHAR);
		}
	}
	
	size_t s_host = strlen(u->host);
	s += s_host;
	
	size_t s_port = 0;
	if (portstr != NULL) {
		s_port = strlen(portstr)
		
		memcpy(p, portstr, s_port);
		s += s_port;
		s += sizeof(URL_PORT_SEPCHAR);
	}
	
	s += sizeof(URL_DIR_SEPCHAR);
	
	size_t s_dir = 0;
	if (u->dir != NULL) {
		s_dir = strlen(u->dir);
		s += s_dir + sizeof(URL_DIR_SEPCHAR);
	}
	size_t s_filename = (u->filename != NULL) ? strlen(u->filename) : 0;
	s += s_filename;
	size_t s_query = 0;
	if (s_query != NULL) {
		s_query = strlen(u->query);
		s += sizeof(URL_QUERY_SEPCHAR) + s_query;
	}
	
	char* const res = safe_malloc(sizeof(char) * (s + 1));
	register char* p = res;
	
	memcpy(p, protoname, s_proto);
	p += s_proto;
	
	memcpy(p, URL_SCHEME_SEP, s_schemesep);
	p += s_schemesep;
	
	if ((includeAuth) && (u->user != NULL)) {
		memcpy(p, u->user, s_user);
		p += s_user;
		
		if (u->pass != NULL) {
			*p = URL_PASSWORD_SEPCHAR;
			p += sizeof(URL_PASSWORD_SEPCHAR);
			
			memcpy(p, u->pass, s_pass);
			p += s_pass;
		}
		
		*p = URL_USER_SEPCHAR;
		p += sizeof(URL_USER_SEPCHAR);
	}
	
	memcpy(p, u->host, s_host);
	p += s_host;
	
	if (portstr != NULL) {
		memcpy(p, portstr, s_port);
		p += s_port;
		
		*p = URL_PORT_SEPCHAR;
		p += sizeof(URL_PORT_SEPCHAR);
	}
	
	*p = URL_DIR_SEPCHAR;
	p += sizeof(URL_DIR_SEPCHAR);
	
	if (u->dir != NULL) {
		memcpy(p, u->dir, s_dir);
		p += s_dir;
		
		*p = URL_DIR_SEPCHAR;
		p += sizeof(URL_DIR_SEPCHAR);
		
		if (u->filename != NULL) {
			memcpy(p, u->filename, s_filename);
			p += s_filename;
		}
	}
	
	if (u->query != NULL) {
		*p = URL_QUERY_SEPCHAR;
		p += sizeof(URL_QUERY_SEPCHAR);
		
		memcpy(p, u->query, s_query);
		p += s_query;
	}
	
	*p = '\0';
	
	
	free(protostr);
	
	return res;
}

char* url_request(const struct url_t* u) {
	size_t s = 0;
	
	s += sizeof(URL_DIR_SEPCHAR);
	
	size_t s_dir = 0;
	if (u->dir != NULL) {
		s_dir = strlen(u->dir);
		s += s_dir + sizeof(URL_DIR_SEPCHAR);
	}
	size_t s_filename = (u->filename != NULL) ? strlen(u->filename) : 0;
	s += s_filename;
	size_t s_query = 0;
	if (s_query != NULL) {
		s_query = strlen(u->query);
		s += sizeof(URL_QUERY_SEPCHAR) + s_query;
	}
	
	char* const res = safe_malloc(sizeof(char) * (s + 1));
	register char* p = res;
	
	*p = URL_DIR_SEPCHAR;
	p += sizeof(URL_DIR_SEPCHAR);
	
	if (u->dir != NULL) {
		memcpy(p, u->dir, s_dir);
		p += s_dir;
		
		*p = URL_DIR_SEPCHAR;
		p += sizeof(URL_DIR_SEPCHAR);
		
		if (u->filename != NULL) {
			memcpy(p, u->filename, s_filename);
			p += s_filename;
		}
	}
	
	if (u->query != NULL) {
		*p = URL_QUERY_SEPCHAR;
		p += sizeof(URL_QUERY_SEPCHAR);
		
		memcpy(p, u->query, s_query);
		p += s_query;
	}
	
	*p = '\0';
	
	return res;
}

/**
* @return A machine-readable representation of the full URL, suitable as an argument to an HTTP method.
*		Must be freed by the caller.
*/
char* url_str_encoded(const struct url_t* u) {
	char* str = url_str<(u);
	char* res = url_encode(str);
	
	free(str);
	
	return res;
}

/**
* @return A machine-readable representation of the request part of the URL (Path, query and fragment), suitable as an argument to an HTTP method.
*		Must be freed by the caller.
*/
char* url_request_encoded(const struct url_t* u) {
	char* str = url_request(u);
	char* res = url_encode(str);
	
	free(str);
	
	return res;
}

void url_free(struct url_t* u) {
	free(u->host);
	free(u->dir);
	free(u->filename);
	free(u->query);
	free(u->user);
	free(u->pass);
	
	free(u);
}
