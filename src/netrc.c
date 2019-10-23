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
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "axel.h"
#include "netrc.h"

static char *p;
static char *s_addr;
static char *e_addr;

static size_t file_size(int fd)
{
	struct stat st;
	fstat(fd, &st);
	return st.st_size;
}

static int netrc_mmap(const char *filename)
{
	int fd;
	size_t sz;
	char *aux = NULL;
	char *home = NULL;
	char *path = NULL;
	const char suffix[] = "/.netrc";

	if (filename && *filename) {
		aux = path = (char *)filename;
	} else if ((path = getenv("NETRC"))) {
		aux = path;
	} else if ((home = getenv("HOME"))) {
		size_t i = strlen(home);
		if ((path = malloc(i + sizeof(suffix)))) {
			memcpy(path, home, i);
			memcpy(path+i, suffix, sizeof(suffix));
		}
	}
	if (!path)
		return 0;
	fd = open(path, O_RDONLY,0);
	if (!aux)
		free(path);
	if (fd == -1) {
		return 0;
	} else {
		sz = file_size(fd);
		s_addr = mmap(NULL, sz, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
		close(fd);
		e_addr = s_addr + sz;
		return sz;
	}
}

static void netrc_munmap(size_t sz)
{
	munmap(s_addr, sz);
}

static size_t
memspn(const char *s, const char *t, size_t len)
{
	size_t sz = 0;

	while (*s && len-- && strchr(t, *s++))
		sz++;
	return sz;
}

static size_t
memcspn(const char *s, const char *t, size_t len)
{
	size_t sz = 0;

	while (*s && len--)
		if (strchr(t, *s))
			return sz;
		else
			s++, sz++;
	return sz;
}

static size_t next_token(char **t)
{
	char *q;
	size_t len;
	const char *delims = " \t\n";

	if (!p)
		p = s_addr;
	p += memspn(p, delims, (e_addr - p));
	q = p;
	q += memcspn(q, delims, (e_addr - q));
	if (q == e_addr)
		return 0;
	len = q-p;
	*t = p;
	p = q;
	return len;
}

static void
get_creds(char *user, size_t ul, char *pwd, size_t pl)
{
	size_t len;
	char *tok;

	while ((len = next_token(&tok))) {
		if (!strncmp("login", tok, len)) {
			len = next_token(&tok);
			/* next_token() doesn't null-terminate */
			if (len <= ul)
				strlcpy(user, tok, len+1);
		} else if (!strncmp("password", tok, len)) {
			len = next_token(&tok);
			if (len <= pl)
				strlcpy(pwd, tok, len+1);
		} else if (!strncmp("machine", tok, len) || !strncmp("default", tok, len)) {
			p -= len;
			break;
		}
	}
}

int
netrc_parse(const char *filename, const char *host,
	    char *user, size_t ul, char *password, size_t pl)
{
	size_t len;
	size_t sz;
	char *tok;

	if (!(sz = netrc_mmap(filename)))
		return 0;
	while ((len = next_token(&tok))) {
		if (!strncmp("default", tok, len)) {
			get_creds(user, ul, password, pl);
			break;
		} else if (!strncmp("machine", tok, len)) {
			if ((len = next_token(&tok)) && !strncmp(host, tok, len)) {
				get_creds(user, ul, password, pl);
				break;
			}
		}
	}
	netrc_munmap(sz);
	return 1;
}
