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
#include "netrc.h"

#define REL_NETRC	"/.netrc"
#define IS_DELIM(x)	(*x == ' ' || *x == '\t' || *x == '\n')

static int fd;
static size_t sz;
static char *p;
static char *s_addr;
static char *e_addr;

static size_t file_size(void)
{
	struct stat st;
	fstat(fd, &st);
	return st.st_size;
}

static int netrc_open(const char * const filename)
{
	int ret;
	char *path;
	char *netrc;
	char *home;

	ret = 0;
	if (filename && filename[0] != '\0') {
		path = strdup(filename);
	} else {
		if ((netrc = getenv("NETRC"))) {
			path = strdup(netrc);
		} else if ((home = getenv("HOME"))) {
			size_t i = strlen(home) + strlen(REL_NETRC) + 1;
			path = malloc(i);
			snprintf(path, i, "%s%s", home, REL_NETRC);
		} else {
			return ret;
		}
	}
	if ((fd = open(path, O_RDONLY, 0)) == -1) {
		ret = 0;
	} else {
		sz = file_size();
		s_addr = mmap(NULL, sz, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
		e_addr = s_addr + sz;
		ret = 1;
	}
	free(path);
	return ret;
}

static void netrc_close(void)
{
	munmap(s_addr, sz);
	close(fd);
}

static int next_token(char **t)
{
	char *q;
	int len;

	if (!p)
		p = s_addr;
	while (IS_DELIM(p) && p < e_addr)
		p++;
	q = p;
	while (!IS_DELIM(q) && q < e_addr)
		q++;
	if (q == e_addr)
		return 0;
	len = q-p;
	*t = p;
	p = q;
	return len;
}

static void get_creds(char *user, char *pwd)
{
	int len;
	char *tok;

	while ((len = next_token(&tok))) {
		if (!strncmp("login", tok, len)) {
			len = next_token(&tok);
			strncpy(user, tok, len);
			user[len] = '\0';
		} else if (!strncmp("password", tok, len)) {
			len = next_token(&tok);
			strncpy(pwd, tok, len);
			pwd[len] = '\0';
		} else if (!strncmp("machine", tok, len) || !strncmp("default", tok, len)) {
			p -= len;
			break;
		}
	}
}

int
netrc_parse(const char *filename, const char *host, char *user, char *password)
{
	int len;
	char *tok;

	if (!netrc_open(filename))
		return 0;
	while ((len = next_token(&tok))) {
		if (!strncmp("default", tok, len)) {
			get_creds(user, password);
			break;
		} else if (!strncmp("machine", tok, len)) {
			if ((len = next_token(&tok)) && !strncmp(host, tok, len)) {
				get_creds(user, password);
				break;
			}
		}
	}
	netrc_close();
	return 0;
}
