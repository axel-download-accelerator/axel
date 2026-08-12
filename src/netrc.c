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

#include "config.h"
#include <sys/mman.h>
#include "axel.h"
#include "netrc.h"

/* Linux-only hint, we just don't get the prefaulting elsewhere */
#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

struct netrc{
	size_t sz;
	char *s_addr;
};

typedef struct {
	char *data;
	size_t len;
} buffer_t;

static const char *tok_delim = " \t\n";

/* Note: strchr() matches the terminating NUL of the set, so a NUL byte
   in the input counts as part of any set. A .netrc has no business
   containing one, and taking it for a delimiter is the safe reading. */

// FIXME optimize
static size_t
memspn(const char *s, const char *accept, size_t len)
{
	size_t i;

	for (i = 0; i < len && strchr(accept, s[i]); i++) ;
	return i;
}

// FIXME optimize
static size_t
memcspn(const char *s, const char *reject, size_t len)
{
	size_t i;

	for (i = 0; i < len && !strchr(reject, s[i]); i++) ;
	return i;
}

/**
 * Like strtok_r(), but bounded instead of NUL-terminated, and it leaves
 * the input alone.
 *
 * Pass the buffer on the first call, and NULL on the following ones;
 * save_ptr keeps what's left of it. A zero-length token means the end
 * of the buffer was reached.
 */
static buffer_t
memtok(const char *addr, size_t len, const char *delim, buffer_t *save_ptr)
{
	buffer_t ret;
	size_t sz;

	if (addr) {
		save_ptr->data = (char *)addr;
		save_ptr->len = len;
	}

	/* skip the delimiters before the token */
	sz = memspn(save_ptr->data, delim, save_ptr->len);
	save_ptr->data += sz;
	save_ptr->len -= sz;

	ret.data = save_ptr->data;
	ret.len = memcspn(save_ptr->data, delim, save_ptr->len);

	/* and consume the token itself */
	save_ptr->data += ret.len;
	save_ptr->len -= ret.len;

	return ret;
}

static size_t
netrc_mmap(const char *path, char **addr)
{
	const char *home = NULL;

	if (!path || !*path)
		path = getenv("NETRC");

	if (!path || !*path) {
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

	int fd = open(path, O_RDONLY);

	if (home)
		free((void*)path);

	if (fd == -1)
		return 0;

	struct stat st;

	/* an empty file can't be mapped, and there'd be nothing to parse */
	if (fstat(fd, &st) == -1 || st.st_size <= 0) {
		close(fd);
		return 0;
	}

	*addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE,
		     fd, 0);

	close(fd);

	if (*addr == MAP_FAILED)
		return 0;

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
netrc_free(netrc_t *netrc)
{
	if (!netrc)
		return;

	munmap(netrc->s_addr, netrc->sz);
	free(netrc);
}

static void
skip_line(buffer_t *buf)
{
	char *nl = memchr(buf->data, '\n', buf->len);
	size_t skip = nl ? (size_t)(nl + 1 - buf->data) : buf->len;

	buf->data += skip;
	buf->len -= skip;
}

/**
 * Skip the body of a macdef entry, which runs up to the next empty line.
 */
static void
skip_macdef(buffer_t *buf)
{
	do {
		skip_line(buf);
	} while (buf->len && *buf->data != '\n');
}

enum {
	NETRC_LOGIN,
	NETRC_PASSWORD,
	NETRC_ACCOUNT,
	NETRC_MACDEF,
	NETRC_MACHINE,
	NETRC_DEFAULT,
};

void
netrc_parse(netrc_t *netrc, const char *host,
	    char *user, size_t user_len, char *pass, size_t pass_len)
{
	bool matched = false;
	size_t host_len;
	buffer_t tok, save_buf = { 0 };
#define TOKEN(s) s, sizeof(s) - 1
	const struct parser {
		const char * const key;
		size_t key_len;
		char *dst;
		size_t len;
	} parser[] = {
		[NETRC_LOGIN]    = { TOKEN("login"), user, user_len },
		[NETRC_PASSWORD] = { TOKEN("password"), pass, pass_len },
		/* known to the format, but of no use to us */
		[NETRC_ACCOUNT]  = { TOKEN("account"), NULL, 0 },
		[NETRC_MACDEF]   = { TOKEN("macdef"), NULL, 0 },
		/* entry delimiters */
		[NETRC_MACHINE]  = { TOKEN("machine"), NULL, 0 },
		[NETRC_DEFAULT]  = { TOKEN("default"), NULL, 0 },
	};
#undef TOKEN
	enum { parser_len = sizeof(parser) / sizeof(*parser), };

	if (!netrc)
		return;

	host_len = strlen(host);

	tok = memtok(netrc->s_addr, netrc->sz, tok_delim, &save_buf);
	while (tok.len) {
		const struct parser *p = parser;

		/* comments run up to the end of the line */
		if (*tok.data == '#') {
			skip_line(&save_buf);
			tok = memtok(NULL, 0, tok_delim, &save_buf);
			continue;
		}

		while (p < parser + parser_len
		       && (p->key_len != tok.len
			   || memcmp(p->key, tok.data, tok.len)))
			p++;

		/* unknown token? -> abort */
		if (p >= parser + parser_len)
			return;

		switch (p - parser) {
		case NETRC_MACHINE:
			/* if we already got our entry, we're done */
			if (matched)
				return;

			tok = memtok(NULL, 0, tok_delim, &save_buf);
			matched = tok.len == host_len
				  && !memcmp(host, tok.data, tok.len);
			break;

		case NETRC_DEFAULT:
			if (matched)
				return;

			matched = true;
			break;

		case NETRC_MACDEF:
			/* consume the macro name, then its body */
			tok = memtok(NULL, 0, tok_delim, &save_buf);
			skip_macdef(&save_buf);
			break;

		default:
			tok = memtok(NULL, 0, tok_delim, &save_buf);
			if (!matched || !p->dst)
				break;

			/* the buffers are large enough for anything
			   sensible, so just truncate as we do elsewhere */
			if (tok.len >= p->len)
				tok.len = p->len - 1;
			memcpy(p->dst, tok.data, tok.len);
			p->dst[tok.len] = 0;
			break;
		}

		tok = memtok(NULL, 0, tok_delim, &save_buf);
	}
}
