/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2019      David da Silva Polverari

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

/* .netrc parsing implementation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "axel.h"
#include "netrc.h"

enum lookup_state {
	NOTHING,
	DEFAULT,
	MACHINE,
	HOST_ENTRY,
	LOGIN,
	PASSWORD
};

void
netrc_parse(conn_t *conn)
{
	FILE *fp;
	bool found = false;
	char *home;
	char *tok, *ntok;
	char line[MAX_STRING];
	char const *delims = " \t\n";
	enum lookup_state state = NOTHING;

	if (conn->conf->netrc_filename[0] == '\0') {
		home = getenv("HOME");
		snprintf(conn->conf->netrc_filename, MAX_STRING, "%s/.netrc", home);
	}
	if ((fp = fopen(conn->conf->netrc_filename, "r")) == NULL)
		return;
	while (!found && fgets(line, MAX_STRING, fp)) {
		tok = strtok_r(line, delims, &ntok);
		if (tok && *tok == '#')
			continue;
		while (tok != NULL) {
			switch (state) {
			case NOTHING:
				if (!strcmp("machine", tok)) {
					state = MACHINE;
				} else if (!strcmp("default", tok)) {
					state = DEFAULT;
				}
				break;
			case MACHINE:
				if (!strcmp(conn->host, tok)) {
					state = HOST_ENTRY;
				} else {
					state = NOTHING;
				}
				break;
			case DEFAULT:
				state = HOST_ENTRY;
				// fall through
			case HOST_ENTRY:
				if (!strcmp("login", tok)) {
					state = LOGIN;
				} else if (!strcmp("password", tok)) {
					state = PASSWORD;
				}
				break;
			case LOGIN:
				strcpy(conn->user, tok);
				state = HOST_ENTRY;
				break;
			case PASSWORD:
				strcpy(conn->pass, tok);
				found = true;
				break;
			default:
				break;
			}
			tok = strtok_r(NULL, delims, &ntok);
		}
	}
}
