/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2020    Jason

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
*/

#ifndef AXEL_COOKIE_H
#define AXEL_COOKIE_H

/* At least 4096 bytes per cookie according to RFC6265 section 6.1 */
#define MAX_COOKIE 4096

#define COOKIE_CHUNK 512

#define COOKIE_PREALLOCATE_NUM 10

/* Basic support for cookies */
typedef struct {
	char *name;
	char *value;
	void *next;
} cookie_t;

#define free_if_exists(variable) { if (variable) free(variable); }

void cookielist_free(cookie_t *cookielist, int num);
int cookielist_loadfile(cookie_t *cookielist, FILE *fd);
void cookielist_header(char *dst, const cookie_t *cookielist, int num, int maxlen);

int cookie_setup(cookie_t *cookie);
int cookie_strcpy(char *str, const char *dst, const char *space);
int cookie_load(cookie_t *cookie, const abuf_t *abuf);

#endif /* AXEL_COOKIE_H */