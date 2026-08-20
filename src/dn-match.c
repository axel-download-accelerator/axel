/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2020      Ismael Luceno

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

#include "config.h"
#include <strings.h>
#include "axel.h"

#define DN_NEQ 1

static
int
dn_prefix_equal(const char *hostname, const char *pat, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (!hostname[i] ||
		    tolower((unsigned char)hostname[i]) !=
		    tolower((unsigned char)pat[i]))
			return 0;
	}
	return 1;
}

static
int
dn_equal(const char *hostname, const char *pat, size_t pat_len)
{
	return strlen(hostname) == pat_len &&
		dn_prefix_equal(hostname, pat, pat_len);
}

/**
 * Hostname matching according to RFC-6125 section 6.4.3.
 *
 * For simplicity, at most one wildcard is supported, on the leftmost label
 * only.
 *
 * Hostname must be normalized and ASCII-only.
 *
 * @returns Negative on malformed input, Zero if matched, non-zero otherwise.
 */
int
dn_match(const char *hostname, const char *pat, size_t pat_len)
{
	if (!pat_len || memchr(pat, '\0', pat_len))
		return DN_MATCH_MALFORMED;

	/* The pattern is partitioned at the first wildcard or dot */
	size_t left = 0;
	while (left < pat_len && pat[left] != '.' && pat[left] != '*')
		left++;

	/* We can't match an IDN against a wildcard */
	NONSTRING const char ace_prefix[4] = "xn--";
	if (left < pat_len && pat[left] == '*' &&
	    left != 0)
		return DN_MATCH_MALFORMED;
	if (left < pat_len && pat[left] == '*' &&
	    !strncasecmp(hostname, ace_prefix, 4))
		return DN_NEQ;

	/* Compare left-side partition */
	if (!dn_prefix_equal(hostname, pat, left))
		return DN_NEQ;

	hostname += left;
	pat += left;
	pat_len -= left;

	/* Wildcard? */
	size_t right = 0;
	if (*pat == '*') {
		if (pat_len <= 1 || pat[1] != '.')
			return DN_MATCH_MALFORMED;
		pat_len--;
		pat++;
		while (right < pat_len && pat[right] != '.')
			right++;
		const size_t rem = strcspn(hostname, ".");
		/* Shorter label in hostname? */
		if (!rem || right > rem)
			return DN_NEQ;
		/* Skip the longest match and adjust pat_len */
		hostname += rem - right;
	}

	/* Compare right-side partition */
	return dn_equal(hostname, pat, pat_len) ? 0 : DN_NEQ;
}
