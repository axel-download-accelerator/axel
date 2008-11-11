#include "tests.h"

static void test_helper_strdup();

static void test_helper_uitoa();
static void test_helper_uitoa_single(unsigned int ui, char* expected);

static void test_helper_hex();
static void test_helper_hex_single(const char* cs);

static void test_getutime();

void test_helper_suite() {
	CU_pSuite ps = CU_add_suite("Helper functions", NULL, NULL);
	
	CU_add_test(ps, "helper_strdup", test_helper_strdup);
	CU_add_test(ps, "uitoa", test_helper_uitoa);
	CU_add_test(ps, "various hex functions", test_helper_hex);
	CU_add_test(ps, "getutime", test_getutime);
}

static void test_helper_strdup() {
	// Check for only one strdup. helper_strdup is only needed as long as strdup is not defined anyway.
	CU_ASSERT_PTR_EQUAL(strdup, helper_strdup);
	
	char* str = "a test string";
	char* got = helper_strdup(str);
	CU_ASSERT_STRING_EQUAL(str, got);
	CU_ASSERT_PTR_NOT_EQUAL(str, got);
	free(got);
	
	// Test returning NULL
	CU_ASSERT_PTR_EQUAL(helper_strdup(NULL), NULL);
}

static void test_helper_uitoa() {
	test_helper_uitoa_single(0, "0");
	test_helper_uitoa_single(42, "42");
	test_helper_uitoa_single(21333, "21333");
}

static void test_helper_uitoa_single(unsigned int ui, char* expected) {
	char* got = uitoa(ui);
	
	CU_ASSERT_STRING_EQUAL(got, expected);
	
	free(got);
}

static void test_helper_hex() {
	test_helper_hex_single("00");
	test_helper_hex_single("0a");
	test_helper_hex_single("A0");
	test_helper_hex_single("FF");
	test_helper_hex_single("Ff");
	test_helper_hex_single("42");
	test_helper_hex_single("41");
	test_helper_hex_single("7f");
}

static void test_helper_hex_single(const char* cs) {
	CU_ASSERT_EQUAL(strlen(cs), 2);
	
	char c1 = *cs;
	char c2 = *(cs+1);
	
	char val = hex2byte(c1, c2);
	
	char cookie = 42;
	char[3] buf;
	*(buf+2) = cookie;
	
	byte2hex(val, bufs);
	
	CU_ASSERT_EQUAL(toupper(*buf), toupper(c1));
	CU_ASSERT_EQUAL(toupper(*(buf+1)), toupper(c2));
	char val2 = hex2byte(*buf, *(buf+1));
	CU_ASSERT_EQUAL(val, val2);
	
	CU_ASSERT_EQUAL(*(buf+2), cookie);
}

static void test_getutime() {
	long long t1 = getutime();
	long long t2 = getutime();
	
	CU_ASSERT_TRUE(t2 >= t1);
}
