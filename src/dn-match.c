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
*/

#include "config.h"
#include <strings.h>
#include "axel.h"

#define DN_NEQ 1

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
	/* The pattern is partitioned at the first wildcard or dot */
	const size_t left = strcspn(pat, ".*");

	/* We can't match an IDN against a wildcard */
	const char ace_prefix[4] = "xn--";
	if (pat[left] == '*' && !strncasecmp(hostname, ace_prefix, 4))
		return DN_NEQ;

	/* Compare left-side partition */
	if (strncasecmp(hostname, pat, left))
		return DN_NEQ;

	hostname += left;
	pat += left;
	pat_len -= left;

	/* Wildcard? */
	size_t right = 0;
	if (*pat == '*') {
		--pat_len;
		right = strcspn(++pat, ".");
		const size_t rem = strcspn(hostname, ".");
		/* Shorter label in hostname? */
		if (right > rem)
			return DN_NEQ;
		/* Skip the longest match and adjust pat_len */
		hostname += rem - right;
	}

	/* Check premature end of pattern (malformed certificate) */
	if (pat_len == right - strlen(pat + right))
		return DN_MATCH_MALFORMED;

	/* Compare right-side partition */
	return !!strcasecmp(hostname, pat);
}
