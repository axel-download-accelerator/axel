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

struct netrc{
	size_t sz;
	char *s_addr;
};

typedef struct {
	char *data;
	size_t len;
} buffer_t;

static const char *tok_delim = " \t\n";

// FIXME optimize
static size_t
memspn(const char *s, const char *accept, size_t len)
{
	size_t sz = 0;

	while (*s && len-- && strchr(accept, *s++))
		sz++;
	return sz;
}

// FIXME optimize
static size_t
memcspn(const char *s, const char *reject, size_t len)
{
	size_t sz = 0;

	while (*s && len--)
		if (strchr(reject, *s))
			return sz;
		else
			s++, sz++;
	return sz;
}

static buffer_t
memtok(const char *addr, size_t len, const char *delim, buffer_t *save_ptr)
{
	size_t sz;
	char *p, *q;
	buffer_t ret;

	if (!addr) {
		p = save_ptr->data;
	} else {
		p = (char *) addr;
		save_ptr->len = len;
	}
	sz = memspn(p, delim, save_ptr->len);
	p += sz;
	save_ptr->len -= sz;
	q = p;
	sz = memcspn(q, delim, save_ptr->len);
	q += sz;
	save_ptr->data = q;
	ret.len = q - p;
	ret.data = p;
	return ret;
}

static size_t
netrc_mmap(const char *path, char **addr)
{
	const char *home = NULL;

	if (!path || !*path)
		path = getenv("NETRC");

	if (!path) {
		const char suffix[] = "/.netrc";

		home = getenv("HOME");
		if (!home)
			return 0;

		size_t i = strlen(home);
		char *tmp = malloc(i + sizeof(suffix));
		if (!tmp)
			return 0;

		memcpy(tmp, home, i);
		memcpy(tmp + i, suffix, sizeof(suffix));
		path = tmp;
	}

	int fd = open(path, O_RDONLY, 0);

	if (home)
		free((void*)path);

	if (fd == -1)
		return 0;

	struct stat st;
	fstat(fd, &st);

	*addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE,
		     fd, 0);

	close(fd);
	return st.st_size;
}

netrc_t *
netrc_init(const char *path)
{
	netrc_t *netrc;

	netrc = calloc(1, sizeof(netrc_t));
	if (netrc) {
		netrc->sz = netrc_mmap(path, &netrc->s_addr);
		if (netrc->sz)
			return netrc;
		free(netrc);
	}
	return NULL;
}

void
netrc_parse(netrc_t *netrc, const char *host, char *user, size_t user_len, char *pass, size_t pass_len)
{
	bool matched = false;
	buffer_t tok, save_buf = {};
	const struct parser {
		const char * const key;
		char *dst;
		size_t len;
	} parser[] = {
		{ "login", user, user_len },
		{ "password", pass, pass_len },
		{ "machine", NULL, 0 },
		{ "default", NULL, 0 },
	};
	enum { parser_len = sizeof(parser) / sizeof(*parser), };

	tok = memtok(netrc->s_addr, netrc->sz, tok_delim, &save_buf);
	while (tok.len) {
		const struct parser *p = parser;
		while (p < parser + parser_len && strncmp(p->key, tok.data, tok.len))
			p++;

		/* unknown token? -> abort */
		if (p >= parser + parser_len)
			return;

		/* next "machine"/"default" entry */
		if (!p->dst) {
			/* if we already got one, we're done */
			if (matched)
				break;
			if (tok.data[0] == 'm') {
				tok = memtok(NULL, 0, tok_delim, &save_buf);
				matched = !strncmp(host, tok.data, tok.len);
			} else {
				matched = true;
			}
		} else {
			tok = memtok(NULL, 0, tok_delim, &save_buf);
			if (matched) {
				// FIXME should we be aborting?
				if (tok.len >= p->len)
					tok.len = p->len - 1;
				memcpy(p->dst, tok.data, tok.len);
				p->dst[tok.len] = 0;
			}
		}
		tok = memtok(NULL, 0, tok_delim, &save_buf);
	}
}
