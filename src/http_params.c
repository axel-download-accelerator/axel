/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2026 SiYu Liu <2028566723@qq.com>

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 USA.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* HTTP response header parameter parsing. */

#include "config.h"

#include "http_params.h"

#include <string.h>
#include <strings.h>

static const char *
skip_whitespace(const char *p, const char *end)
{
	while (p < end && (*p == ' ' || *p == '\t'))
		p++;

	return p;
}

bool
http_find_parameter(const char *header, const char *name,
		    struct http_parameter *parameter)
{
	const char *end;
	const char *p;
	size_t wanted_length;

	if (!header || !name || !*name || !parameter) {
		return false;
	}

	end = header + strcspn(header, "\r\n");
	wanted_length = strlen(name);
	p = header;

	// NOTE: skip attachment
	while (p < end && *p != ';') {
		p++;
	}

	while (p < end) {
		const char *name_start;
		const char *name_end;
		const char *value_start;
		const char *value_end;
		bool quoted = false;
		bool valid = true;

		p++;		// skip ';'
		p = skip_whitespace(p, end);
		name_start = p;

		while (p < end && *p != '=' && *p != ';')
			p++;

		name_end = p;
		while (name_end > name_start &&
		       (name_end[-1] == ' ' || name_end[-1] == '\t')) {
			name_end--;
		}

		if (p == end)
			break;
		if (*p == ';')
			continue;

		p++;
		p = skip_whitespace(p, end);

		if (p < end && *p == '"') {
			quoted = true;
			p++;
			value_start = p;

			while (p < end && *p != '"') {
				if (*p == '\\') {
					if (p + 1 == end) {
						valid = false;
						p = end;
						break;
					}
					p += 2;
				} else {
					p++;
				}
			}

			value_end = p;
			if (p == end) {
				valid = false;
			} else {
				p++;
				p = skip_whitespace(p, end);

				if (p < end && *p != ';') {
					valid = false;
					while (p < end && *p != ';') {
						p++;
					}
				}
			}
		} else {
			value_start = p;
			while (p < end && *p != ';')
				p++;

			value_end = p;
			while (value_end > value_start &&
			       (value_end[-1] == ' '
				|| value_end[-1] == '\t')) {
				value_end--;
			}
		}

		if (valid && (size_t) (name_end - name_start) == wanted_length
		    && strncasecmp(name_start, name, wanted_length) == 0) {
			parameter->value = value_start;
			parameter->length = (size_t) (value_end - value_start);
			parameter->quoted = quoted;
			return true;
		}
	}

	return false;
}

bool
http_copy_parameter(char *dest, size_t size,
		    const struct http_parameter *parameter)
{
	size_t output_length = 0;
	size_t i;
	size_t j;

	if (!dest || !size || !parameter || !parameter->value)
		return false;

	for (i = 0; i < parameter->length; i++) {
		if (parameter->quoted && parameter->value[i] == '\\') {
			i++;
			if (i == parameter->length)
				return false;
		}

		output_length++;
	}

	if (output_length >= size)
		return false;

	for (i = 0, j = 0; i < parameter->length; i++) {
		if (parameter->quoted && parameter->value[i] == '\\')
			i++;

		dest[j] = parameter->value[i];
		j++;
	}
	dest[j] = '\0';

	return true;
}
