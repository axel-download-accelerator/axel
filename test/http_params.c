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

/* RFC 8187 extended parameter decoding. */

TEST(a_utf8_extended_parameter_is_percent_decoded)
{
	struct http_parameter parameter;
	char output[64];

	ASSERT(http_find_parameter("attachment; "
				   "filename*=UTF-8''%E3%83%86%E3%82%B9%E3%83%88.m2ts",
				   "filename*", &parameter));
	ASSERT(http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
	CHECK_STR(output, "\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88.m2ts");
}

TEST(a_two_byte_utf8_character_is_percent_decoded)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter
	       ("attachment; filename*=UTF-8'fr'caf%C3%A9.txt", "filename*",
		&parameter));
	ASSERT(http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
	CHECK_STR(output, "caf\xC3\xA9.txt");
}

TEST(a_chinese_filename_is_percent_decoded)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter("attachment; "
				   "filename*=UTF-8'zh-CN'%E6%B5%8B%E8%AF%95.txt",
				   "filename*", &parameter));
	ASSERT(http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
	CHECK_STR(output, "\xE6\xB5\x8B\xE8\xAF\x95.txt");
}

TEST(the_optional_language_is_accepted_and_ignored)
{
	struct http_parameter parameter;
	char output[64];

	ASSERT(http_find_parameter("attachment; "
				   "filename*=UTF-8'ja'%E3%83%86%E3%82%B9%E3%83%88.m2ts",
				   "filename*", &parameter));
	ASSERT(http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
	CHECK_STR(output, "\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88.m2ts");
}

TEST(the_utf8_charset_name_ignores_case)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter("attachment; filename*=utf-8''report.txt",
				   "filename*", &parameter));
	ASSERT(http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
	CHECK_STR(output, "report.txt");
}

TEST(unencoded_attribute_characters_are_preserved)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter
	       ("attachment; filename*=UTF-8''report-1_final.txt", "filename*",
		&parameter));
	ASSERT(http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
	CHECK_STR(output, "report-1_final.txt");
}

TEST(a_percent_encoded_path_separator_is_only_decoded_here)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter
	       ("attachment; filename*=UTF-8''dir%2Ffile.txt", "filename*",
		&parameter));
	ASSERT(http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
	CHECK_STR(output, "dir/file.txt");
}

TEST(an_unsupported_charset_is_rejected_without_output)
{
	struct http_parameter parameter;
	char output[16] = "unchanged";

	ASSERT(http_find_parameter
	       ("attachment; filename*=ISO-8859-1''report.txt", "filename*",
		&parameter));
	ASSERT(!http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
	CHECK_STR(output, "unchanged");
}

TEST(an_invalid_percent_escape_is_rejected_without_output)
{
	struct http_parameter parameter;
	char output[16] = "unchanged";

	ASSERT(http_find_parameter("attachment; filename*=UTF-8''bad%ZZ.txt",
				   "filename*", &parameter));
	ASSERT(!http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
	CHECK_STR(output, "unchanged");
}

TEST(a_truncated_percent_escape_is_rejected)
{
	struct http_parameter parameter;
	char output[16];

	ASSERT(http_find_parameter
	       ("attachment; filename*=UTF-8''bad%2", "filename*", &parameter));
	ASSERT(!http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
}

TEST(a_percent_encoded_nul_is_rejected)
{
	struct http_parameter parameter;
	char output[16];

	ASSERT(http_find_parameter("attachment; filename*=UTF-8''a%00b.txt",
				   "filename*", &parameter));
	ASSERT(!http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
}

TEST(an_overlong_utf8_sequence_is_rejected)
{
	struct http_parameter parameter;
	char output[16];

	ASSERT(http_find_parameter("attachment; filename*=UTF-8''%C0%AF.txt",
				   "filename*", &parameter));
	ASSERT(!http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
}

TEST(a_utf8_surrogate_sequence_is_rejected)
{
	struct http_parameter parameter;
	char output[16];

	ASSERT(http_find_parameter("attachment; filename*=UTF-8''%ED%A0%80.txt",
				   "filename*", &parameter));
	ASSERT(!http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
}

TEST(a_four_byte_utf8_sequence_is_accepted)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter
	       ("attachment; filename*=UTF-8''%F0%9F%98%80.txt", "filename*",
		&parameter));
	ASSERT(http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
	CHECK_STR(output, "\xF0\x9F\x98\x80.txt");
}

TEST(a_truncated_utf8_sequence_is_rejected)
{
	struct http_parameter parameter;
	char output[16];

	ASSERT(http_find_parameter
	       ("attachment; filename*=UTF-8''%E3%83", "filename*",
		&parameter));
	ASSERT(!http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
}

TEST(a_codepoint_beyond_the_unicode_range_is_rejected)
{
	struct http_parameter parameter;
	char output[16];

	ASSERT(http_find_parameter
	       ("attachment; filename*=UTF-8''%F4%90%80%80.txt", "filename*",
		&parameter));
	ASSERT(!http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
}

TEST(a_quoted_extended_value_is_rejected)
{
	struct http_parameter parameter;
	char output[16];

	ASSERT(http_find_parameter
	       ("attachment; filename*=\"UTF-8''report.txt\"", "filename*",
		&parameter));
	ASSERT(parameter.quoted);
	ASSERT(!http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
}

TEST(a_raw_space_in_an_extended_value_is_rejected)
{
	struct http_parameter parameter;
	char output[32];

	ASSERT(http_find_parameter("attachment; filename*=UTF-8''two words.txt",
				   "filename*", &parameter));
	ASSERT(!http_decode_extended_parameter
	       (output, sizeof(output), &parameter));
}

TEST(an_extended_value_needs_room_for_its_final_nul)
{
	struct http_parameter parameter;
	char too_small[5] = "keep";
	char exact[sizeof("a.txt")];

	ASSERT(http_find_parameter
	       ("attachment; filename*=UTF-8''a.txt", "filename*", &parameter));
	ASSERT(!http_decode_extended_parameter
	       (too_small, sizeof(too_small), &parameter));
	CHECK_STR(too_small, "keep");
	ASSERT(http_decode_extended_parameter
	       (exact, sizeof(exact), &parameter));
	CHECK_STR(exact, "a.txt");
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
	REGISTER_DESC(a_utf8_extended_parameter_is_percent_decoded,
		      "a UTF-8 extended parameter is percent-decoded");
	REGISTER_DESC(a_two_byte_utf8_character_is_percent_decoded,
		      "a two-byte UTF-8 character is percent-decoded");
	REGISTER_DESC(a_chinese_filename_is_percent_decoded,
		      "a Chinese filename and language tag are decoded");
	REGISTER_DESC(the_optional_language_is_accepted_and_ignored,
		      "the optional language is accepted and ignored");
	REGISTER_DESC(the_utf8_charset_name_ignores_case,
		      "the UTF-8 charset name ignores case");
	REGISTER_DESC(unencoded_attribute_characters_are_preserved,
		      "unencoded attribute characters are preserved");
	REGISTER_DESC(a_percent_encoded_path_separator_is_only_decoded_here,
		      "a percent-encoded path separator is only decoded here");
	REGISTER_DESC(an_unsupported_charset_is_rejected_without_output,
		      "an unsupported charset is rejected without output");
	REGISTER_DESC(an_invalid_percent_escape_is_rejected_without_output,
		      "an invalid percent escape is rejected without output");
	REGISTER_DESC(a_truncated_percent_escape_is_rejected,
		      "a truncated percent escape is rejected");
	REGISTER_DESC(a_percent_encoded_nul_is_rejected,
		      "a percent-encoded NUL is rejected");
	REGISTER_DESC(an_overlong_utf8_sequence_is_rejected,
		      "an overlong UTF-8 sequence is rejected");
	REGISTER_DESC(a_utf8_surrogate_sequence_is_rejected,
		      "a UTF-8 surrogate sequence is rejected");
	REGISTER_DESC(a_four_byte_utf8_sequence_is_accepted,
		      "a four-byte UTF-8 sequence is accepted");
	REGISTER_DESC(a_truncated_utf8_sequence_is_rejected,
		      "a truncated UTF-8 sequence is rejected");
	REGISTER_DESC(a_codepoint_beyond_the_unicode_range_is_rejected,
		      "a codepoint beyond the Unicode range is rejected");
	REGISTER_DESC(a_quoted_extended_value_is_rejected,
		      "a quoted extended value is rejected");
	REGISTER_DESC(a_raw_space_in_an_extended_value_is_rejected,
		      "a raw space in an extended value is rejected");
	REGISTER_DESC(an_extended_value_needs_room_for_its_final_nul,
		      "an extended value needs room for its final NUL");

	RUN_ALL();
	return DONE();
}
