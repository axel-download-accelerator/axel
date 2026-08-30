// SPDX-FileCopyrightText: Copyright 2026 The Axel contributors
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * test/http.c — HTTP header parameter parsing tests
 *
 * http_header_params() has to tell a quoted semicolon from a real parameter
 * separator and honour backslash escapes inside quotes.  The cases below are
 * the ones that used to come out wrong.
 */

#include "config.h"

#include "harness.h"

#include <string.h>

#include "http_params.h"

/* Parse a synthetic header value and pull out the filename parameter. */
static void
parse(const char *value, char *out, size_t len)
{
	struct header_params params[] = {
		{ "filename", out, len },
	};

	*out = 0;
	http_header_params(value, NULL, 0, params, 1);
}

/* ── the value and its delimiters ─────────────────────────────────────── */

TEST(a_quoted_semicolon_stays_in_the_value)
{
	char out[64];

	parse("attachment; filename=\"a;b.jpg\"", out, sizeof(out));
	CHECK_STR(out, "a;b.jpg");
}

TEST(an_unquoted_value_stops_at_the_semicolon)
{
	char out[64];

	parse("attachment; filename=a;b.jpg", out, sizeof(out));
	CHECK_STR(out, "a");
}

TEST(a_backslash_escape_is_decoded)
{
	char out[64];

	parse("attachment; filename=\"a\\\"b.jpg\"", out, sizeof(out));
	CHECK_STR(out, "a\"b.jpg");
}

TEST(single_quotes_delimit_like_double_quotes)
{
	char out[64];

	parse("attachment; filename='a;b.jpg'", out, sizeof(out));
	CHECK_STR(out, "a;b.jpg");
}

/* ── picking the parameter out of a list ──────────────────────────────── */

TEST(filename_is_found_among_other_parameters)
{
	char out[64];

	parse("attachment; size=123; filename=\"x.jpg\"; foo=bar", out,
	      sizeof(out));
	CHECK_STR(out, "x.jpg");
}

TEST(a_trailing_semicolon_is_ignored)
{
	char out[64];

	parse("attachment; filename=\"x.jpg\";", out, sizeof(out));
	CHECK_STR(out, "x.jpg");
}

/* ── when there is nothing to find ────────────────────────────────────── */

TEST(a_value_without_filename_leaves_the_buffer_alone)
{
	char out[64];
	struct header_params params[] = {
		{ "filename", out, sizeof(out) },
	};

	strcpy(out, "unchanged");
	http_header_params("attachment; size=123", NULL, 0, params, 1);
	CHECK_STR(out, "unchanged");
}

/* ── the leading main value ───────────────────────────────────────────── */

TEST(the_main_value_is_captured_when_asked)
{
	char main_val[64];
	char out[64];
	struct header_params params[] = {
		{ "filename", out, sizeof(out) },
	};

	http_header_params("attachment; filename=\"x.jpg\"", main_val,
			   sizeof(main_val), params, 1);
	CHECK_STR(main_val, "attachment");
	CHECK_STR(out, "x.jpg");
}

int
main(void)
{
	REGISTER_DESC(a_quoted_semicolon_stays_in_the_value,
		      "a quoted semicolon stays inside the value");
	REGISTER_DESC(an_unquoted_value_stops_at_the_semicolon,
		      "an unquoted value ends at the first semicolon");
	REGISTER_DESC(a_backslash_escape_is_decoded,
		      "a backslash-escaped quote is decoded");
	REGISTER_DESC(single_quotes_delimit_like_double_quotes,
		      "single quotes delimit like double quotes");
	REGISTER_DESC(filename_is_found_among_other_parameters,
		      "filename is picked out from among other parameters");
	REGISTER_DESC(a_trailing_semicolon_is_ignored,
		      "a trailing semicolon after the value is ignored");
	REGISTER_DESC(a_value_without_filename_leaves_the_buffer_alone,
		      "a value with no filename leaves the caller's buffer alone");
	REGISTER_DESC(the_main_value_is_captured_when_asked,
		      "the leading main value is captured when a buffer is given");

	RUN_ALL();
	return DONE();
}
