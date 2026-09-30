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

#ifndef AXEL_HTTP_PARAMS_H
#define AXEL_HTTP_PARAMS_H

#include <stdbool.h>
#include <stddef.h>

struct http_parameter
{
	const char *value;
	size_t length;
	bool quoted;
};

bool http_find_parameter(const char *header, const char *name,
			 struct http_parameter *parameter);

bool http_copy_parameter(char *dest, size_t size,
			 const struct http_parameter *parameter);

bool http_decode_extended_parameter(char *dest, size_t size,
				    const struct http_parameter *parameter);

bool http_content_disposition_filename(const char *header, char *filename,
				       size_t size);

#endif /* AXEL_HTTP_PARAMS_H */
