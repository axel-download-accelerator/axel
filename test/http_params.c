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

/* Content-Disposition filename selection and sanitization. */

TEST(an_extended_filename_is_preferred_over_the_legacy_fallback)
{
	char output[64];

	ASSERT(http_content_disposition_filename
	       ("attachment; filename=\"????????.m2ts\"; "
		"filename*=UTF-8''%E3%83%86%E3%82%B9%E3%83%88.m2ts", output,
		sizeof(output)));
	CHECK_STR(output, "\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88.m2ts");
}

TEST(an_invalid_extended_filename_falls_back_to_filename)
{
	char output[32];

	ASSERT(http_content_disposition_filename
	       ("attachment; filename=\"fallback.txt\"; "
		"filename*=UTF-8''bad%ZZ.txt", output, sizeof(output)));
	CHECK_STR(output, "fallback.txt");
}

TEST(an_unsupported_extended_charset_falls_back_to_filename)
{
	char output[32];

	ASSERT(http_content_disposition_filename
	       ("attachment; filename=\"fallback.txt\"; "
		"filename*=ISO-8859-1''report.txt", output, sizeof(output)));
	CHECK_STR(output, "fallback.txt");
}

TEST(a_legacy_filename_is_used_when_no_extended_one_exists)
{
	char output[32];

	ASSERT(http_content_disposition_filename
	       ("attachment; filename=\"a;b.txt\"", output, sizeof(output)));
	CHECK_STR(output, "a;b.txt");
}

TEST(decoded_separators_and_control_characters_are_sanitized)
{
	char output[32];

	ASSERT(http_content_disposition_filename("attachment; "
						 "filename*=UTF-8''dir%2Fline%0Abad%3F.txt",
						 output, sizeof(output)));
	CHECK_STR(output, "dir_line_bad_.txt");
}

TEST(a_legacy_filename_is_sanitized_after_quoted_pair_copying)
{
	char output[32];

	ASSERT(http_content_disposition_filename
	       ("attachment; filename=\"dir\\\\file?.txt\"", output,
		sizeof(output)));
	CHECK_STR(output, "dir_file_.txt");
}

TEST(an_unusable_extended_filename_falls_back_to_filename)
{
	char output[32];

	ASSERT(http_content_disposition_filename
	       ("attachment; filename=safe.txt; filename*=UTF-8''..", output,
		sizeof(output)));
	CHECK_STR(output, "safe.txt");
}

/* A long stem, to push names past what a directory entry holds. */
#define LONG_STEM \
	"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
	"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
	"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
	"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"

/* Whether every byte before the suffix is the letter 'a'. */
static bool
is_stem_of(const char *name, size_t length, const char *suffix)
{
	size_t suffix_length = strlen(suffix);

	if (strlen(name) != length)
		return false;
	if (suffix_length && strcmp(name + length - suffix_length, suffix))
		return false;

	for (size_t i = 0; i < length - suffix_length; i++)
		if (name[i] != 'a')
			return false;

	return true;
}

TEST(a_long_name_is_cut_down_to_fit_a_directory_entry)
{
	char output[1024];
	char header[2048];

	snprintf(header, sizeof(header),
		 "attachment; filename=\"%s.txt\"", LONG_STEM);
	ASSERT(http_content_disposition_filename(header, output,
						 sizeof(output)));
	CHECK(is_stem_of(output, 255, ".txt"));
}

TEST(a_compressed_name_keeps_both_of_its_suffixes)
{
	char output[1024];
	char header[2048];

	snprintf(header, sizeof(header),
		 "attachment; filename=\"%s.tar.gz\"", LONG_STEM);
	ASSERT(http_content_disposition_filename(header, output,
						 sizeof(output)));
	CHECK(is_stem_of(output, 255, ".tar.gz"));
}

TEST(a_pair_is_not_made_of_a_final_word_that_is_too_long)
{
	char output[1024];
	char header[2048];

	/* The whole of .extensionthatistoowide is over the budget, so
	 * there is no extension to keep and nothing but the stem is. */
	snprintf(header, sizeof(header),
		 "attachment; filename=\"%s.extensionthatistoowide\"", LONG_STEM);
	ASSERT(http_content_disposition_filename(header, output,
						 sizeof(output)));
	CHECK(is_stem_of(output, 255, ""));
}

TEST(an_ordinary_suffix_does_not_take_the_word_in_front_of_it)
{
	char output[1024];
	char header[2048];

	/* Only a compressor takes the second dot.  report.final.txt is two
	 * ordinary suffixes, so the name is cut to the last one. */
	snprintf(header, sizeof(header),
		 "attachment; filename=\"%s.final.txt\"", LONG_STEM);
	ASSERT(http_content_disposition_filename(header, output,
						 sizeof(output)));
	CHECK(is_stem_of(output, 255, ".txt"));
}

TEST(each_compressor_suffix_takes_the_word_in_front_of_it)
{
	static const char *const pairs[] = {
		".tar.7z", ".tar.bz2", ".tar.bz3", ".tar.br", ".tar.gz",
		".tar.lz", ".tar.lz4", ".tar.lzma", ".tar.lzo", ".tar.rz",
		".tar.sz", ".tar.xz", ".tar.z", ".tar.zz", ".tar.zst",
		".tar.Z",
	};
	char output[1024];
	char header[2048];

	for (size_t i = 0; i < sizeof(pairs) / sizeof(*pairs); i++) {
		snprintf(header, sizeof(header), "attachment; filename=\"%s%s\"",
			 LONG_STEM, pairs[i]);
		ASSERT(http_content_disposition_filename(header, output,
							 sizeof(output)));
		CHECK(is_stem_of(output, 255, pairs[i]));
	}
}

/* The word that a compressor suffix takes with it is the one naming the
 * format, so it is a plain word: letters and digits, and nothing that
 * would say the name rather than the format. */
TEST(only_a_plain_word_before_a_compressor_suffix_is_kept)
{
	static const char *const kept[] = {
		"tar", "2024", "x2", "Z", "a",
	};
	static const char *const refused[] = {
		"log-2024", "my-file", "a-", "v2-final", "under_score",
		"my file", "tar!", "caf\xc3\xa9",
	};
	/* Short enough that the cut lands inside the word rather than in
	 * the stem, which is where a word dropped in halves would show. */
	char stem[251];
	char output[1024];
	char header[2048];
	char expected[64];

	memset(stem, 'a', 250);
	stem[250] = '\0';

	/* Only the one word before the dot is looked at, so it has to be
	 * the one sitting against the suffix. */
	for (size_t i = 0; i < sizeof(kept) / sizeof(*kept); i++) {
		snprintf(expected, sizeof(expected), ".%s.gz", kept[i]);
		snprintf(header, sizeof(header),
			 "attachment; filename=\"%s.%s.gz\"", stem, kept[i]);
		ASSERT(http_content_disposition_filename(header, output,
							 sizeof(output)));
		CHECK(is_stem_of(output, 255, expected));
	}

	/* A word that is not one is dropped whole.  A cut inside it would
	 * leave the front of it welded to the suffix, and a .l.gz off the
	 * front of log-2024 is a name nothing opens. */
	for (size_t i = 0; i < sizeof(refused) / sizeof(*refused); i++) {
		snprintf(header, sizeof(header),
			 "attachment; filename=\"%s.%s.gz\"", stem, refused[i]);
		ASSERT(http_content_disposition_filename(header, output,
							 sizeof(output)));
		/* the whole word goes, so the name is the stem and .gz and
		 * comes out two bytes under the budget rather than at it */
		CHECK(is_stem_of(output, 253, ".gz"));
	}

	/* Only the last component in front of the dot is judged, so in
	 * a.b.gz it is b that has to be a plain word, and a is stem. */
	snprintf(header, sizeof(header), "attachment; filename=\"%s.a.b.gz\"",
		 stem);
	ASSERT(http_content_disposition_filename(header, output, sizeof(output)));
	CHECK(is_stem_of(output, 255, ".b.gz"));
}

TEST(a_hidden_name_keeps_its_leading_dot)
{
	char output[16];

	ASSERT(http_content_disposition_filename
	       ("attachment; filename=\".hiddenrc\"", output, sizeof(output)));
	CHECK_STR(output, ".hiddenrc");
}

TEST(a_cut_never_lands_inside_a_character)
{
	char output[1024];
	char header[2048];
	char value[1024];
	size_t n = 0;
	size_t length;

	/* Three-byte characters: a cut at 255 would fall inside one. */
	for (int i = 0; i < 90; i++)
		n += sprintf(value + n, "%%E6%%B5%%8B");
	sprintf(value + n, ".bin");
	snprintf(header, sizeof(header), "attachment; filename*=UTF-8''%s",
		 value);

	ASSERT(http_content_disposition_filename(header, output,
						 sizeof(output)));
	length = strlen(output);
	CHECK(length == 253);
	CHECK_STR(output + length - 4, ".bin");
	/* 83 whole characters and a terminator: no partial one. */
	CHECK_EQ(length, (size_t) 83 * 3 + 4);
	for (size_t i = 0; i < 83; i++)
		CHECK_LSTR(output + i * 3, 3, "\xE6\xB5\x8B");
}

TEST(a_name_with_no_room_for_a_stem_is_rejected)
{
	char output[8];

	/* The extension alone fills the buffer. */
	ASSERT(!http_content_disposition_filename
	       ("attachment; filename=\"aaaa.tar.gz\"", output,
		sizeof(output)));
	CHECK_STR(output, "");
}

TEST(a_long_extended_name_still_wins_over_the_legacy_one)
{
	char output[1024];
	char header[2048];
	char value[1024];
	size_t n = 0;
	size_t length;

	for (int i = 0; i < 90; i++)
		n += sprintf(value + n, "%%E6%%B5%%8B");
	sprintf(value + n, ".txt");
	snprintf(header, sizeof(header),
		 "attachment; filename=\"short.txt\"; filename*=UTF-8''%s",
		 value);

	ASSERT(http_content_disposition_filename(header, output,
						 sizeof(output)));
	length = strlen(output);
	CHECK_EQ(length, (size_t) 83 * 3 + 4);
	CHECK_STR(output + length - 4, ".txt");
}

TEST(a_name_too_long_for_the_buffer_itself_is_refused)
{
	char output[32];

	/* Cutting only applies to a name that was read in full, and this
	 * one does not fit to begin with.  The caller falls back to the
	 * name from the URL, which is the right answer for a buffer this
	 * small: conn_t keeps a whole MAX_STRING, so in practice a value
	 * that long never arrives. */
	ASSERT(!http_content_disposition_filename
	       ("attachment; filename=\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txt\"",
		output, sizeof(output)));
	CHECK_STR(output, "");
}

TEST(no_usable_filename_clears_an_old_result)
{
	char output[32] = "old-response.txt";

	ASSERT(!http_content_disposition_filename
	       ("attachment; size=123", output, sizeof(output)));
	CHECK_STR(output, "");
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

	REGISTER_DESC
	    (an_extended_filename_is_preferred_over_the_legacy_fallback,
	     "an extended filename is preferred over its legacy fallback");
	REGISTER_DESC(an_invalid_extended_filename_falls_back_to_filename,
		      "an invalid extended filename falls back to filename");
	REGISTER_DESC(an_unsupported_extended_charset_falls_back_to_filename,
		      "an unsupported extended charset falls back to filename");
	REGISTER_DESC(a_legacy_filename_is_used_when_no_extended_one_exists,
		      "a legacy filename is used when no extended one exists");
	REGISTER_DESC(decoded_separators_and_control_characters_are_sanitized,
		      "decoded separators and control characters are sanitized");
	REGISTER_DESC(a_legacy_filename_is_sanitized_after_quoted_pair_copying,
		      "a copied legacy filename is sanitized");
	REGISTER_DESC(an_unusable_extended_filename_falls_back_to_filename,
		      "an unusable extended filename falls back to filename");
	REGISTER_DESC(a_long_name_is_cut_down_to_fit_a_directory_entry,
		      "a long name is cut down to fit a directory entry");
	REGISTER_DESC(a_compressed_name_keeps_both_of_its_suffixes,
		      "a compressed name keeps both of its suffixes");
	REGISTER_DESC(a_pair_is_not_made_of_a_final_word_that_is_too_long,
		      "a final word over the budget is not an extension");
	REGISTER_DESC(an_ordinary_suffix_does_not_take_the_word_in_front_of_it,
		      "an ordinary suffix keeps only itself");
	REGISTER_DESC(each_compressor_suffix_takes_the_word_in_front_of_it,
		      "each compressor suffix takes the word in front of it");
	REGISTER_DESC(only_a_plain_word_before_a_compressor_suffix_is_kept,
		      "only a plain word before a compressor suffix is kept");
	REGISTER_DESC(a_hidden_name_keeps_its_leading_dot,
		      "a hidden name keeps its leading dot");
	REGISTER_DESC(a_cut_never_lands_inside_a_character,
		      "a cut never lands inside a character");
	REGISTER_DESC(a_name_with_no_room_for_a_stem_is_rejected,
		      "a name with no room for a stem is rejected");
	REGISTER_DESC(a_long_extended_name_still_wins_over_the_legacy_one,
		      "a long extended name still wins over filename");
	REGISTER_DESC(a_name_too_long_for_the_buffer_itself_is_refused,
		      "a name too long for the buffer is refused");
	REGISTER_DESC(no_usable_filename_clears_an_old_result,
		      "no usable filename leaves an empty result");

	RUN_ALL();
	return DONE();
}
