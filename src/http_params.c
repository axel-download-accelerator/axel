/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2008      Y Giridhar Appaji Nag
  Copyright 2008-2009 Philipp Hagemeister
  Copyright 2015      Joao Eriberto Mota Filho
  Copyright 2016      Ivan Gimenez
  Copyright 2016      Phillip Berndt
  Copyright 2016      Sjjad Hashemian
  Copyright 2016      Stephen Thirlwall
  Copyright 2017      Antonio Quartulli
  Copyright 2017      David Polverari
  Copyright 2017-2019 Ismael Luceno
  Copyright 2018-2019 Shankar

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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* HTTP header parameter parsing */

#include "config.h"
#include "http_params.h"

#include <string.h>
#include <strings.h>

/*
 * Consume one parameter value, starting at *pp (just past '=').  A value may
 * be quoted ('"' or '\'') and quoted values may use backslash escapes.  The
 * decoded (unquoted) value is copied into dest, NUL-terminated and limited to
 * length bytes, when dest is non-NULL; otherwise it is only skipped.  On
 * return *pp points just past the value.
 */
static void
http_param_value(const char **pp, const char *end, char *dest, size_t length)
{
	const char *p = *pp;
	const char quote = (*p == '"' || *p == '\'') ? *p++ : 0;
	char *out = dest;
	char *out_end = dest ? dest + length - 1 : NULL;

	while (p < end) {
		if (quote) {
			if (*p == quote) {	/* closing quote */
				p++;
				break;
			}
			if (*p == '\\' && p + 1 < end) {	/* escaped char */
				if (out && out < out_end)
					*out++ = p[1];
				p += 2;
				continue;
			}
		} else if (*p == ';') {	/* next parameter */
			break;
		}

		if (out && out < out_end)
			*out++ = *p;
		p++;
	}

	if (out)
		*out = 0;
	*pp = p;
}

/*
 * Parse one header value of the form "<main> [; <name>=<value> ...]".
 * Copy <main> (up to the first ';', trimmed) into main_val when non-NULL.
 * Then, for each parameter whose name matches an entry of params, copy its
 * decoded value into that entry's dest.  Stops at the end of the line.
 */
void
http_header_params(const char *h, char *main_val, size_t main_len,
		   struct header_params *params, size_t nparams)
{
	const char *end = h + strcspn(h, "\n");
	const char *p = h;

	if (main_val && main_len) {
		const char *m = p + strspn(p, " \t");	/* skip leading space */
		size_t len = strcspn(m, ";");

		if (m + len > end)
			len = end - m;
		while (len && (m[len - 1] == ' ' || m[len - 1] == '\t'))
			len--;
		if (len >= main_len)
			len = main_len - 1;
		memcpy(main_val, m, len);
		main_val[len] = 0;
	}
	p += strcspn(p, ";");

	while (p < end && *p == ';') {
		p++;				/* skip ';' */
		p += strspn(p, " \t");		/* skip whitespace */

		const char *name = p;
		p += strcspn(p, "=;\n");
		const char *name_end = p;

		while (name_end > name &&
		       (name_end[-1] == ' ' || name_end[-1] == '\t'))
			name_end--;		/* trim trailing whitespace */

		if (p < end && *p == '=') {	/* has a value */
			p++;
			p += strspn(p, " \t");

			char *dest = NULL;
			size_t length = 0;
			const size_t name_len = (size_t)(name_end - name);

			for (size_t i = 0; i < nparams; i++)
				if (strlen(params[i].name) == name_len &&
				    strncasecmp(params[i].name, name,
						name_len) == 0) {
					dest = params[i].dest;
					length = params[i].length;
					break;
				}

			http_param_value(&p, end, dest, length);
		}
		/* else: a bare parameter (no '=') or the end of the line; the
		   loop's `*p == ';'` test skips it / exits. */
	}
}
