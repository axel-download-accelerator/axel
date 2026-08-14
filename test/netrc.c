// SPDX-FileCopyrightText: Copyright 2026 Ismael Luceno <ismael@iodev.co.uk>
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * test/netrc.c — .netrc parsing tests
 *
 * The parser works directly on a read-only mapping with no NUL to stop at,
 * so most of what is worth testing here is where it stops: a machine name
 * has to match in full, a keyword has to match in full, and the last token
 * of the file has to be the last thing read.  Each of those was a bug.
 *
 * Every case writes a real file and maps it, because the size and the
 * layout of that file are what the code is sensitive to.
 */

#include "config.h"

#include "harness.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "netrc.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

static char tmpdir[] = "/tmp/axel-netrc.XXXXXX";
static char netrc_path[sizeof(tmpdir) + sizeof("/.netrc")];

struct creds {
	char user[64];
	char pass[64];
};

/* Returns 0 on success, like the syscalls it wraps. */
static int
write_netrc(const void *data, size_t len)
{
	FILE *fp = fopen(netrc_path, "w");

	if (!fp) {
		perror(netrc_path);
		return -1;
	}

	if (len && fwrite(data, len, 1, fp) != 1) {
		perror(netrc_path);
		fclose(fp);
		return -1;
	}

	if (fclose(fp)) {
		perror(netrc_path);
		return -1;
	}

	return 0;
}

static netrc_t *
netrc_with(const char *body)
{
	if (write_netrc(body, strlen(body)))
		return NULL;

	return netrc_init(netrc_path);
}

static void
lookup(netrc_t *netrc, const char *host, struct creds *c)
{
	*c->user = *c->pass = 0;
	netrc_parse(netrc, host, c->user, sizeof(c->user),
		    c->pass, sizeof(c->pass));
}

/* ── the shapes a .netrc comes in ─────────────────────────────────────── */

TEST(an_entry_on_one_line_is_found)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n");

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");
	CHECK_STR(c.pass, "s3cr3t");
	netrc_free(netrc);
}

TEST(an_entry_spread_over_several_lines_is_found)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com\n"
		"\tlogin alice\n"
		"\tpassword s3cr3t\n");

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");
	CHECK_STR(c.pass, "s3cr3t");
	netrc_free(netrc);
}

TEST(the_first_matching_entry_wins)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n"
		"machine example.com login bob password hunter2\n");

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");
	CHECK_STR(c.pass, "s3cr3t");
	netrc_free(netrc);
}

TEST(default_supplies_the_hosts_with_no_entry)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n"
		"default login guest password anon\n");

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");
	CHECK_STR(c.pass, "s3cr3t");

	lookup(netrc, "anything.test", &c);
	CHECK_STR(c.user, "guest");
	CHECK_STR(c.pass, "anon");
	netrc_free(netrc);
}

TEST(a_login_without_a_password_leaves_the_password_alone)
{
	struct creds c;
	netrc_t *netrc = netrc_with("machine example.com login alice\n");

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");
	CHECK_STR(c.pass, "");
	netrc_free(netrc);
}

TEST(a_host_with_no_entry_gets_nothing)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n");

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "other.test", &c);
	CHECK_STR(c.user, "");
	CHECK_STR(c.pass, "");
	netrc_free(netrc);
}

/* ── matching in full ─────────────────────────────────────────────────── */

/* The machine name used to be compared over the length of the token found
 * in the file, so an entry only had to be a prefix of the host being
 * contacted.  conn_set() runs again for every redirect, which let the
 * server on the other end pick the name. */
TEST(an_entry_is_not_offered_to_a_host_it_merely_prefixes)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n");

	ASSERT_NOTNULL(netrc);

	lookup(netrc, "example.com.attacker.test", &c);
	CHECK_STR(c.user, "");
	CHECK_STR(c.pass, "");

	/* and the other way around, a host that prefixes the entry */
	lookup(netrc, "example.co", &c);
	CHECK_STR(c.user, "");
	CHECK_STR(c.pass, "");

	/* the exact name still works */
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");
	netrc_free(netrc);
}

/* Same comparison read the keywords, so "machine log" was taken for a
 * "login" keyword and the name following it eaten as a user name. */
TEST(a_machine_named_like_a_keyword_is_not_one)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine log login alice password s3cr3t\n"
		"machine example.com login bob password hunter2\n");

	ASSERT_NOTNULL(netrc);

	lookup(netrc, "log", &c);
	CHECK_STR(c.user, "alice");
	CHECK_STR(c.pass, "s3cr3t");

	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "bob");
	CHECK_STR(c.pass, "hunter2");
	netrc_free(netrc);
}

/* The other side of it: a word standing where a keyword goes is only the
 * keyword it spells out in full.  "log" is not "login", so the parse
 * stops at it rather than taking the word after it for a user name. */
TEST(a_word_that_merely_starts_a_keyword_is_not_one)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com log alice password s3cr3t\n");

	ASSERT_NOTNULL(netrc);

	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "");
	CHECK_STR(c.pass, "");
	netrc_free(netrc);
}

/* ── the keywords that carry nothing we want ──────────────────────────── */

TEST(account_is_skipped_with_its_argument)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice account acct "
		"password s3cr3t\n");

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");
	CHECK_STR(c.pass, "s3cr3t");
	netrc_free(netrc);
}

TEST(a_macdef_body_is_skipped_up_to_the_empty_line)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n"
		"macdef init\n"
		"\tbinary\n"
		"\tcd pub\n"
		"\n"
		"machine other.test login bob password hunter2\n");

	ASSERT_NOTNULL(netrc);

	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");

	/* the entry after the macro body is still reachable */
	lookup(netrc, "other.test", &c);
	CHECK_STR(c.user, "bob");
	CHECK_STR(c.pass, "hunter2");

	/* and the body itself is not read as entries */
	lookup(netrc, "pub", &c);
	CHECK_STR(c.user, "");
	netrc_free(netrc);
}

TEST(a_macdef_that_runs_to_the_end_of_the_file_terminates)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n"
		"macdef init\n"
		"\tbinary\n");

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");
	netrc_free(netrc);
}

/* ── comments ─────────────────────────────────────────────────────────── */

/* Not part of the format proper, but wget honours them and people have
 * them in their files.  The parser gives up on anything it does not know,
 * so without this a single comment hides the whole file. */
TEST(a_comment_runs_to_the_end_of_its_line)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"# credentials\n"
		"machine example.com login alice password s3cr3t # mine\n"
		"#machine disabled.test login bob password hunter2\n"
		"machine other.test login carol password sesame\n");

	ASSERT_NOTNULL(netrc);

	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");
	CHECK_STR(c.pass, "s3cr3t");

	lookup(netrc, "disabled.test", &c);
	CHECK_STR(c.user, "");
	CHECK_STR(c.pass, "");

	lookup(netrc, "other.test", &c);
	CHECK_STR(c.user, "carol");
	netrc_free(netrc);
}

/* ── the end of the file ──────────────────────────────────────────────── */

/**
 * Parse a page-sized .netrc with nothing readable after it, and exit 0
 * if the credentials came back right.
 *
 * The overrun this looks for is a read of the byte just past the mapping,
 * which normally lands in whatever the allocator put there and goes
 * unnoticed.  To make it fault we hand the kernel a one-page hole with a
 * PROT_NONE page above it, and let it place the mapping there: Linux
 * fills the highest gap that fits, and the hole is it.  Where that does
 * not hold the mapping simply lands elsewhere and the case still runs,
 * only without the guard, so this needs no fallback.
 *
 * It runs in a child of its own because the failure it is after is a
 * SIGSEGV, and a suite that dies takes the rest of its tests with it.
 */
static int
page_sized_child(void)
{
	const size_t page = (size_t)sysconf(_SC_PAGESIZE);
	struct creds c;
	netrc_t *netrc;
	char *hole;

	hole = mmap(NULL, 2 * page, PROT_NONE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (hole == MAP_FAILED || munmap(hole, page))
		return 1;

	netrc = netrc_init(netrc_path);
	if (!netrc)
		return 1;

	lookup(netrc, "example.com", &c);
	if (strcmp(c.user, "alice") || strcmp(c.pass, "s3cr3t"))
		return 1;

	/* a host with no entry, so the parse runs to the very end */
	lookup(netrc, "nowhere.test", &c);
	if (*c.user || *c.pass)
		return 1;

	netrc_free(netrc);
	return 0;
}

/* A file whose size is a multiple of the page size has nothing mapped
 * after it, so an overrun faults instead of reading padding.  memtok()
 * used not to subtract the token from what was left of the buffer, and
 * this is the case that catches it. */
TEST(a_page_sized_file_is_not_read_past)
{
	static const char entry[] =
		"machine example.com login alice password s3cr3t\n";
	const size_t page = (size_t)sysconf(_SC_PAGESIZE);
	size_t pad, i;
	int status;
	pid_t pid;
	char *buf;
	int err;

	ASSERT_GT(page, sizeof(entry));
	pad = page - (sizeof(entry) - 1);

	buf = malloc(page);
	ASSERT_NOTNULL(buf);

	/* comment lines, so the parser has to walk every one of them
	   before it reaches the entry at the very end */
	memset(buf, '#', pad);
	for (i = 63; i < pad; i += 64)
		buf[i] = '\n';
	buf[pad - 1] = '\n';
	memcpy(buf + pad, entry, sizeof(entry) - 1);

	err = write_netrc(buf, page);
	free(buf);
	ASSERT_OK(err);

	pid = fork();
	ASSERT_NE(pid, -1);
	if (!pid)
		_exit(page_sized_child());

	ASSERT_NE(waitpid(pid, &status, 0), -1);
	CHECK_EQ(WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status),
		 0);
}

TEST(an_entry_cut_short_by_the_end_of_the_file_is_not_read_past)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n"
		"machine");

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");
	netrc_free(netrc);
}

TEST(a_file_of_nothing_but_separators_yields_nothing)
{
	struct creds c;
	netrc_t *netrc = netrc_with(" \t\n\n \n");

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "");
	CHECK_STR(c.pass, "");
	netrc_free(netrc);
}

/* ── what the parser refuses to guess at ──────────────────────────────── */

TEST(an_unknown_keyword_stops_the_parse)
{
	struct creds c;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n"
		"bogus\n"
		"machine other.test login bob password hunter2\n");

	ASSERT_NOTNULL(netrc);

	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "alice");

	/* everything past the keyword it could not place is ignored */
	lookup(netrc, "other.test", &c);
	CHECK_STR(c.user, "");
	netrc_free(netrc);
}

/* ── the output buffers ───────────────────────────────────────────────── */

TEST(a_value_longer_than_the_buffer_is_truncated)
{
	struct {
		char user[8];
		char guard_a[16];
		char pass[8];
		char guard_b[16];
	} b;
	size_t i, clobbered = 0;
	netrc_t *netrc;

	netrc = netrc_with("machine example.com "
			   "login averylongusername "
			   "password averylongpassword\n");
	ASSERT_NOTNULL(netrc);

	memset(&b, 0x7f, sizeof(b));
	*b.user = *b.pass = 0;
	netrc_parse(netrc, "example.com", b.user, sizeof(b.user),
		    b.pass, sizeof(b.pass));

	CHECK_STR(b.user, "averylo");
	CHECK_STR(b.pass, "averylo");

	for (i = 0; i < sizeof(b.guard_a); i++)
		clobbered += b.guard_a[i] != 0x7f;
	for (i = 0; i < sizeof(b.guard_b); i++)
		clobbered += b.guard_b[i] != 0x7f;
	CHECK_EQ(clobbered, 0);

	netrc_free(netrc);
}

/* conf_auth_setup() only asks when it has nothing, and relies on being
 * able to tell "no entry" from "an empty one". */
TEST(a_host_with_no_entry_leaves_the_buffers_untouched)
{
	char user[64], pass[64];
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n");

	ASSERT_NOTNULL(netrc);
	strcpy(user, "from-the-url");
	strcpy(pass, "also-from-the-url");
	netrc_parse(netrc, "other.test", user, sizeof(user),
		    pass, sizeof(pass));
	CHECK_STR(user, "from-the-url");
	CHECK_STR(pass, "also-from-the-url");
	netrc_free(netrc);
}

/* ── files that cannot be used ────────────────────────────────────────── */

TEST(a_missing_file_yields_no_handle)
{
	char missing[sizeof(netrc_path) + 8];

	snprintf(missing, sizeof(missing), "%s/absent", tmpdir);
	CHECK_NULL(netrc_init(missing));
}

TEST(an_empty_file_yields_no_handle)
{
	ASSERT_OK(write_netrc("", 0));
	CHECK_NULL(netrc_init(netrc_path));
}

/* open(2) is happy to hand back a directory, and mmap(2) is not. */
TEST(a_directory_yields_no_handle)
{
	CHECK_NULL(netrc_init(tmpdir));
}

/* Everything below conf_netrc_set() has to cope with the lookup being
 * switched off. */
TEST(no_handle_at_all_is_not_a_fault)
{
	struct creds c;

	lookup(NULL, "example.com", &c);
	CHECK_STR(c.user, "");
	CHECK_STR(c.pass, "");
	netrc_free(NULL);
}

/* ── picking the file ─────────────────────────────────────────────────── */

TEST(an_empty_path_falls_back_to_the_NETRC_variable)
{
	static const char body[] =
		"machine example.com login env password var\n";
	struct creds c;
	netrc_t *netrc;

	ASSERT_OK(write_netrc(body, sizeof(body) - 1));
	ASSERT_OK(setenv("NETRC", netrc_path, 1));

	netrc = netrc_init("");
	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "env");
	CHECK_STR(c.pass, "var");
	netrc_free(netrc);

	/* and a NULL path means the same thing */
	netrc = netrc_init(NULL);
	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "env");
	netrc_free(netrc);

	unsetenv("NETRC");
}

TEST(without_NETRC_the_file_is_the_one_under_HOME)
{
	char home_netrc[sizeof(tmpdir) + sizeof("/.netrc")];
	char *saved_home = getenv("HOME");
	struct creds c;
	netrc_t *netrc;
	FILE *fp;

	saved_home = saved_home ? strdup(saved_home) : NULL;
	unsetenv("NETRC");

	snprintf(home_netrc, sizeof(home_netrc), "%s/.netrc", tmpdir);
	fp = fopen(home_netrc, "w");
	ASSERT_NOTNULL(fp);
	fputs("machine example.com login home password dir\n", fp);
	ASSERT_OK(fclose(fp));

	ASSERT_OK(setenv("HOME", tmpdir, 1));
	netrc = netrc_init("");

	if (saved_home) {
		setenv("HOME", saved_home, 1);
		free(saved_home);
	} else {
		unsetenv("HOME");
	}

	ASSERT_NOTNULL(netrc);
	lookup(netrc, "example.com", &c);
	CHECK_STR(c.user, "home");
	CHECK_STR(c.pass, "dir");
	netrc_free(netrc);

	remove(home_netrc);
}

/* ── the lookup cache ─────────────────────────────────────────────────── */

TEST(repeated_lookups_give_the_same_answer)
{
	struct creds first, again;
	netrc_t *netrc = netrc_with(
		"machine example.com login alice password s3cr3t\n"
		"default login guest password anon\n");

	ASSERT_NOTNULL(netrc);

	lookup(netrc, "example.com", &first);
	lookup(netrc, "other.test", &again);	/* a second host in between */
	lookup(netrc, "example.com", &again);
	CHECK_STR(again.user, first.user);
	CHECK_STR(again.pass, first.pass);

	/* the entry that came from default is remembered as such */
	lookup(netrc, "other.test", &again);
	CHECK_STR(again.user, "guest");
	CHECK_STR(again.pass, "anon");

	netrc_free(netrc);
}

static const struct {
	const char *host;
	const char *user;
	const char *pass;
} racer_want[] = {
	{ "a.test", "ua", "pa" },
	{ "b.test", "ub", "pb" },
	{ "c.test", "uc", "pc" },
	{ "d.test", "guest", "anon" },
};

struct racer {
	netrc_t *netrc;
	unsigned rounds;
	unsigned mismatches;
};

static void *
racer_run(void *p)
{
	struct racer *r = p;
	unsigned i;

	for (i = 0; i < r->rounds; i++) {
		size_t k = i % ARRAY_SIZE(racer_want);
		struct creds c;

		lookup(r->netrc, racer_want[k].host, &c);
		if (strcmp(c.user, racer_want[k].user)
		    || strcmp(c.pass, racer_want[k].pass))
			r->mismatches++;
	}

	return NULL;
}

/* search.c runs conn_set() from several threads over one conf_t, so the
 * cache is written concurrently.  Run this one under -fsanitize=thread. */
TEST(concurrent_lookups_agree)
{
	enum { THREADS = 8, ROUNDS = 250 };
	struct racer racer[THREADS];
	pthread_t thread[THREADS];
	unsigned mismatches = 0;
	unsigned started = 0;
	unsigned i;
	netrc_t *netrc = netrc_with(
		"machine a.test login ua password pa\n"
		"machine b.test login ub password pb\n"
		"machine c.test login uc password pc\n"
		"default login guest password anon\n");

	ASSERT_NOTNULL(netrc);

	for (i = 0; i < THREADS; i++) {
		racer[i].netrc = netrc;
		racer[i].rounds = ROUNDS;
		racer[i].mismatches = 0;
		if (pthread_create(&thread[i], NULL, racer_run, &racer[i]))
			break;
		started++;
	}

	for (i = 0; i < started; i++) {
		pthread_join(thread[i], NULL);
		mismatches += racer[i].mismatches;
	}

	CHECK_EQ(started, THREADS);
	CHECK_EQ(mismatches, 0);
	netrc_free(netrc);
}

/* ── scratch directory ────────────────────────────────────────────────── */

static void
cleanup(void)
{
	remove(netrc_path);
	rmdir(tmpdir);
}

int
main(void)
{
	if (!mkdtemp(tmpdir)) {
		perror("mkdtemp");
		return 99;
	}
	snprintf(netrc_path, sizeof(netrc_path), "%s/netrc", tmpdir);
	atexit(cleanup);

	REGISTER_DESC(an_entry_on_one_line_is_found,
		      "an entry on one line yields its login and password");
	REGISTER_DESC(an_entry_spread_over_several_lines_is_found,
		      "an entry may be spread over several lines");
	REGISTER_DESC(the_first_matching_entry_wins,
		      "the first entry for a machine wins over a later one");
	REGISTER_DESC(default_supplies_the_hosts_with_no_entry,
		      "default supplies the hosts with no entry of their own");
	REGISTER_DESC(a_login_without_a_password_leaves_the_password_alone,
		      "an entry with no password yields the login alone");
	REGISTER_DESC(a_host_with_no_entry_gets_nothing,
		      "a host with no entry gets no credentials");
	REGISTER_DESC(an_entry_is_not_offered_to_a_host_it_merely_prefixes,
		      "an entry is not offered to a host it merely prefixes");
	REGISTER_DESC(a_machine_named_like_a_keyword_is_not_one,
		      "a machine named like the start of a keyword is not one");
	REGISTER_DESC(a_word_that_merely_starts_a_keyword_is_not_one,
		      "a word that only starts a keyword is not that keyword");
	REGISTER_DESC(account_is_skipped_with_its_argument,
		      "account is skipped along with its argument");
	REGISTER_DESC(a_macdef_body_is_skipped_up_to_the_empty_line,
		      "a macdef body is skipped up to the empty line");
	REGISTER_DESC(a_macdef_that_runs_to_the_end_of_the_file_terminates,
		      "a macdef with no empty line after it ends at the file");
	REGISTER_DESC(a_comment_runs_to_the_end_of_its_line,
		      "a comment runs to the end of its line and hides it");
	REGISTER_DESC(a_page_sized_file_is_not_read_past,
		      "a file the size of a page is not read past its end");
	REGISTER_DESC(an_entry_cut_short_by_the_end_of_the_file_is_not_read_past,
		      "a keyword with no argument at the end of the file ends it");
	REGISTER_DESC(a_file_of_nothing_but_separators_yields_nothing,
		      "a file of nothing but separators yields no credentials");
	REGISTER_DESC(an_unknown_keyword_stops_the_parse,
		      "an unknown keyword stops the parse rather than being guessed");
	REGISTER_DESC(a_value_longer_than_the_buffer_is_truncated,
		      "a value longer than the buffer is truncated, not overrun");
	REGISTER_DESC(a_host_with_no_entry_leaves_the_buffers_untouched,
		      "a host with no entry leaves the caller's buffers alone");
	REGISTER_DESC(a_missing_file_yields_no_handle,
		      "a .netrc that is not there yields no handle");
	REGISTER_DESC(an_empty_file_yields_no_handle,
		      "an empty .netrc yields no handle");
	REGISTER_DESC(a_directory_yields_no_handle,
		      "a .netrc that is a directory yields no handle");
	REGISTER_DESC(no_handle_at_all_is_not_a_fault,
		      "a lookup with the .netrc switched off is a no-op");
	REGISTER_DESC(an_empty_path_falls_back_to_the_NETRC_variable,
		      "an empty path takes the file from the NETRC variable");
	REGISTER_DESC(without_NETRC_the_file_is_the_one_under_HOME,
		      "with no NETRC set the file is the one under HOME");
	REGISTER_DESC(repeated_lookups_give_the_same_answer,
		      "a second lookup of a host gives what the first did");
	REGISTER_DESC(concurrent_lookups_agree,
		      "lookups from several threads all give the right answer");

	RUN_ALL();
	return DONE();
}
