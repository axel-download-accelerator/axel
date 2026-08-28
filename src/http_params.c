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

#include <stdbool.h>
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

// transfer a ASCII hex char to 0-15. return -1 if it is not a hex char
static int
hexadecimal_value(unsigned char c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('A' <= c && c <= 'F')
		return c - 'A' + 10;
	if ('a' <= c && c <= 'f')
		return c - 'a' + 10;

	return -1;
}

static bool
is_attribute_character(unsigned char c)
{
	if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ||
	    ('0' <= c && c <= '9'))
		return true;

	switch (c) {
	case '!':
	case '#':
	case '$':
	case '&':
	case '+':
	case '-':
	case '.':
	case '^':
	case '_':
	case '`':
	case '|':
	case '~':
		return true;
	default:
		return false;
	}
}

static bool
read_extended_byte(const char **cursor, const char *end, unsigned char *byte)
{
	const char *p = *cursor;

	if (p == end)
		return false;

	if (*p == '%') {
		int high;
		int low;

		if (end - p < 3)
			return false;

		high = hexadecimal_value((unsigned char) p[1]);
		low = hexadecimal_value((unsigned char) p[2]);
		if (high < 0 || low < 0)
			return false;

		*byte = (unsigned char) ((high << 4) | low);
		p += 3;
	} else {
		*byte = (unsigned char) *p;
		if (!is_attribute_character(*byte))
			return false;

		p++;
	}

	if (*byte == 0)
		return false;

	*cursor = p;
	return true;
}

// language can be empty
static bool
is_language(const char *start, const char *end)
{
	for (const char *p = start; p < end; p++) {
		unsigned char c = (unsigned char) *p;

		if (!(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ||
		      ('0' <= c && c <= '9') || c == '-'))
			return false;
	}

	return true;
}

static bool
read_continuation_byte(const char **cursor, const char *end,
		       unsigned char *byte, size_t *length)
{
	if (!read_extended_byte(cursor, end, byte) || (*byte & 0xc0) != 0x80)
		return false;

	(*length)++;
	return true;
}

static bool
validate_extended_utf8(const char *start, const char *end, size_t *length)
{
	const char *p = start;
	size_t decoded_length = 0;

	while (p < end) {
		unsigned char first;
		unsigned char second;
		unsigned char third;
		unsigned char fourth;

		if (!read_extended_byte(&p, end, &first))
			return false;
		decoded_length++;

		if (first <= 0x7f)
			continue;

		if (first >= 0xc2 && first <= 0xdf) {
			if (!read_continuation_byte
			    (&p, end, &second, &decoded_length))
				return false;
			continue;
		}

		if (first >= 0xe0 && first <= 0xef) {
			if (!read_continuation_byte
			    (&p, end, &second, &decoded_length)
			    || !read_continuation_byte(&p, end, &third,
						       &decoded_length))
				return false;

			if ((first == 0xe0 && second < 0xa0)
			    || (first == 0xed && second > 0x9f))
				return false;
			continue;
		}

		if (first >= 0xf0 && first <= 0xf4) {
			if (!read_continuation_byte
			    (&p, end, &second, &decoded_length)
			    || !read_continuation_byte(&p, end, &third,
						       &decoded_length)
			    || !read_continuation_byte(&p, end, &fourth,
						       &decoded_length))
				return false;

			if ((first == 0xf0 && second < 0x90)
			    || (first == 0xf4 && second > 0x8f))
				return false;
			continue;
		}

		return false;
	}

	*length = decoded_length;
	return true;
}

bool
http_decode_extended_parameter(char *dest, size_t size,
			       const struct http_parameter *parameter)
{
	const char *charset_end;
	const char *language_end;
	const char *value_start;
	const char *end;
	const char *p;
	size_t decoded_length;
	size_t output = 0;

	if (!dest || !size || !parameter || !parameter->value
	    || parameter->quoted)
		return false;

	end = parameter->value + parameter->length;
	charset_end = memchr(parameter->value, '\'', parameter->length);
	if (!charset_end)
		return false;

	language_end =
	    memchr(charset_end + 1, '\'', (size_t) (end - charset_end - 1));
	if (!language_end)
		return false;

	if ((size_t) (charset_end - parameter->value) != 5 ||
	    strncasecmp(parameter->value, "UTF-8", 5) != 0 ||
	    !is_language(charset_end + 1, language_end))
		return false;

	value_start = language_end + 1;
	if (!validate_extended_utf8(value_start, end, &decoded_length) ||
	    decoded_length >= size)
		return false;

	for (p = value_start; p < end;) {
		unsigned char byte;

		if (!read_extended_byte(&p, end, &byte))
			return false;
		dest[output++] = (char) byte;
	}
	dest[output] = '\0';

	return true;
}

static bool
sanitize_filename(char *filename)
{
	static const char invalid[] = "/\\?%*:|<>";

	char *end = filename + strlen(filename);

	while (end > filename && (end[-1] == ' ' || end[-1] == '\t'))
		*--end = '\0';

	for (unsigned char *p = (unsigned char *)filename; *p; p++) {
		if (*p < 0x20 || *p == 0x7f
		    || strchr(invalid, (char) *p) != NULL)
			*p = '_';
	}

	return *filename && strcmp(filename, ".") != 0
	    && strcmp(filename, "..") != 0;
}

bool
http_content_disposition_filename(const char *header, char *filename,
				  size_t size)
{
	struct http_parameter parameter;

	if (!filename || !size)
		return false;

	filename[0] = '\0';
	if (!header)
		return false;

	if (http_find_parameter(header, "filename*", &parameter) &&
	    http_decode_extended_parameter(filename, size, &parameter) &&
	    sanitize_filename(filename))
		return true;

	filename[0] = '\0';
	if (http_find_parameter(header, "filename", &parameter) &&
	    http_copy_parameter(filename, size, &parameter) &&
	    sanitize_filename(filename))
		return true;

	filename[0] = '\0';
	return false;
}
