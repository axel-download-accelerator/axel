/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2020  Ismael Luceno

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

#define _ISOC99_SOURCE

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "axel.h"


/**
 * Abstract buffer allocation/free.
 * @returns 0 if OK, a negative value on error.
 */
int
abuf_setup(abuf_t *abuf, size_t len)
{
	char *p = realloc(abuf->p, len);
	if (!p && len)
		return -ENOMEM;
	abuf->p = p;
	abuf->len = len;
	return 0;
}

int
abuf_printf(abuf_t *abuf, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	for (;;) {
		size_t len = vsnprintf(abuf->p, abuf->len, fmt, ap);
		if (len < abuf->len)
			break;
		int r = abuf_setup(abuf, len + 1);
		if (r < 0)
			return r;
	}

	va_end(ap);
	return 0;
}

/**
 * String concatenation.  The buffer must contain a valid C string.
 * @returns 0 if OK, or negative value on error.
 */
int
abuf_strcat(abuf_t *abuf, const char *src)
{
	size_t nread = strlcat(abuf->p, src, abuf->len);
	if (nread > abuf->len) {
		size_t done = abuf->len - 1;
		int ret = abuf_setup(abuf, nread);
		if (ret < 0)
			return ret;
		memcpy(abuf->p + done, src + done, nread - done);
	}

	return 0;
}
