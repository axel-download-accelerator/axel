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


/* Does the state just read describe the file the server just described?
 *
 * The state file names neither the URL nor the size it was written for, so a
 * download whose output name collides with an unfinished, unrelated one will
 * happily load the other's progress.  Nothing then adds up: the byte counts
 * print as nonsense, every connection asks for a range past the end of the
 * file, and none of it is ever going to finish.
 *
 * What can be checked is the shape.  The chunks axel_divide() lays down tile
 * the file in order, and a connection stops at the end of its own chunk, so
 * the offsets have to climb from zero to exactly the size the server gave.
 * An old-format state file carries no chunk ends -- axel_divide() has just
 * recomputed those from the current size -- and for one of those this only
 * really checks the progress made within each chunk. */
static
bool
state_fits_download(const axel_t *axel)
{
	off_t start = 0;

	if (axel->bytes_done < 0 || axel->bytes_done > axel->size)
		return false;

	for (int i = 0; i < axel->conf->num_connections; i++) {
		const conn_t *conn = &axel->conn[i];

		if (conn->lastbyte > axel->size)
			return false;
		if (conn->currentbyte < start ||
		    conn->currentbyte > conn->lastbyte)
			return false;

		start = conn->lastbyte;
	}

	return start == axel->size;
}


int
stfile_load(axel_t *axel)
{
	int fd = stfile_open(axel->filename, O_RDONLY, 0);
	if (fd == -1)
		return 0;

	/* What to go back to if the state turns out not to be ours */
	uint16_t wanted_conns = axel->conf->num_connections;
	uint16_t nconns;
	int old_format = 0;
	off_t stsize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	ssize_t nread = read(fd, &nconns, sizeof(nconns));
	if (nread != sizeof(nconns)) {
		printf(_("%s.st: Error, truncated state file\n"),
		       axel->filename);
		close(fd);
		return -1;
	}

	if (nconns < 1) {
		fprintf(stderr,
			_("Bogus number of connections stored in state file\n"));
		close(fd);
		return -1;
	}

	if (stsize < (off_t)(sizeof(nconns) +
			     sizeof(axel->bytes_done) +
			     2 * nconns *
			     sizeof(axel->conn[0].currentbyte))) {
		/* FIXME this might be wrong, the file may have been
		 * truncated, we need another way to check. */
#ifndef NDEBUG
		printf(_("State file has old format.\n"));
#endif
		old_format = 1;
	}

	if (!axel_conn_resize(axel, nconns)) {
		close(fd);
		return -1;
	}

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

	close(fd);

	if (!state_fits_download(axel)) {
		axel_message(axel, _("State file %s.st belongs to another "
				     "download, ignoring it."), axel->filename);
		axel->bytes_done = 0;
		return axel_conn_resize(axel, wanted_conns) ? 0 : -1;
	}

	axel_message(axel,
		     _("State file found: %jd bytes downloaded, %jd to go."),
		     (intmax_t)axel->bytes_done,
		     (intmax_t)(axel->size - axel->bytes_done));

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
