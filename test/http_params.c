// SPDX-FileCopyrightText: Copyright 2026 SiYu Liu <2028566723@qq.com>
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * test/http_params.c -- HTTP header parameter parsing tests.
 */

#include "config.h"

#include "harness.h"

#include "http_params.h"

/* Finding and copying ordinary parameters. */

TEST(an_unquoted_parameter_is_found_and_copied)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter("attachment; filename=a.txt", "filename",
				   &parameter));
	CHECK_LSTR(parameter.value, parameter.length, "a.txt");
	CHECK(!parameter.quoted);
	ASSERT(http_copy_parameter(output, sizeof(output), &parameter));
	CHECK_STR(output, "a.txt");
}

TEST(a_quoted_parameter_is_found_without_its_quotes)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter("attachment; filename=\"a.txt\"", "filename",
				   &parameter));
	CHECK_LSTR(parameter.value, parameter.length, "a.txt");
	CHECK(parameter.quoted);
	ASSERT(http_copy_parameter(output, sizeof(output), &parameter));
	CHECK_STR(output, "a.txt");
}

TEST(parameter_names_ignore_case_and_surrounding_space)
{
	struct http_parameter parameter;

	ASSERT(http_find_parameter("attachment ; FiLeNaMe \t= \t\"report.txt\"",
				   "filename", &parameter));
	CHECK_LSTR(parameter.value, parameter.length, "report.txt");
	CHECK(parameter.quoted);
}

TEST(a_parameter_name_must_match_in_full)
{
	struct http_parameter parameter;

	ASSERT(http_find_parameter
	       ("attachment; xfilename=wrong-1; filename*=wrong-2; "
		"filename=right.txt", "filename", &parameter));
	CHECK_LSTR(parameter.value, parameter.length, "right.txt");
}

TEST(an_extended_parameter_is_returned_in_its_raw_form)
{
	struct http_parameter parameter;

	ASSERT(http_find_parameter("attachment; filename=a.txt; "
				   "filename*=UTF-8''%E6%B5%8B%E8%AF%95.txt",
				   "filename*", &parameter));
	CHECK_LSTR(parameter.value, parameter.length,
		   "UTF-8''%E6%B5%8B%E8%AF%95.txt");
	CHECK(!parameter.quoted);
}

TEST(a_broken_segment_before_the_parameter_is_skipped)
{
	struct http_parameter parameter;

	ASSERT(http_find_parameter("attachment; broken; filename=right.txt",
				   "filename", &parameter));
	CHECK_LSTR(parameter.value, parameter.length, "right.txt");
}

/* Quoted-string delimiters and quoted-pairs. */

TEST(a_semicolon_inside_quotes_belongs_to_the_value)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter("attachment; filename=\"a;b.txt\"; size=123",
				   "filename", &parameter));
	CHECK_LSTR(parameter.value, parameter.length, "a;b.txt");
	ASSERT(http_copy_parameter(output, sizeof(output), &parameter));
	CHECK_STR(output, "a;b.txt");
}

TEST(quoted_pairs_are_unescaped_when_the_value_is_copied)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter
	       ("attachment; filename=\"say \\\"hello\\\".txt\"", "filename",
		&parameter));
	CHECK_LSTR(parameter.value, parameter.length, "say \\\"hello\\\".txt");
	ASSERT(http_copy_parameter(output, sizeof(output), &parameter));
	CHECK_STR(output, "say \"hello\".txt");
}

TEST(an_unclosed_quoted_value_is_rejected)
{
	struct http_parameter parameter;

	ASSERT(!http_find_parameter("attachment; filename=\"unterminated.txt",
				    "filename", &parameter));
}

TEST(non_space_after_a_closing_quote_invalidates_the_parameter)
{
	struct http_parameter parameter;

	ASSERT(!http_find_parameter
	       ("attachment; filename=\"bad.txt\"unexpected", "filename",
		&parameter));
}

/* Bounds and failure behaviour. */

TEST(a_parameter_search_stops_at_the_end_of_the_header_line)
{
	struct http_parameter parameter;

	ASSERT(!http_find_parameter("attachment; size=123\r\n"
				    "Content-Disposition: attachment; filename=next-line.txt\r\n",
				    "filename", &parameter));
}

TEST(a_missing_parameter_leaves_the_result_structure_unchanged)
{
	static const char marker[] = "unchanged";
	struct http_parameter parameter = {
		.value = marker,
		.length = sizeof(marker) - 1,
		.quoted = true,
	};

	ASSERT(!http_find_parameter
	       ("attachment; size=123", "filename", &parameter));
	CHECK(parameter.value == marker);
	CHECK_EQ(parameter.length, sizeof(marker) - 1);
	CHECK(parameter.quoted);
}

TEST(a_small_destination_is_rejected_without_being_changed)
{
	struct http_parameter parameter;
	char output[4] = "old";

	ASSERT(http_find_parameter("attachment; filename=a.txt", "filename",
				   &parameter));
	ASSERT(!http_copy_parameter(output, sizeof(output), &parameter));
	CHECK_STR(output, "old");
}

TEST(a_destination_with_exactly_enough_space_is_accepted)
{
	struct http_parameter parameter;
	char output[sizeof("a.txt")];

	ASSERT(http_find_parameter("attachment; filename=a.txt", "filename",
				   &parameter));
	ASSERT(http_copy_parameter(output, sizeof(output), &parameter));
	CHECK_STR(output, "a.txt");
}

int
main(void)
{
	REGISTER_DESC(an_unquoted_parameter_is_found_and_copied,
		      "an unquoted parameter is found and copied");
	REGISTER_DESC(a_quoted_parameter_is_found_without_its_quotes,
		      "a quoted parameter is returned without its quotes");
	REGISTER_DESC(parameter_names_ignore_case_and_surrounding_space,
		      "parameter names ignore case and surrounding space");
	REGISTER_DESC(a_parameter_name_must_match_in_full,
		      "a parameter name has to match in full");
	REGISTER_DESC(an_extended_parameter_is_returned_in_its_raw_form,
		      "an extended parameter is returned without decoding");
	REGISTER_DESC(a_broken_segment_before_the_parameter_is_skipped,
		      "a broken segment before a parameter is skipped");
	REGISTER_DESC(a_semicolon_inside_quotes_belongs_to_the_value,
		      "a semicolon inside quotes belongs to the value");
	REGISTER_DESC(quoted_pairs_are_unescaped_when_the_value_is_copied,
		      "quoted-pairs are unescaped when copied");
	REGISTER_DESC(an_unclosed_quoted_value_is_rejected,
		      "an unclosed quoted value is rejected");
	REGISTER_DESC(non_space_after_a_closing_quote_invalidates_the_parameter,
		      "non-space after a closing quote invalidates the parameter");
	REGISTER_DESC(a_parameter_search_stops_at_the_end_of_the_header_line,
		      "a parameter search does not cross the header line");
	REGISTER_DESC(a_missing_parameter_leaves_the_result_structure_unchanged,
		      "a failed search leaves its result structure unchanged");
	REGISTER_DESC(a_small_destination_is_rejected_without_being_changed,
		      "a small destination is rejected without partial output");
	REGISTER_DESC(a_destination_with_exactly_enough_space_is_accepted,
		      "a destination with room for the final NUL is accepted");

	RUN_ALL();
	return DONE();
}
