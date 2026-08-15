/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2008      Philipp Hagemeister
  Copyright 2016      Stephen Thirlwall
  Copyright 2017-2026 Ismael Luceno

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

/* The .st file that lets an interrupted download be resumed.
 *
 * It holds the number of connections, how many bytes are done in total, and
 * the range each connection was working on, all in the machine's own
 * representation.  A file older than the range fields is recognised by its
 * length and still read; the ranges are recomputed for it. */

#include "config.h"
#include "axel.h"
#include "assert.h"
#include "stfile.h"

static
char *
stfile_makename(const char *bname)
{
	const char suffix[] = ".st";
	const size_t bname_len = strlen(bname);
	char *buf = malloc(bname_len + sizeof(suffix));
	if (!buf) {
		perror("stfile_open");
		abort();
	}
	memcpy(buf, bname, bname_len);
	memcpy(buf + bname_len, suffix, sizeof(suffix));
	return buf;
}


int
stfile_unlink(const char *bname)
{
	char *stname = stfile_makename(bname);
	int ret = unlink(stname);
	free(stname);
	return ret;
}

int
stfile_access(const char *bname, int mode)
{
	char *stname = stfile_makename(bname);
	int ret = access(stname, mode);
	free(stname);
	return ret;
}


static
int
stfile_open(const char *bname, int flags, mode_t mode)
{
	char *stname = stfile_makename(bname);
	int fd = open(stname, flags, mode);
	free(stname);
	return fd;
}


int
stfile_load(axel_t *axel)
{
	int fd = stfile_open(axel->filename, O_RDONLY, 0);
	if (fd == -1)
		return 0;

	int old_format = 0;
	off_t stsize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	ssize_t nread = read(fd, &axel->conf->num_connections,
			     sizeof(axel->conf->num_connections));
	if (nread != sizeof(axel->conf->num_connections)) {
		printf(_("%s.st: Error, truncated state file\n"),
		       axel->filename);
		close(fd);
		return -1;
	}

	if (axel->conf->num_connections < 1) {
		fprintf(stderr,
			_("Bogus number of connections stored in state file\n"));
		close(fd);
		return -1;
	}

	if (stsize < (off_t)(sizeof(axel->conf->num_connections) +
			     sizeof(axel->bytes_done) +
			     2 * axel->conf->num_connections *
			     sizeof(axel->conn[0].currentbyte))) {
		/* FIXME this might be wrong, the file may have been
		 * truncated, we need another way to check. */
#ifndef NDEBUG
		printf(_("State file has old format.\n"));
#endif
		old_format = 1;
	}

	void *new_conn = realloc(axel->conn, sizeof(conn_t) *
				 axel->conf->num_connections);
	if (!new_conn) {
		close(fd);
		return -1;
	}
	axel->conn = new_conn;

	memset(axel->conn + 1, 0,
	       sizeof(conn_t) * (axel->conf->num_connections - 1));

	if (old_format)
		axel_divide(axel);

	nread = read(fd, &axel->bytes_done, sizeof(axel->bytes_done));
	assert(nread == sizeof(axel->bytes_done));
	for (int i = 0; i < axel->conf->num_connections; i++) {
		nread = read(fd, &axel->conn[i].currentbyte,
			     sizeof(axel->conn[i].currentbyte));
		assert(nread == sizeof(axel->conn[i].currentbyte));
		if (!old_format) {
			nread = read(fd, &axel->conn[i].lastbyte,
				     sizeof(axel->conn[i].lastbyte));
			assert(nread == sizeof(axel->conn[i].lastbyte));
		}
	}

	axel_message(axel,
		     _("State file found: %jd bytes downloaded, %jd to go."),
		     axel->bytes_done, axel->size - axel->bytes_done);

	close(fd);
	return 1;
}


/**
 * Save the state of the current download.
 */
void
stfile_save(axel_t *axel)
{
	/* No use for such a file if the server doesn't support
	   resuming anyway.. */
	if (!axel->conn[0].supported)
		return;

	int fd;
	fd = stfile_open(axel->filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (fd == -1) {
		return;		/* Not 100% fatal.. */
	}

	ssize_t nwrite;
	(void)nwrite; /* workaround unused variable warning */
	nwrite =
	    write(fd, &axel->conf->num_connections,
		  sizeof(axel->conf->num_connections));
	assert(nwrite == sizeof(axel->conf->num_connections));

	nwrite = write(fd, &axel->bytes_done, sizeof(axel->bytes_done));
	assert(nwrite == sizeof(axel->bytes_done));

	for (int i = 0; i < axel->conf->num_connections; i++) {
		nwrite =
		    write(fd, &axel->conn[i].currentbyte,
			  sizeof(axel->conn[i].currentbyte));
		assert(nwrite == sizeof(axel->conn[i].currentbyte));
		nwrite =
		    write(fd, &axel->conn[i].lastbyte,
			  sizeof(axel->conn[i].lastbyte));
		assert(nwrite == sizeof(axel->conn[i].lastbyte));
	}
	close(fd);
}
