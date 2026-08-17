// SPDX-FileCopyrightText: Copyright 2026 Mohammad Yousaf <myousaf64@users.noreply.github.com>
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * test/tcp.c - TCP connection wait tests.
 *
 * A non-blocking connect makes a socket writable for both success and
 * failure.  tcp_wait_for_connection must read SO_ERROR before it accepts
 * that readiness.  A full socket buffer also provides a local timeout case
 * without depending on a network route that can change under the test.
 */

#include "config.h"

#include "harness.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "axel.h"

static void
close_pair(int pair[2])
{
	close(pair[0]);
	close(pair[1]);
}

TEST(a_writable_socket_reports_a_completed_connection)
{
	int pair[2];

	ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
	ASSERT_EQ(tcp_wait_for_connection(pair[0], 0), 1);
	close_pair(pair);
}

TEST(a_full_socket_buffer_reports_a_timeout)
{
	char buffer[4096] = { 0 };
	int flags;
	int pair[2];
	ssize_t written;

	ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
	flags = fcntl(pair[0], F_GETFL);
	ASSERT_NE(flags, -1);
	ASSERT_EQ(fcntl(pair[0], F_SETFL, flags | O_NONBLOCK), 0);

	for (;;) {
		written = write(pair[0], buffer, sizeof(buffer));
		if (written >= 0)
			continue;
		ASSERT(errno == EAGAIN || errno == EWOULDBLOCK);
		break;
	}

	ASSERT_EQ(tcp_wait_for_connection(pair[0], 0), 0);
	close_pair(pair);
}

TEST(a_refused_connection_reports_the_socket_error)
{
	struct sockaddr_in address;
	socklen_t address_len;
	int flags;
	int listener;
	int ret;
	int socket_fd;

	listener = socket(AF_INET, SOCK_STREAM, 0);
	ASSERT_NE(listener, -1);
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	ASSERT_EQ(bind(listener, (struct sockaddr *)&address, sizeof(address)), 0);
	ASSERT_EQ(listen(listener, 1), 0);
	address_len = sizeof(address);
	ASSERT_EQ(getsockname(listener, (struct sockaddr *)&address, &address_len), 0);
	close(listener);

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	ASSERT_NE(socket_fd, -1);
	flags = fcntl(socket_fd, F_GETFL);
	ASSERT_NE(flags, -1);
	ASSERT_EQ(fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK), 0);
	ret = connect(socket_fd, (struct sockaddr *)&address, sizeof(address));
	ASSERT_EQ(ret, -1);
	if (errno == EINPROGRESS) {
		ASSERT_EQ(tcp_wait_for_connection(socket_fd, 1), -1);
		ASSERT_EQ(errno, ECONNREFUSED);
	} else {
		ASSERT_EQ(errno, ECONNREFUSED);
	}
	close(socket_fd);
}

int
main(void)
{
	REGISTER_DESC(a_writable_socket_reports_a_completed_connection,
		      "a writable socket with no error reports a completed connection");
	REGISTER_DESC(a_full_socket_buffer_reports_a_timeout,
		      "a socket that cannot become writable reports a timeout");
	REGISTER_DESC(a_refused_connection_reports_the_socket_error,
		      "a refused connection reports its socket error");
	RUN_ALL();
	return DONE();
}
