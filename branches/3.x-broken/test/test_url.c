/* Test url.c */

#include "tests.h"

static void test_url_encode();
static void test_url_encode_single(const char* input, const char* expected);

static void test_url_heuristic_decode();
static void test_url_heuristic_decode_single(const char* input, const char* expected);

static void test_url_parse_unencoded();
static void test_url_parse_unencoded_single(const char* urlstr, const struct url_t* expected);
static void test_url_parse_unencoded_single_long(const char* urlstr,
	int proto,
	const char* user, const char* pass,
	const char* host, unsigned int port,
	const char* dir, const char* filename, const char* query
);

static void test_url_str();
static void test_url_str_single(const struct url_t* u, const char* expected);

static void test_url_request();
static void test_url_request_single(const struct url_t* u, const char* expected);

static const url_t* test_helper_urlgen(
	int proto,
	const char* user, const char* pass,
	const char* host, unsigned int port,
	const char* dir, const char* filename, const char* query
);



void test_url_suite() {
	CU_pSuite ps = CU_add_suite("URL functions", NULL, NULL);
	
	CU_add_test(ps, "url_encode", test_url_encode);
	CU_add_test(ps, "url_heuristic_decode", test_url_heuristic_decode);
	CU_add_test(ps, "url_parse_unencoded", test_url_parse_unencoded);
	CU_add_test(ps, "url_str", test_url_str);
	CU_add_test(ps, "url_request", test_url_request);
}

static void test_url_encode() {
	test_url_encode_single("http://example.org/1/a/s/d?x=y", "http://example.org/1/a/s/d?x=y");
	test_url_encode_single("http://example.org/2/a /s/d?x=y z", "http://example.org/2/a%20/s/d?x=y%20z");
	test_url_encode_single("http://example.org/3/a\n/s/d?x=y\nz", "http://example.org/3/a%0A/s/d?x=y%0Az");
	test_url_encode_single("http://example.org/4/%", "http://example.org/4/%25");
	test_url_encode_single("http://example.org/5/%26", "http://example.org/5/%2526");
	test_url_encode_single("http://example.org/", "http://example.org/");
}

static void test_url_encode_single(const char* input, const char* expected) {
	char* res = url_encode(input);
	
	CU_ASSERT_STRING_EQUAL(res, expected);
	
	free(res);
}

static void test_url_heuristic_decode() {
	// These tests are naturally heuristic and test wget compatibility
	test_url_heuristic_decode_single("http://example.org/a/s/d", "http://example.org/a/s/d");
	test_url_heuristic_decode_single("\n", "\n");
	test_url_heuristic_decode_single("a%25b", "a%b");
	test_url_heuristic_decode_single("%0a", "\n");
	test_url_heuristic_decode_single("%0A", "\n");
	test_url_heuristic_decode_single("%x0a", "%x0a");
}

static void test_url_heuristic_decode_single(const char* input, const char* expected) {
	char* res = url_heuristic_decode(input);
	
	CU_ASSERT_STRING_EQUAL(res, expected);
	
	free(res);
}


static void test_url_parse_unencoded() {
	test_url_parse_unencoded_single_long("http://example.org:4223/",
		PROTO_HTTP,
		NULL, NULL,
		"example.org", 4223,
		NULL, NULL, NULL
		);
	
	test_url_parse_unencoded_single_long("http://example.org:1",
		PROTO_HTTP,
		NULL, NULL,
		"example.org", 1,
		NULL, NULL, NULL
		);
	
	test_url_parse_unencoded_single_long("http://example.org:1/a/b/?query",
		PROTO_HTTP,
		NULL, NULL,
		"example.org", 1,
		"a/b", NULL, "query"
		);
	
	test_url_parse_unencoded_single_long("http://example.org:1/filename",
		PROTO_HTTP,
		NULL, NULL,
		"example.org", 1,
		NULL, "filename", NULL
		);
	
	test_url_parse_unencoded_single_long("http://example.org/1/2/3asd?x=y&z=.",
		PROTO_HTTP,
		NULL, NULL,
		"example.org", 80,
		"1/2", "3asd", "x=y&z=."
		);
	
	test_url_parse_unencoded_single_long("http://a.x:b@example.org/1/2/3asd?x=y&z=.",
		PROTO_HTTP,
		"a.x", "b",
		"example.org", 80,
		"1/2", "3asd", "x=y&z=."
		);
	
	test_url_parse_unencoded_single_long("a.x:b@example.org/1/2/3asd?x=y&z=.",
		PROTO_DEFAULT,
		"a.x", "b",
		"example.org", 80,
		"1/2", "3asd", "x=y&z=."
		);
	
	#ifdef FTP
		test_url_parse_unencoded_single_long("ftp://example.org/1/2/3",
			PROTO_FTP,
			NULL, NULL,
			"example.org", 4223,
			"1/2", "3asd", "x=y&z=."
			);
	#endif
}

static void test_url_parse_unencoded_single(const char* urlstr, const struct url_t* expected) {
	url_t* got = url_parse_unencoded(urlstr);
	
	CU_ASSERT_STRING(got->proto, expected->proto);
	CU_ASSERT_STRING_EQUAL(got->host, expected->host);
	CU_ASSERT_STRING(got->port, expected->port);
	CU_ASSERT_STRING_EQUAL(got->dir, expected->dir);
	CU_ASSERT_STRING_EQUAL(got->filename, expected->filename);
	CU_ASSERT_STRING_EQUAL(got->query, expected->query);
	CU_ASSERT_STRING_EQUAL(got->user, expected->user);
	CU_ASSERT_STRING_EQUAL(got->pass, expected->pass);
	
	url_free(got);
}

static void test_url_parse_unencoded_single_long(const char* urlstr,
	int proto,
	const char* user, const char* pass,
	const char* host, unsigned int port,
	const char* dir, const char* filename, const char* query
) {
	url_t* expected = test_helper_urlgen(
		proto,
		user, pass
		host, port
		dir, filename, query
		);
	
	test_url_parse_unencoded_single(urlstr, expected);
	
	url_free(expected);
}

static void test_url_str() {
	url* u = parse_url("http://example.org/");
	
	test_url_str_single(u, "http://example.org/");
	heap_strcpy(&(u->dir), "a/b");
	test_url_str_single(u, "http://example.org/a/b/");
	heap_strcpy(&(u->filename), "c");
	test_url_str_single(u, "http://example.org/a/b/c");
	heap_strcpy(&(u->query), "query");
	test_url_str_single(u, "http://example.org/a/b/c?query");
	u->port = 422;
	test_url_str_single(u, "http://example.org:422/a/b/c?query");
	heap_strcpy(&(u->user), "asdf");
	test_url_str_single(u, "http://asdf@example.org:422/a/b/c?query");
	heap_strcpy(&(u->password), "secret");
	test_url_str_single(u, "http://asdf:secret@example.org:422/a/b/c?query");
	heap_strcpy(&(u->password), "");
	test_url_str_single(u, "http://asdf:@example.org:422/a/b/c?query");
	
	url_free(u);
}

static void test_url_str_single(const struct url_t* u, const char* expected) {
	char* got = url_str(u);
	
	CU_ASSERT_STRING_EQUAL(got, expected);
	
	free(got);
}

static void test_url_request() {
	url* u = parse_url("http://example.org/");
	
	test_url_str_single(u, "/");
	heap_strcpy(&(u->dir), "a/b");
	test_url_str_single(u, "/a/b/");
	heap_strcpy(&(u->filename), "c");
	test_url_str_single(u, "/a/b/c");
	heap_strcpy(&(u->query), "query");
	test_url_str_single(u, "/a/b/c?query");
	
	url_free(u);
}

static void test_url_request_single(const struct url_t* u, const char* expected) {
	char* got = url_request(u);
	
	CU_ASSERT_STRING_EQUAL(got, expected);
	
	free(got);
}

static const url_t* test_helper_urlgen(
	int proto,
	const char* user, const char* pass,
	const char* host, unsigned int port,
	const char* dir, const char* filename, const char* query
) {
	struct url_t* res = malloc(sizeof(struct url_t));
	
	res->proto = proto;
	res->host = strdup(host);
	res->port = port;
	res->dir = strdup(dir);
	res->filename = strdup(filename);
	res->query = strdup(query);
	res->user = strdup(user);
	res->pass = strdup(pass);
	
	return res;
}
