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

/* Separator of URL scheme and host. Note that this is only correct for all URI schemes we support as the official separator is just a colon. */
#define URL_SCHEME_SEP "://"
#define URL_USER_SEPCHAR '@'
#define URL_PASSWORD_SEPCHAR ':'
#define URL_PORT_SEPCHAR ':'
#define URL_DIR_SEPCHAR '/'
#define URL_QUERY_SEPCHAR '?'

// All string fields are stored unencoded
typedef struct {
	/* Protocol id as defined in proto.h */
	int proto;
	char* host;
	unsigned int port;
	char* dir; // The directory of the request, excluding the first and last slash. NULL for the top directory
	char* filename; // The file name after the directory, NULL for none
	char* query; // Query and fragment of the URL without the initiating question mark, NULL for none
	char* user; // NULL if no user specified (set anonymous if user is necessary)
	char* pass; // NULL if no password specified (set to any value if required)
} url_t;

char* url_encode(const char* origurl);
char* url_heuristic_decode(const char * urlstr)

url_t* url_parse_heuristic(const char* urlstr);
url_t* url_parse_unencoded(const char* urlstr);

char* url_str(const url_t* u);
char* url_request(const url_t* u);
char* url_str_encoded(const url_t* u);
char* url_request_encoded(const url_t* u);

void url_free(url_t* u);
