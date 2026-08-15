// SPDX-FileCopyrightText: Copyright 2026 Ismael Luceno <ismael@iodev.co.uk>
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * test/conf.c — what may be sent, and where
 *
 * A header given with -H goes out with every request, and a redirect can
 * lead anywhere, so axel has two questions to answer before it follows one:
 * which of those headers carry a credential, and whether where it landed is
 * near enough for one to travel there.
 *
 * Both answers come out of string comparisons that have to fail closed.  A
 * header named for something a credential merely starts with must not be
 * mistaken for one, and a host that differs only in case must not be
 * mistaken for another -- the first would hold back an ordinary header, and
 * the second is the leak this is all here to prevent.
 */

#include "config.h"

#include "harness.h"

#include <stdlib.h>
#include <string.h>

#include "axel.h"

/* src/conf.c reaches for this when a config file names an interface.  No
 * test here does, so this only has to exist, and defining it is what keeps
 * src/tcp.c -- and with it the rest of the network stack -- out of the
 * link.  It answers the way the real one does when it finds nothing. */
int
get_if_ip(char *dst, size_t len, const char *iface)
{
	(void)iface;

	if (len)
		*dst = 0;

	return 0;
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

/* A conf holding nothing but the given headers.  conf_init() is not what is
 * wanted here: it reads the system and user config files, so what came back
 * would be whoever ran the suite. */
static conf_t *
conf_with(const char *const *headers, size_t count)
{
	static conf_t conf;

	memset(&conf, 0, sizeof(conf));
	for (size_t i = 0; i < count; i++)
		strlcpy(conf.add_header[i], headers[i],
			sizeof(conf.add_header[i]));
	conf.add_header_count = count;

	return &conf;
}

/* ── which headers carry a credential ─────────────────────────────────── */

TEST(the_three_credential_headers_are_private)
{
	ASSERT(conf_header_is_private("Cookie: session=1"));
	ASSERT(conf_header_is_private("Authorization: Bearer tok"));
	ASSERT(conf_header_is_private("Proxy-Authorization: Basic eA=="));
}

TEST(the_name_is_read_regardless_of_case)
{
	ASSERT(conf_header_is_private("cookie: session=1"));
	ASSERT(conf_header_is_private("COOKIE: session=1"));
	ASSERT(conf_header_is_private("aUtHoRiZaTiOn: Bearer tok"));
}

TEST(an_ordinary_header_is_not_private)
{
	ASSERT(!conf_header_is_private("User-Agent: Axel"));
	ASSERT(!conf_header_is_private("Accept: */*"));
	ASSERT(!conf_header_is_private("X-Custom: keepme"));
}

TEST(a_header_that_merely_starts_with_one_is_not_it)
{
	ASSERT(!conf_header_is_private("Cookies: two"));
	ASSERT(!conf_header_is_private("Cookie-Policy: strict"));
	ASSERT(!conf_header_is_private("Authorization-Note: none"));
}

TEST(a_header_that_merely_ends_with_one_is_not_it)
{
	ASSERT(!conf_header_is_private("X-Cookie: session=1"));
	ASSERT(!conf_header_is_private("Set-Cookie: session=1"));
}

TEST(a_name_with_no_colon_after_it_is_not_a_header)
{
	ASSERT(!conf_header_is_private("Cookie"));
	ASSERT(!conf_header_is_private("Cookie session=1"));
	ASSERT(!conf_header_is_private(""));
}

TEST(space_typed_before_the_colon_is_still_that_header)
{
	ASSERT(conf_header_is_private("Cookie : session=1"));
	ASSERT(conf_header_is_private("Cookie\t: session=1"));
}

TEST(a_list_of_ordinary_headers_holds_no_credential)
{
	static const char *const headers[] = {
		"User-Agent: Axel",
		"X-Custom: keepme",
	};

	ASSERT(!conf_has_private_headers(conf_with(headers,
						   ARRAY_SIZE(headers))));
}

TEST(a_credential_anywhere_in_the_list_is_found)
{
	static const char *const headers[] = {
		"User-Agent: Axel",
		"X-Custom: keepme",
		"Cookie: session=1",
	};

	ASSERT(conf_has_private_headers(conf_with(headers,
						  ARRAY_SIZE(headers))));
}

TEST(an_empty_list_holds_no_credential)
{
	ASSERT(!conf_has_private_headers(conf_with(NULL, 0)));
}

/* ── where a credential may follow ────────────────────────────────────── */

TEST(a_redirect_within_a_host_keeps_them)
{
	ASSERT(conf_credentials_may_follow(PROTO_HTTP, "example.com",
					   PROTO_HTTP, "example.com"));
	ASSERT(conf_credentials_may_follow(PROTO_HTTPS, "example.com",
					   PROTO_HTTPS, "example.com"));
}

TEST(a_redirect_to_another_host_does_not)
{
	ASSERT(!conf_credentials_may_follow(PROTO_HTTP, "example.com",
					    PROTO_HTTP, "example.org"));
	ASSERT(!conf_credentials_may_follow(PROTO_HTTPS, "example.com",
					    PROTO_HTTPS, "evil.example.com"));
}

TEST(a_host_that_differs_only_in_case_is_the_same_host)
{
	ASSERT(conf_credentials_may_follow(PROTO_HTTP, "Example.COM",
					   PROTO_HTTP, "example.com"));
}

TEST(a_host_a_redirect_merely_extends_is_another_host)
{
	ASSERT(!conf_credentials_may_follow(PROTO_HTTP, "example.com",
					    PROTO_HTTP, "example.com.evil.net"));
	ASSERT(!conf_credentials_may_follow(PROTO_HTTP, "example.com",
					    PROTO_HTTP, "example.co"));
}

TEST(leaving_tls_leaves_them_behind)
{
	ASSERT(!conf_credentials_may_follow(PROTO_HTTPS, "example.com",
					    PROTO_HTTP, "example.com"));
	ASSERT(!conf_credentials_may_follow(PROTO_FTPS, "example.com",
					    PROTO_FTP, "example.com"));
}

TEST(taking_up_tls_keeps_them)
{
	ASSERT(conf_credentials_may_follow(PROTO_HTTP, "example.com",
					   PROTO_HTTPS, "example.com"));
	ASSERT(conf_credentials_may_follow(PROTO_FTP, "example.com",
					   PROTO_FTPS, "example.com"));
}

TEST(a_host_is_what_counts_not_the_protocol_it_is_reached_by)
{
	/* Nothing above says the two ends have to speak the same thing */
	ASSERT(conf_credentials_may_follow(PROTO_FTP, "example.com",
					   PROTO_HTTP, "example.com"));
	ASSERT(!conf_credentials_may_follow(PROTO_FTP, "example.com",
					    PROTO_HTTP, "example.org"));
}

int
main(void)
{
	REGISTER_DESC(the_three_credential_headers_are_private,
		      "Cookie, Authorization and Proxy-Authorization are credentials");
	REGISTER_DESC(the_name_is_read_regardless_of_case,
		      "a credential header is recognised whatever its case");
	REGISTER_DESC(an_ordinary_header_is_not_private,
		      "an ordinary header is not taken for a credential");
	REGISTER_DESC(a_header_that_merely_starts_with_one_is_not_it,
		      "a header whose name a credential's merely starts is not one");
	REGISTER_DESC(a_header_that_merely_ends_with_one_is_not_it,
		      "a header whose name merely ends in a credential's is not one");
	REGISTER_DESC(a_name_with_no_colon_after_it_is_not_a_header,
		      "a name with no colon after it is not a credential header");
	REGISTER_DESC(space_typed_before_the_colon_is_still_that_header,
		      "space typed before the colon still names that header");
	REGISTER_DESC(a_list_of_ordinary_headers_holds_no_credential,
		      "a list of ordinary headers is reported to hold none");
	REGISTER_DESC(a_credential_anywhere_in_the_list_is_found,
		      "a credential anywhere in the list is reported");
	REGISTER_DESC(an_empty_list_holds_no_credential,
		      "a list with no headers at all is reported to hold none");

	REGISTER_DESC(a_redirect_within_a_host_keeps_them,
		      "a redirect that stays on the host keeps the credentials");
	REGISTER_DESC(a_redirect_to_another_host_does_not,
		      "a redirect to another host leaves the credentials behind");
	REGISTER_DESC(a_host_that_differs_only_in_case_is_the_same_host,
		      "a host that differs only in case is the same host");
	REGISTER_DESC(a_host_a_redirect_merely_extends_is_another_host,
		      "a host name the target merely extends is another host");
	REGISTER_DESC(leaving_tls_leaves_them_behind,
		      "a redirect out of TLS leaves the credentials behind");
	REGISTER_DESC(taking_up_tls_keeps_them,
		      "a redirect into TLS on the same host keeps them");
	REGISTER_DESC(a_host_is_what_counts_not_the_protocol_it_is_reached_by,
		      "the host decides, not which protocol reaches it");

	RUN_ALL();
	return DONE();
}
